#include "../fs.h"
#include "../lib.h"
#include "../paging.h"
#include "../x86_desc.h"
#include "keyboard.h"
#include "rtc.h"
#include "syscall.h"

#define BLOCK_START 0x08000000

/* flush_tlb
 *   DESCRIPTION: Flushes the TLB by reloading the CR3 register,
 *                which is necessary when page tables have been modified.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: TLB is cleared, forcing page table entries to be reloaded on next memory access.
 */
void flush_tlb() {
  asm volatile(
      "\
  movl    %%cr3,%%eax \n \
  movl    %%eax,%%cr3 \
  " : : : "eax");  // flush TLB
}
/* processes_init
 *   DESCRIPTION: Initializes PID to -1(inactive) for all processes
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:
 */
void processes_init() {
  int i;
  for (i = 0; i < 6; i++) {
    active_pids[i] = -1;  // initialize all PIDs to -1
  }
  for (i = 0; i < 3; i++) {
    terminal_pids[i] = -1;  // initialize all terminal PIDs to -1
  }
  current_pid = -1;  // set current PID to -1 (no active processes)
}
/* get_free_pid
 *   DESCRIPTION: Finds an unused PID for a new process.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: Returns the index of the first free PID slot
 *   SIDE EFFECTS:
 */
int get_free_pid() {
  int i;
  for (i = 0; i < 6; i++) {
    if (active_pids[i] == -1) {  // check available PID
      return i;                  // return the index of the pid
    }
  }
  return -1;  // no available PID return -1
}
/* get_pcb_pointer
 *   DESCRIPTION: Calculates the pointer to the PCB for a given PID.
 *   INPUTS:  PID for which to get the PCB pointer
 *   OUTPUTS: none
 *   RETURN VALUE: Pointer to the PCB of the given PID
 *   SIDE EFFECTS:
 */
pcb_t* get_pcb_pointer(int pid) {
  return (pcb_t*)(0x800000 - (0x2000 * (1 + pid)));  // calculate the pointer to the PCB
}
/* get_kernel_start_esp
 *   DESCRIPTION: Retrieves the starting kernel stack pointer for a given PID
 *   INPUTS: PID for which to retrieve the stack pointer
 *   OUTPUTS: none
 *   RETURN VALUE: The starting kernel stack pointer address
 *   SIDE EFFECTS:
 */
uint32_t get_kernel_start_esp(int pid) {
  return (uint32_t)(0x800000 - (0x2000 * (pid)) - 0x4);  // calculate the starting kernel stack pointer
}
/* read_handler
 *   DESCRIPTION: handles read operation for a given file descriptor
 *   INPUTS:  - fd: File descriptor of the file to read from
 *            - buf: Buffer where the data read from the file should be stored
 *            - nbytes: Number of bytes to read
 *   OUTPUTS: none
 *   RETURN VALUE: return the number of bytes successfully read,  if file descriptor is invalid, return -1
 *   SIDE EFFECTS:
 */
int32_t read_handler(int32_t fd, void* buf, int32_t nbytes) {
  if (check_valid_fd(fd) == -1) return -1;                            // check if file descriptor is valid
  return file_descriptors[fd].file_table_ptr->read(fd, buf, nbytes);  // call the read function of the file table
}
/* write_handler
 *   DESCRIPTION: handles write operation for a given file descriptor
 *   INPUTS:  - fd: File descriptor of the file to read from
 *            - buf: Buffer where the data read from the file should be stored
 *            - nbytes: Number of bytes to read
 *   OUTPUTS: none
 *   RETURN VALUE: the data is written to the target device or file, if file descriptor is invalid, return -1
 *   SIDE EFFECTS:
 */
int32_t write_handler(int32_t fd, const void* buf, int32_t nbytes) {
  if (check_valid_fd(fd) == -1) return -1;                             // check if file descriptor is valid
  return file_descriptors[fd].file_table_ptr->write(fd, buf, nbytes);  // call the write function of the file table
}
/* open_handler
 *   DESCRIPTION: Opens a file, directory, or device based on the filename and initializes any necessary data structures or hardware
 *   INPUTS: - filename: The name of the file, directory, or device to open
 *   OUTPUTS: none
 *   RETURN VALUE: Returns a descriptor on success; -1 on failure
 *   SIDE EFFECTS:
 */
