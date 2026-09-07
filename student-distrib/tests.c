#include "tests.h"

#include "fs.h"
#include "interrupts/keyboard.h"
#include "interrupts/rtc.h"
#include "interrupts/syscall_handler.h"
#include "lib.h"
#include "x86_desc.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER \
  printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result) \
  printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure() {
  /* Use exception #15 for assertions, otherwise
     reserved by Intel */
  asm volatile("int $15");
}

/* Checkpoint 1 tests */

/* IDT Test - Example
 *
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test() {
  TEST_HEADER;

  int i;
  int result = PASS;
  for (i = 0; i < 10; ++i) {
    if (i == 1) continue;  // skip intel reserved entry
    if ((idt[i].offset_15_00 == NULL) &&
        (idt[i].offset_31_16 == NULL)) {
      assertion_failure();
      result = FAIL;
    }
  }

  return result;
}

// add more tests here

// Triggers division by zero exception
// Will halt machine after printing "div0 exception"
int div0_test() {
  TEST_HEADER;
  int a = 1;
  int b = 0;
  return a / b;
}
// Triggers page fault exception
// Will halt machine after printing "page_fault exception"
int page_fault_test() {
  TEST_HEADER;
  int* address = (int*)1;
  int a = *address;
  a++;
  return PASS;
}

// Dereferences memory that should be accessable after paging
// returns PASS if value was dereferenced
int page_dereference_succeed_test() {
  TEST_HEADER;
  int* succeed_address = (int*)0xB8000;  // video memory start
  int a = *succeed_address;
  a++;
  return PASS;
}
// Dereferences memory that shouldn't be accessable after paging
// Will halt machine after printing "page_fault exception"
int page_dereference_fail_test() {
  TEST_HEADER;
  int* failaddress = (int*)1;
  int a = *failaddress;
  a++;
  return PASS;
}

// called from rtc.c : rtc_handler
// increments all values in video memory, flashing the screen with charachters.
int test_rtc() {
  TEST_HEADER;
  test_interrupts();
  return PASS;
}

/* Checkpoint 2 tests */
// test rtc write with incorrect frequency
int test_rtc_freq_fail() {
  putc('\n');
  TEST_HEADER;
  uint32_t f = 5;
  if (rtc_write(0, &f, 4) == -1) return PASS;  // not power of 2 should fail
  return FAIL;
}
// test rtc write with correct frequency
int test_rtc_freq_success() {
  putc('\n');
  TEST_HEADER;
  uint32_t f = 2;
  if (rtc_write(0, &f, 4) == -1) return FAIL;
  return PASS;  // this is the power of 2 should pass
}
// test rtc write with incorrect byte size
int test_rtc_byte() {
  putc('\n');
  TEST_HEADER;
  uint32_t f = 2;
  if (rtc_write(0, &f, 3) == -1) return PASS;  // rtc_write should fail because byte size is incorrect
  return FAIL;
}
// test rtc open
int test_rtc_open() {
  putc('\n');
  TEST_HEADER;
  int r = rtc_open(NULL);
  if (r) return FAIL;
  return PASS;
}
// test if rtc can be closed after opened rtc driver
int test_rtc_close() {
  putc('\n');
  TEST_HEADER;
  int r = rtc_close(1);
  if (r) return FAIL;  // when rtc is not open it should fail
  return PASS;         // after rtc is opened it should pass
}
// test rtc interrupt and change printing rate when receive an interrupt
int rtc_frequency_test() {
  TEST_HEADER;
  int result = PASS;
  uint32_t freq;
  int i;
  int rate;
  rtc_open(NULL);

  // Test all possible frequencies
  for (freq = 2, rate = 0; freq <= 1024; freq *= 2, rate++) {
    // Set frequency
    rtc_write(0, &freq, sizeof(freq));
    for (i = 0; i < (rate + 5) * 2; i++) {
      rtc_read(0, NULL, 0);
      printf("1");
    }
    printf("\n");
  }

  // Close RTC
  rtc_close(1);

  return result;
}

