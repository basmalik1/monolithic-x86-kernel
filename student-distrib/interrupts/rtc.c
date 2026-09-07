#include "rtc.h"

#include "../fs.h"
#include "../i8259.h"
#include "../lib.h"
#include "../tests.h"
#include "syscall_handler.h"

volatile int rtc_interrupt_flag = 0;
int rtc_count = 0;
int current_index = 0;
/* rtc_init
 *   DESCRIPTION: Initializes the RTC
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Initializes the RTC
 */
void rtc_init() {
  outb(RTC_B_REG | RTC_INT_MASK, RTC_PORT);  // select register B, and disable NMI
  char prev = inb(RTC_PORTD);                // read the current value of register B
  outb(RTC_B_REG | RTC_INT_MASK, RTC_PORT);  // set the index again (a read will reset the index to register D)
  outb(prev | 0x40, RTC_PORTD);              // write the previous value ORed with 0x40. This turns on bit 6 of register B

  // set freq to 1024 Hz
  char rate = 0x06;                          // 6 is the rate to set 1024 Hz
  outb(RTC_A_REG | RTC_INT_MASK, RTC_PORT);  // set index to register A, disable NMI
  prev = inb(RTC_PORTD);                     // get initial value of register A
  outb(RTC_A_REG | RTC_INT_MASK, RTC_PORT);  // reset index to A
  outb((prev & 0xF0) | rate, RTC_PORTD);     // write only our rate to A. Note, rate is the bottom 4 bits.

  // read from C register to reset the IRQ line
  outb(RTC_C_REG, RTC_PORT);  // select register C
  inb(RTC_PORTD);             // just throw away contents

  enable_irq(RTC_IRQ);  // enable RTC IRQ line
}

/* rtc_handler
 *   DESCRIPTION: Handles the RTC interrupt
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void rtc_handler() {
  rtc_interrupt_flag = 1;
  // read from C register to reset the IRQ line
  outb(RTC_C_REG, RTC_PORT);  // select register C
  inb(RTC_PORTD);             // just throw away contents

  send_eoi(RTC_IRQ);  // send EOI to the PIC
  int i, j;
  sti();
  for (j = 0; j < 3; j++) {  // update all rtc counters, even if they are not active
    if (terminal_pids[j] != -1) {
      for (i = 2; i < 8; i++) {
        fd_t* fds = (get_pcb_pointer(terminal_pids[j])->file_descriptors);
        if (fds[i].file_table_ptr == &rtc_jumptable) {
          if (fds[i].file_pos > 0) {
            fds[i].file_pos -= 1;
          }
        }
      }
    }
  }
  // if (file_descriptors != NULL) {
  //   for (i = 2; i < 8; i++) {
  //     if (file_descriptors[i].file_table_ptr == &rtc_jumptable) {
  //       if (file_descriptors[i].file_pos > 0) {
  //         file_descriptors[i].file_pos -= 1;
  //       }
  //     }
  //   }
  // }
  if (rtc_count > 0) {
    rtc_count--;
  }
  if (rtc_count == 0) {
    rtc_count = 8;
    for (i = 0; i < 3; i++) {
      current_index = (current_index + 1) % 3;
      if (terminal_pids[current_index] != -1 && terminal_pids[current_index] != current_pid) {
        process_switch(terminal_pids[current_index]);
        cli();
        break;
      }
    }
  } else {
    cli();
  }
}
/* rtc_set_freq
 *   DESCRIPTION: onvert different frequency into rate and write new rate to the A
 *   INPUTS: frequency value
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void rtc_set_freq(uint32_t f) {
  if ((f & (f - 1)) != 0 || f < 2 || f > 1024) return;  // check if frequency is power of 2 and inside the range
  int rate = 16 - log2_for_frequency(f);                // calculate rate value
  cli();                                                // Disable interrupts
  outb(RTC_A_REG | RTC_INT_MASK, RTC_PORT);             // Disable NMI and select register A
  char prev2 = inb(RTC_PORTD);                          // Read current value of register A
  outb(RTC_A_REG | RTC_INT_MASK, RTC_PORT);             // Reset index to A
  outb((prev2 & 0xF0) | rate, RTC_PORTD);               // Set new rate in lower 4 bits
  sti();                                                // Enable interrupts
}
/* rtc_open
 *   DESCRIPTION: open the rtc file with freqnecy value 2hz
 *   INPUTS: a pointer to filename
 *   OUTPUTS: fd
 *   RETURN VALUE: any errors occured return -1 otherwise return fd.
 *   SIDE EFFECTS: none
 */