int32_t open_handler(const uint8_t* filename) {
  if (strncmp("rtc", (int8_t*)filename, 3) == 0) {  // check if the file is rtc
    // Case trying to open RTC
    return rtc_open(filename);
  } else if (strncmp(".", (int8_t*)filename, 1) == 0) {  // check if the file is directory
    // Case trying to open directory
    return directory_open(filename);
  } else {
    // Case trying to open file
    return file_open(filename);
  }
  return -1;
}
/* close_handler
 *   DESCRIPTION:Closes a file descriptor
 *   INPUTS: - fd: File descriptor of the file to close
 *   OUTPUTS: none
 *   RETURN VALUE: The close function should return 0 on successful completion, and -1 on failure
 *   SIDE EFFECTS:
 */
int32_t close_handler(int32_t fd) {
  if (check_valid_fd(fd) == -1) return -1;                // check if file descriptor is valid
  return file_descriptors[fd].file_table_ptr->close(fd);  // correct way to do this
}
/* set_paging_for_process
 *   DESCRIPTION:Sets up paging for a specific process. The first process starts at 8MB, and each subsequent process is allocated an additional 4MB.
 *   INPUTS: the pid for which to set up paging
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:Modifies the page directory to set up paging for the process
 */
void set_paging_for_process(int pid) {
  page_directory[32].pd_4mb.address_31_22 = 2 + pid;  // first process starts at 8mb, plus 4 mb per
  flush_tlb();
}
/* process_create
 *   DESCRIPTION:Creates a new process for a given command.
 *   INPUTS: command - the command to execute for the new process
 *           terminal - the terminal associated with the process
 *           parent_pid - the pid of the parent process
 *   OUTPUTS: none
 *   RETURN VALUE: returns the pid of the new process on success, -1 on failure
 *   SIDE EFFECTS:modifies the pcb and the active_pids array to reflect the new process
 */
int32_t process_create(const uint8_t* command, int terminal, int parent_pid) {
  // check if file exists
  dentry_t dentry;
  uint8_t file_start_bytes[4];  // first 4 bytes of the file

  uint8_t file_name[32];   // file name with max length 32
  uint8_t file_args[128];  // file arguments with max length 128
  int command_pos = 0;     // initialize command position to 0
  for (command_pos = 0; command_pos < 32; command_pos++) {
    if (command[command_pos] == '\0') {  // check if the command is null
      file_name[command_pos] = '\0';     // if command is null set the file name to null
      file_args[0] = '\0';               // set the file arguments to null
      break;
    }

    if (command[command_pos] == ' ') {                                       // check if the command is space
      file_name[command_pos] = '\0';                                         // set the file name to null
      strncpy((int8_t*)file_args, (int8_t*)command + command_pos + 1, 128);  // copy the arguments to file_args
      break;
    }

    file_name[command_pos] = command[command_pos];  // copy the command to file name
  }

  if (read_dentry_by_name(file_name, &dentry) != -1) {  // check if the file exists
    if (dentry.file_type != 2) {
      return -1;  // non-file
    }

    if (read_data(dentry.inode, 0, file_start_bytes, 4) != 4) {
      return -1;  // error reading file
    }
    if ((strncmp((int8_t*)file_start_bytes + 1, (int8_t*)"ELF", 3) == 0) && (file_start_bytes[0] == 0x7F)) {
      // Case executable file
      int pid;
      sti();
      if ((pid = get_free_pid()) == -1) {
        printf("Too many processes\n");
        return -1;
      }
      active_pids[pid] = 0;
      cli();

      set_paging_for_process(pid);
      uint32_t file_pos = 0;
      uint8_t* memory_pos = (uint8_t*)(BLOCK_START + 0x00048000);         // file start offset
      while (read_data(dentry.inode, file_pos, memory_pos, 4096) != 0) {  // copy file to allocated space
        file_pos += 4096;
        memory_pos += 4096;
      }

      pcb_t* pcb = get_pcb_pointer(pid);
      int i;
      for (i = 0; i < 8; i++) {
        pcb->file_descriptors[i].inode = -1;
      }
      pcb->file_descriptors[0].file_table_ptr = &terminal_jumptable;  // STDIN
      pcb->file_descriptors[0].inode = -2;                            // -1 is default, -2 is valid but fake
      pcb->file_descriptors[1].file_table_ptr = &terminal_jumptable;  // STDOUT
      pcb->file_descriptors[1].inode = -2;                            // see above

      pcb->pid = pid;// Set the pid in the PCB

      pcb->terminal_id = terminal; // Set the terminal id in the PCB

      pcb->kernel_esp = get_kernel_start_esp(pid);  // highest position in kernel block
      if (parent_pid != -1) {
        pcb->parent_pcb = get_pcb_pointer(current_pid);
      } else {
        pcb->parent_pcb = NULL;
      }

      strncpy((int8_t*)pcb->args, (int8_t*)file_args, 128);// Copy the file arguments to the PCB

      return pid; 

    } else {
      return -1;  // non-executable file
    }
  }
  return -1;
}
/* process_start
 *   DESCRIPTION:Starts a process with a given pid. if pid is valid and active, then sets up paging for the process
 *                and saves the current ebp and esp if a process is running. 
 *                  then sets the esp for the kernel, updates the current pid,
 *   INPUTS: pid -- the process ID of the process to start
 *   OUTPUTS: none
 *   RETURN VALUE: returns 0 if the process is successfully started, -1 otherwise.
 *   SIDE EFFECTS:modifies the active_pids array, the page directory, the current_pid, the file_descriptors, the terminal_pids, and the tss.esp0. 
 *                Also, it changes the state of the processor by enabling interrupts and using the iret instruction
 */
