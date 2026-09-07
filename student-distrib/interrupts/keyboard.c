#include "keyboard.h"

#include "../i8259.h"
#include "../lib.h"
#include "../x86_desc.h"
#include "syscall_handler.h"

#define VIDEO_MEM_INDEX 0xB8

static void keyboard_backspace(void);
// static void keyboard_enter(void);
static void clear_terminal_buffer(void);

static int switch_terminal(int);

/* scan code => keycap mapping 10 numbers and 26 letters */
/* From https://wiki.osdev.org/PS/2_Keyboard */
const uint8_t key_map[64] = {'\0', '\0' /*esc*/, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\0' /*Backspace*/,
                             '\0' /*tab*/, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\0' /*enter*/,
                             '\0' /*L-Ctrl*/, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
                             '\0' /*L-Shift*/, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', '\0' /*R-Shift*/,
                             '\0', '\0' /*L-Alt*/, ' ', '\0' /*CapsLock*/};

const uint8_t key_map_shift[64] =
    {'\0', '\0' /*esc*/, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\0' /*Backspace*/,
     '\0' /*tab*/, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\0' /*enter*/,
     '\0' /*L-Ctrl*/, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',
     '\0' /*L-Shift*/, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', '\0' /*R-Shift*/,
     '\0', '\0' /*L-Alt*/, ' ', '\0' /*CapsLock*/};

const uint8_t key_map_caps[64] =
    {'\0', '\0' /*esc*/, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\0' /*Backspace*/,
     '\0' /*tab*/, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\0' /*enter*/,
     '\0' /*L-Ctrl*/, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',
     '\0' /*L-Shift*/, '\\', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', '\0' /*R-Shift*/,
     '\0', '\0' /*L-Alt*/, ' ', '\0' /*CapsLock*/};

const uint8_t key_map_caps_shift[64] =
    {'\0', '\0' /*esc*/, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\0' /*Backspace*/,
     '\0' /*tab*/, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', '\0' /*enter*/,
     '\0' /*L-Ctrl*/, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '\"', '~',
     '\0' /*L-Shift*/, '|', 'z', 'x', 'c', 'v', 'b', 'n', 'm', '<', '>', '?', '\0' /*R-Shift*/,
     '\0', '\0' /*L-Alt*/, ' ', '\0' /*CapsLock*/};

uint8_t ctrl_state = 0;
uint8_t shift_state = 0;
uint8_t alt_state = 0;
uint8_t caps_state = 0;
uint8_t caps_dirty = 0;
volatile uint8_t enter_state[3] = {0, 0, 0};

/* GOAT: https://wiki.osdev.org/Formatted_Printing */

char terminal_buffers[3 * 128];
uint8_t buffer_indices[3];

char *terminal_buffer = terminal_buffers;
uint8_t buffer_index = 0;

uint8_t terminal_active[3] = {1, 0, 0};

/* terminal_open
 *   DESCRIPTION:
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:
 */
int32_t terminal_open(const uint8_t *filename) {
  return -1;
}

/* terminal_close
 *   DESCRIPTION:
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:
 */
int32_t terminal_close(int32_t fd) {
  return -1;
}

/* terminal_read
 *   DESCRIPTION:
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:
 */
int32_t terminal_read(int32_t fd, void *buf, int32_t nbytes) {
  if (fd != 0) return -1;
  if (nbytes != 0 && buf == NULL) {
    return -1;
  }
  if (nbytes < 0) {
    return -1;
  }

  int terminal_id = get_pcb_pointer(current_pid)->terminal_id;

  while (!enter_state[terminal_id]) {
    // Look Pretty
  }

  nbytes = (nbytes < buffer_index) ? nbytes : buffer_index;

  cli();
  memcpy(buf, terminal_buffer, nbytes);
  enter_state[terminal_id] = 0;
  sti();
  clear_terminal_buffer();

  return nbytes;
}
static void clear_terminal_buffer(void) {
  int i;
  for (i = 0; i < TERMINAL_BUFFER_SIZE; i++) {
    terminal_buffer[i] = '\0';
  }
  buffer_index = 0;
}