// Tests various fs functionality (write)
// should return -1
int test_basic_file_read() {
  TEST_HEADER;
  int i;
  int32_t fd = file_open((uint8_t*)"frame0.txt");
  if (fd == -1) {
    return FAIL;
  }
  char buf[4096];
  int32_t bytes_read = file_read(fd, buf, 4096);
  if (bytes_read == -1) {
    return FAIL;
  }
  for (i = 0; i < bytes_read; i++) {
    putc(buf[i]);
  }

  file_close(fd);
  putc('\n');
  return PASS;
}

int test_fs_strange_buffer() {
  TEST_HEADER;
  int i;
  int32_t fd = file_open((uint8_t*)"frame0.txt");
  if (fd == -1) {
    return FAIL;
  }
  char buf[26];
  int32_t bytes_read;
  while ((bytes_read = file_read(fd, buf, 26))) {
    if (bytes_read == -1) {
      return FAIL;
    }
    for (i = 0; i < bytes_read; i++) {
      putc(buf[i]);
    }
    // printf("\nBytes read: %d\n", bytes_read);
  }

  file_close(fd);
  putc('\n');
  return PASS;
}

int test_executable_read() {
  TEST_HEADER;
  int i;

  int32_t fd = file_open((uint8_t*)"testprint");
  if (fd == -1) {
    return FAIL;
  }
  char buf[4096];
  int32_t bytes_read;
  while ((bytes_read = file_read(fd, buf, 4096))) {
    if (bytes_read == -1) {
      return FAIL;
    }
    for (i = 0; i < bytes_read; i++) {
      if (buf[i] == '\0') continue;  // skip null characters
      putc(buf[i]);
    }
  }

  file_close(fd);
  putc('\n');

  return PASS;
}

int test_long_file_read() {
  TEST_HEADER;
  int i;

  int32_t fd = file_open((uint8_t*)"verylargetextwithverylongname.tx");
  if (fd == -1) {
    return FAIL;
  }
  char buf[4096];
  int32_t bytes_read;
  while ((bytes_read = file_read(fd, buf, 4096))) {
    if (bytes_read == -1) {
      return FAIL;
    }
    for (i = 0; i < bytes_read; i++) {
      putc(buf[i]);
    }
  }

  file_close(fd);
  putc('\n');

  return PASS;
}

int test_fs_read_dir() {
  TEST_HEADER;
  int i;
  int32_t fd = directory_open((uint8_t*)".");
  if (fd == -1) {
    return FAIL;
  }
  char buf[32];
  int32_t bytes_read;
  while ((bytes_read = directory_read(fd, buf, 32))) {
    if (bytes_read == -1) {
      return FAIL;
    }
    for (i = 0; i < bytes_read; i++) {
      putc(buf[i]);
    }
    putc('\n');
  }

  directory_close(fd);
  return PASS;
}

// Repeatedly opens and closes a file/dir.
// Should always get fd 0 if cleanup on close is working correctly
int test_fs_close() {
  TEST_HEADER;
  int cur_fd;
  cur_fd = directory_open((uint8_t*)".");
  printf("cur_fd: %d\n", cur_fd);
  if (cur_fd != 0) assertion_failure();
  directory_close(cur_fd);
  cur_fd = file_open((uint8_t*)"frame0.txt");
  printf("cur_fd: %d\n", cur_fd);
  if (cur_fd != 0) assertion_failure();
  file_close(cur_fd);
  cur_fd = directory_open((uint8_t*)".");
  printf("cur_fd: %d\n", cur_fd);
  if (cur_fd != 0) assertion_failure();
  directory_close(cur_fd);
  return PASS;
}