int32_t process_start(uint8_t pid) {
  if (pid > 5) return -1;                // check if the pid is valid
  if (active_pids[pid] != 0) return -1;  // check if the pid is active
  active_pids[pid] = 1;                  // set the pid to active
  pcb_t* pcb = get_pcb_pointer(pid);

  // set DS REGISTER TO USER_DS (mov2)
  // push SS (USER_DS)
  // push ESP (block start + 4mb - 4)
  // push EFLAGS (take current eflags???, or with 0x0200)
  // push CS (USER_CS)
  // push EIP (entry_point)
  set_paging_for_process(pid);

  uint8_t* new_esp = (uint8_t*)(BLOCK_START + (0x400000 - 0x4));
  uint32_t* entry_point = *(uint32_t**)((BLOCK_START + 0x00048000 + 24));
  // USER_CS MIGHT have to be changed to set the correct bits

  // Save current ebp and esp if running something exists
  if (current_pid != -1) {
    pcb_t* current_pcb = get_pcb_pointer(current_pid);
    asm volatile(
        "        \
        movl %%esp, %0    \n\
        movl %%ebp, %1      \
        " : "=r"(current_pcb->kernel_esp), "=r"(current_pcb->kernel_ebp));
  }
  current_pid = pid;  // update to current task
  file_descriptors = pcb->file_descriptors;
  terminal_pids[pcb->terminal_id] = pid;
  tss.esp0 = get_kernel_start_esp(pid);  // set the esp for kernel
  sti();
  asm volatile(
      "\
        xorl %%eax, %%eax      \n\
        movl %2, %%eax         \n\
        movw  %%ax, %%ds       \n\
        pushl %2               \n\
        pushl %0               \n\
        pushfl                 \n\
        popl %%eax             \n\
        orl  $0x0200, %%eax    \n\
        push %%eax             \n\
        pushl %3               \n\
        pushl %1               \n\
        iret                  "
      :
      : "r"(new_esp), "r"(entry_point), "r"(USER_DS), "r"(USER_CS)
      : "eax");
  return 0;
}
/* process_switch
 *   DESCRIPTION:switches the currently running process to the process with the given pid.
                  it saves the current ebp and esp if a process is running, updates the current pid,
                   sets the esp for the kernel, and sets up paging for the new process.
 *   INPUTS: pid -- the pid of the process to switch to
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:
 */
void process_switch(uint8_t pid) {
  // Save current ebp and esp if running something exists
  if (current_pid != -1) {
    pcb_t* current_pcb = get_pcb_pointer(current_pid);// Get a pointer to the pcb of the current process
 // save the current esp and ebp  
    asm volatile(
        "                   \
        movl %%esp, %0    \n\
        movl %%ebp, %1      \
        " : "=r"(current_pcb->kernel_esp), "=r"(current_pcb->kernel_ebp));
  }

  pcb_t* pcb = get_pcb_pointer(pid);// get a pointer to the PCB of the process to switch to

  // tss.esp0 = pcb->kernel_esp;  // set the esp for kernel
  tss.esp0 = get_kernel_start_esp(pid);
  current_pid = pid;  // update to current task
  file_descriptors = pcb->file_descriptors;
  set_paging_for_process(pid);// set up paging for the new process
  //set the esp and ebp for the new process
  asm volatile(
      "                     \
      movl %0, %%esp      \n\
      movl %1, %%ebp        \
      "
      :
      : "r"(pcb->kernel_esp), "r"(pcb->kernel_ebp)
      : "memory");
  sti();
  return;
}