/* terminal_write
 *   DESCRIPTION:
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:
 */
int32_t terminal_write(int32_t fd, const void *buf, int32_t nbytes) {
  if (fd != 1) return -1;
  int32_t nbytes_written = 0;
  int32_t i;
  cli();
  for (i = 0; i < nbytes; i++) {
    if (*(uint8_t *)(buf + i) != '\0')  // BUGLOG had i outside of () before
    {
      tputc(*(uint8_t *)(buf + i), get_pcb_pointer(current_pid)->terminal_id);
      nbytes_written++;
    }
  }
  sti();
  update_cursor();
  return nbytes_written;
}

/* keyboard_init
 *   DESCRIPTION: Initializes the keyboard
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Initializes the keyboard
 */
void keyboard_init(void) {
  current_terminal = 0;
  enable_irq(KEYBOARD_IRQ_NUM);
}

/* keyboard__handler
 *   DESCRIPTION: Handles the keyboard interrupt
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Echos keyboard input to terminal
 */
void keyboard_handler(void) {
  cli();

  uint8_t scan_code = inb(KEYBOARD_DATA_PORT);
  if (key_map[scan_code] == '\0') {
    if (scan_code == ENTER_PRESSED_SCANCODE) {
      terminal_buffer[buffer_index] = '\n';
      tputc('\n', current_terminal);
      update_cursor();
      buffer_index++;
      enter_state[current_terminal] = 1;
      // keyboard_enter();
    } else if (scan_code == BACKSPACE_SCANCODE) {
      keyboard_backspace();
    } else if (scan_code == LEFT_CONTROL_SCANCODE) {
      ctrl_state = 1;
    } else if (scan_code == LEFT_CONTROL_RELEASED_SCANCODE) {
      ctrl_state = 0;
    } else if (scan_code == LEFT_SHIFT_SCANCODE || scan_code == RIGHT_SHIFT_SCANCODE) {
      shift_state = 1;
    } else if (scan_code == LEFT_SHIFT_RELEASED_SCANCODE || scan_code == RIGHT_SHIFT_RELEASED_SCANCODE) {
      shift_state = 0;
    } else if (scan_code == CAPS_LOCK_SCANCODE && caps_dirty == 0) {
      caps_state = !caps_state;
      caps_dirty = 1;
    } else if (scan_code == CAPS_LOCK_RELEASED_SCANCODE) {
      caps_dirty = 0;
    } else if (scan_code == TAB_PRESSED_SCANCODE) {
      tputs("    ", current_terminal);
      update_cursor();
    } else if (scan_code == LEFT_ALT_PRESSED_SCANCODE) {
      alt_state = 1;
    } else if (scan_code == LEFT_ALT_RELEASED_SCANCODE) {
      alt_state = 0;
    } else if ((alt_state == 1) && (scan_code >= F1_PRESSED_SCANCODE) && (scan_code <= F3_PRESSED_SCANCODE)) {
      send_eoi(KEYBOARD_IRQ_NUM);
      switch_terminal(scan_code - F1_PRESSED_SCANCODE);
      return;
    }

    send_eoi(KEYBOARD_IRQ_NUM);
    sti();
    return;
  } else /*Printable Key*/
  {
    // control
    if (ctrl_state && scan_code == L_SCANCODE && scan_code <= LAST_PRESSED_SCAN_CODE) {
      clear();
      update_cursor();
    }

    if (buffer_index < TERMINAL_BUFFER_SIZE - 1) /* The 128th (if reached) character has to be an enter input */
    {
      // regular
      if (!ctrl_state && !shift_state && !caps_state && scan_code <= LAST_PRESSED_SCAN_CODE) {
        uint8_t c = key_map[scan_code];
        tputc(c, current_terminal);
        update_cursor();
        terminal_buffer[buffer_index] = c;
        buffer_index++;
      }
      // shift
      if (!ctrl_state && shift_state && !caps_state && scan_code <= LAST_PRESSED_SCAN_CODE) {
        uint8_t c = key_map_shift[scan_code];
        tputc(c, current_terminal);
        update_cursor();
        terminal_buffer[buffer_index] = c;
        buffer_index++;
      }
      // caps
      if (!ctrl_state && !shift_state && caps_state && scan_code <= LAST_PRESSED_SCAN_CODE) {
        uint8_t c = key_map_caps[scan_code];
        tputc(c, current_terminal);
        update_cursor();
        terminal_buffer[buffer_index] = c;
        buffer_index++;
      }
      // caps and shift
      if (!ctrl_state && shift_state && caps_state && scan_code <= LAST_PRESSED_SCAN_CODE) {
        uint8_t c = key_map_caps_shift[scan_code];
        tputc(c, current_terminal);
        update_cursor();
        terminal_buffer[buffer_index] = c;
        buffer_index++;
      }
    }
  }

  send_eoi(KEYBOARD_IRQ_NUM);
  sti();
}

