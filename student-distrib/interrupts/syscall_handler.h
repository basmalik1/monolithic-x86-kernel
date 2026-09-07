#include "../fs.h"

void processes_init();
int32_t process_create(const uint8_t* command, int terminal, int parent_pid);
int32_t process_start(uint8_t pid);
void process_switch(uint8_t pid);

int32_t read_handler(int32_t fd, void* buf, int32_t nbytes);
int32_t write_handler(int32_t fd, const void* buf, int32_t nbytes);
int32_t open_handler(const uint8_t* filename);
int32_t close_handler(int32_t fd);
int32_t execute_handler(const uint8_t* file);
int32_t halt_handler(uint32_t status);
int32_t sigreturn_handler();
int32_t vidmap_handler();
int32_t getargs_handler(uint8_t* buf, int32_t nbytes);
int32_t set_handler_handler();

void flush_tlb();
int get_free_pid();

typedef struct pcb_t {
  fd_t file_descriptors[8];
  uint32_t kernel_esp;
  uint32_t kernel_ebp;
  int32_t pid;
  struct pcb_t* parent_pcb;
  uint8_t terminal_id;
  uint8_t scheduled;
  uint8_t args[128];
} pcb_t;

// -1 - not created
// 0 - created, not active
// 1 - active
volatile int8_t active_pids[6];
volatile int8_t current_pid;

volatile int current_terminal;
volatile int8_t terminal_pids[3];

pcb_t* get_pcb_pointer(int pid);