/* execute_handler
 *   DESCRIPTION: Executes a command by creating a new process and loading the executable file into memory.
 *                It sets up the PCB and switches to user mode to start executing the new process
 *   INPUTS: The command is a space-separated sequence of words
 *   OUTPUTS: none
 *   RETURN VALUE: Returns 0 on successful execution, -1 if the file does not exist, is not an executable,
 *                 or if there is an error during execution
 *   SIDE EFFECTS:
 */
int32_t execute_handler(const uint8_t* command) {
  return process_start(process_create(command, get_pcb_pointer(current_pid)->terminal_id, current_pid));
}
/* halt_handler
 *   DESCRIPTION:  terminates a process
 *   INPUTS: the exit status to return to the parent process
 *   OUTPUTS: none
 *   RETURN VALUE: returning the specified value to its parent process
 *   SIDE EFFECTS:
 */
int32_t halt_handler(uint32_t status) {
  int i;
  cli();

  // close all op
  for (i = 2; i < 8; i++) {
    if (file_descriptors[i].inode == -1) {
      continue;
    }
    file_descriptors[i].file_table_ptr->close(i);
  }

  int pid = current_pid;
  pcb_t* pcb = get_pcb_pointer(pid);

  // if parent is null, reopen shell
  if (pcb->parent_pcb == NULL) {
    current_pid = -1;
    active_pids[pid] = -1;
    process_start(process_create((uint8_t*)"shell", pcb->terminal_id, -1));
    return 0;
  }

  // return to parent
  tss.esp0 = get_kernel_start_esp(pcb->parent_pcb->pid);
  file_descriptors = pcb->parent_pcb->file_descriptors;
  current_pid = pcb->parent_pcb->pid;
  terminal_pids[pcb->parent_pcb->terminal_id] = current_pid;
  active_pids[pid] = -1;

  set_paging_for_process(current_pid);
  asm volatile(
      "               \
  movl %0, %%esp    \n\
  movl %1, %%ebp    \n\
  movl %2, %%eax    \n\
  leave             \n\
  ret                 \
  "
      :
      : "r"(pcb->parent_pcb->kernel_esp), "r"(pcb->parent_pcb->kernel_ebp), "r"((uint32_t)status));

  return 0;
}
/* sigreturn_handler
 *   DESCRIPTION: handles the sigreturn system call(checkpoint 3.5)
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: -1
 *   SIDE EFFECTS:
 */
int32_t sigreturn_handler() {
  printf("sigreturn syscall occurred\n");
  return -1;
}
/* vidmap_handler
 *   DESCRIPTION: Maps the text-mode video memory into user space at a pre-set virtual address
 *   INPUTS: a double pointer to the start of the video memory
 *   OUTPUTS: none
 *   RETURN VALUE: Returns 0 on successful mapping, -1 if the input pointer is NULL or not within the valid range
 *   SIDE EFFECTS: Changes the memory mapping of the process
 */
int32_t vidmap_handler(uint8_t** screen_start) {
  if (screen_start == NULL || (uint32_t)screen_start < 0x08000000 || (uint32_t)screen_start > 0x080FFFFF) {  // check if the input pointer is NULL or not within the valid range
    return -1;
  }
  *screen_start = (uint8_t*)((4096 * (VIDEO_MEM_INDEX + 1 + get_pcb_pointer(current_pid)->terminal_id)));  // set the screen start to the video memory
  return 0;
}
/* getargs_handler
 *   DESCRIPTION:  reads the program’s command line arguments into a user-level buffer
 *   INPUTS: buf -- a pointer to the buffer where the arguments will be copied
 *           nbytes -- the maximum number of bytes to copy
 *   OUTPUTS: none
 *   RETURN VALUE: If there are no arguments, or if the arguments and a terminal NULL (0-byte) do not fit in the buffer, simply return -1;
 *                  otherwise succeed return 0
 *   SIDE EFFECTS:
 */
int32_t getargs_handler(uint8_t* buf, int32_t nbytes) {
  pcb_t* pcb = get_pcb_pointer(current_pid);                  // get the pointer to the PCB
  if (nbytes <= strlen((int8_t*)pcb->args) || buf == NULL) {  // check if the arguments is valid or not
    return -1;
  }
  strncpy((int8_t*)buf, (int8_t*)pcb->args, nbytes);  // if is valid copy the arguments to the buffer
  return 0;
}
/* set_handler_handler
 *   DESCRIPTION: handle the set_handler system call(checkpoint 3.5)
 *   INPUTS:
 *   OUTPUTS: none
 *   RETURN VALUE: return -1
 *   SIDE EFFECTS:
 */
int32_t set_handler_handler() {
  printf("set_handler syscall occurred\n");
  return -1;
}