/* keyboard_backspace
 *   DESCRIPTION: Removes last value in buffer
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:
 */
static void keyboard_backspace(void) {
  if (buffer_index == 0 || terminal_buffer == NULL) {
    return;
  }

  buffer_index--;
  terminal_buffer[buffer_index] = '\0';

  terminal_backspace();
  update_cursor();
}
/* switch_terminal
 *   DESCRIPTION: switches the active terminal to the terminal with the given terminal id 
 *                if the terminal is not active, it starts a new shell process on it
 *   INPUTS: term_id -- the id of the terminal to switch to
 *   OUTPUTS: none
 *   RETURN VALUE: always return 0
 *   SIDE EFFECTS:
 */
int switch_terminal(int term_id) {
  if (term_id == current_terminal) { //if the terminal is the current terminal do nothing
    return 0;
  }
  if ((get_free_pid() == -1) && (terminal_active[term_id] == 0)) { //if there are no free pids and the terminal is not active do nothing
    return 0;
  }

  save_pos(); //save current cursor position
  // update the page table to map the video memory to the new terminal
  page_table[VIDEO_MEM_INDEX + 1 + current_terminal].address_low = VIDEO_MEM_INDEX + 1 + current_terminal;
  page_table[VIDEO_MEM_INDEX + 1 + term_id].address_low = VIDEO_MEM_INDEX;
  page_table[VIDEO_MEM_INDEX + 4].address_low = VIDEO_MEM_INDEX + 1 + term_id;
  flush_tlb();
  //copy the video memory to the new terminal
  memcpy((void *)(4096 * (VIDEO_MEM_INDEX + 1 + current_terminal)), (void *)(4096 * (VIDEO_MEM_INDEX)), 4096);
 //copy the video memory from the new terminal to the video memory
  memcpy((void *)(4096 * (VIDEO_MEM_INDEX)), (void *)(4096 * (VIDEO_MEM_INDEX + 4)), 4096);
//save current buffer index and update the buffer index to the new terminal buffer index
  buffer_indices[(int)current_terminal] = buffer_index;
  buffer_index = buffer_indices[term_id];

  terminal_buffer = terminal_buffers + (term_id * 128); //update temrminal buffer 

  current_terminal = term_id;//update current terminal id
  restore_pos(); //restore cursor position
  if (terminal_active[term_id] == 0) { //if the terminal is not active start a new shell process on it
    terminal_active[term_id] = 1; //mark terminal as active
    clear();
    process_start(process_create((uint8_t *)"shell", term_id, -1));
  } else {
    // process_switch(terminal_pids[term_id]);
  }
  sti();
  return 0;
}
