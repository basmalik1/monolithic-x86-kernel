// https://wiki.osdev.org/RTC
#include "../types.h"
#define RTC_PORT 0x70
#define RTC_PORTD 0x71

#define RTC_IRQ 8

#define RTC_A_REG 0x0A
#define RTC_B_REG 0x0B
#define RTC_C_REG 0x0C

#define RTC_INT_MASK 0x80
volatile int rtc_interrupt_flag;
void rtc_init();
int32_t rtc_disable();
void rtc_handler();
void rtc_set_freq(uint32_t f);
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t rtc_open(const uint8_t* filename);
int32_t rtc_close(int32_t fd);
int log2_for_frequency(uint32_t number);