int test_fs_error_checks() {
  TEST_HEADER;
  printf("Testing fake file\n");
  if (file_open((uint8_t*)"nonsense") != -1) {
    return FAIL;
  }
  printf("testing way too long file \n");
  if (file_open((uint8_t*)"thisstringiswaytoolongforanyfiletoactuallybeabletoaccessit") != -1) {
    return FAIL;
  }

  printf("Testing opening NULL file\n");
  if (file_open(NULL) != -1) {
    return FAIL;
  }
  printf("Testing closing non-open fd\n");
  if (file_close(0) != -1) {
    return FAIL;
  }
  printf("Testing reading non-open fd\n");
  if (file_read(0, NULL, 0) != -1) {
    return FAIL;
  }
  printf("Testing writing non-open fd\n");
  if (file_write(0, NULL, 0) != -1) {
    return FAIL;
  }

  printf("Testing opening NULL dir\n");
  if (directory_open(NULL) != -1) {
    return FAIL;
  }
  printf("Testing reading non-open dir\n");
  if (directory_read(0, NULL, 0) != -1) {
    return FAIL;
  }
  printf("Testing closing non-open dir\n");
  if (directory_close(0) != -1) {
    return FAIL;
  }
  printf("Testing writing to dir\n");
  if (directory_write(0, NULL, 0) != -1) {
    return FAIL;
  }
  return PASS;
}

int test_terminal_write() {
  clear();
  putc('\n');
  TEST_HEADER;
  char* buf = "391OS> ";
  int ret = terminal_write(1, buf, 7);
  if (ret == 7) return PASS;
  return FAIL;
}

int test_terminal_read() {
  TEST_HEADER;
  clear();

  char* buf = "Welcome to taco bell. Want a baja blast?\n";
  char buf2[128];
  char* buf3 = "You said: ";
  int ret = 0;
  terminal_write(1, buf, 42);
  ret = terminal_read(0, buf2, 128);
  terminal_write(1, buf3, 11);
  terminal_write(1, buf2, ret);
  return PASS;
}

int test_terminal_scroll() {
  TEST_HEADER;
  clear();
  int i;
  char buf[2];
  for (i = 0; i < 50; i++) {
    buf[0] = '0' + i / 10;
    buf[1] = '0' + i % 10;
    terminal_write(1, buf, 2);
    terminal_read(0, buf, 0);
  }
  return PASS;
}

int test_read_syscall() {
  read_handler(0, NULL, 0);
  return PASS;
}

int test_execute_syscall() {
  execute_handler((uint8_t*)"shell");
  return PASS;
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */

/* Test suite entry point */
void launch_tests() {
  clear();
  /* CP1 Tests*/
  // TEST_OUTPUT("idt_test", idt_test());
  // TEST_OUTPUT("div0_test", div0_test());
  // TEST_OUTPUT("page_fault_test", page_fault_test());
  // TEST_OUTPUT("page_dereference_succeed_test", page_dereference_succeed_test());
  // TEST_OUTPUT("page_dereference_fail_test", page_dereference_fail_test());

  /* RTC TESTS */
  // rtc test in rtc.c
  // TEST_OUTPUT("test_rtc_freq_fail", test_rtc_freq_fail());
  // TEST_OUTPUT("test_rtc_freq_success", test_rtc_freq_success());
  // TEST_OUTPUT("test_rtc_byte", test_rtc_byte());
  // TEST_OUTPUT("test_rtc_open", test_rtc_open());
  // TEST_OUTPUT("test_rtc_close", test_rtc_close());
  // TEST_OUTPUT("rtc_frequency_test", rtc_frequency_test());

  // Terminal Tests
  // TEST_OUTPUT("test_terminal_write()", test_terminal_write());
  // TEST_OUTPUT("test_terminal_read()", test_terminal_read());
  // TEST_OUTPUT("test_terminal_scroll()", test_terminal_scroll());

  /* FS TESTS */
  // TEST_OUTPUT("test_basic_file_read", test_basic_file_read());
  // TEST_OUTPUT("test_fs_strange_buffer", test_fs_strange_buffer());
  // TEST_OUTPUT("test_executable_read", test_executable_read());
  // TEST_OUTPUT("test_long_file_read", test_long_file_read());
  // TEST_OUTPUT("test_fs_read_dir", test_fs_read_dir());
  // TEST_OUTPUT("test_fs_close", test_fs_close());
  // TEST_OUTPUT("test_fs_error_checks", test_fs_error_checks());
  //
  /* Syscall TESTS */
  // TEST_OUTPUT("test_read_syscall", test_read_syscall());
  // TEST_OUTPUT("test_execute_syscall", test_execute_syscall());
}