int32_t rtc_open(const uint8_t* filename) {
  int fd;
  // if (file_descriptors[fd].flags) return -1;
  if ((fd = get_free_fd()) == -1) {
    return -1;
  }
  file_descriptors[fd].file_table_ptr = &rtc_jumptable;
  file_descriptors[fd].inode = 0;       // RTC is not a regular data file inode has to = 0
  file_descriptors[fd].file_pos = 512;  // file_pos will be used as virtual frequency counter
  file_descriptors[fd].flags = 512;     // default frequency is 2Hz
  return fd;
}

/* rtc_read
 *   DESCRIPTION: blocks until and RTC interrupt occurs
 *   INPUTS: fd - file descriptor
 *           buf - a pointer to the buffer
 *           nbytes - number of bytes to read
 *   OUTPUTS: none
 *   RETURN VALUE: return 0 on success
 *   SIDE EFFECTS: none
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes) {
  file_descriptors[fd].file_pos = file_descriptors[fd].flags;  // reset file_pos to the number of cycles
  while (file_descriptors[fd].file_pos != 0) {                 // block until the file_pos is 0
    // block
  }
  return 0;
}

/* rtc_write
 *   DESCRIPTION:sets the frequency of RTC interrupts
 *   INPUTS: fd - file descriptor
 *           buf - a pointer to uint32_t value, stored the frequency
 *           nbytes - number of bytes to read (this should always be 4)
 *   OUTPUTS: none
 *   RETURN VALUE: return -1 for failure and return number of bytes for success
 *   SIDE EFFECTS: none
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes) {
  if (nbytes != 4 || buf == NULL) return -1;  // check for valide inputs value
  uint32_t freq = *(uint32_t*)buf;
  if (
      (freq & (freq - 1)) != 0 ||
      freq < 2 ||
      freq > 1024) {
    return -1;
  }  // Return -1 on failure
  // rtc_set_freq(freq);
  file_descriptors[fd].file_pos = 1024 / freq;  // set to # of cycles (out of 1024)
  file_descriptors[fd].flags = 1024 / freq;     // set to # of cycles (out of 1024)
  return nbytes;                                // return the number of bytes written
}

/* rtc_disable
 *   DESCRIPTION: closes the RTC driver ensuring no more RTC interrupts are received.
 *   INPUTS:
 *   OUTPUTS: none
 *   RETURN VALUE: return 0
 *   SIDE EFFECTS: none
 */
int32_t rtc_disable() {
  disable_irq(RTC_IRQ);  // diable the rtc interrupt line to stop receving interrupts
  return 0;              // return 0 for success
}

/* rtc_close
 *   DESCRIPTION: closes the virtualized rtc
 *   INPUTS: fd - file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: return 0 for success and -1 for failure
 *   SIDE EFFECTS: closes the rtc
 */
int32_t rtc_close(int32_t fd) {
  //   printf("closing fd: %d", fd);
  if (check_valid_fd(fd) == -1) return -1;     // invalid fd
  file_descriptors[fd].file_table_ptr = NULL;  // reset fd
  file_descriptors[fd].file_pos = 0;
  file_descriptors[fd].flags = 0;
  file_descriptors[fd].inode = -1;
  return 0;
}

/* log2_for_frequency
 *   DESCRIPTION: calculate log2 of frequency value
 *   INPUTS: frequency value
 *   OUTPUTS: none
 *   RETURN VALUE: log2 of frequency value
 *   SIDE EFFECTS: none
 */
int log2_for_frequency(uint32_t number) {
  if (number <= 0 || number % 2 != 0) return -1;  // input should be an even and positive number
  int result = 0;                                 // initialize result
  while (number > 1) {                            // loop to calculate the log2 of input value
    number /= 2;
    result++;
  }
  return result;  // return log 2 of input value
}
