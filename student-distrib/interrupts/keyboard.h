#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "../types.h"

/* Port mappings from https://wiki.osdev.org/%228042%22_PS/2_Controller */
#define KEYBOARD_IRQ_NUM 1
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

/* Modifier Key Mappings */
#define BACKSPACE_SCANCODE 0x0E

#define ENTER_PRESSED_SCANCODE 0x1C

#define LEFT_SHIFT_SCANCODE 0x2A
#define LEFT_SHIFT_RELEASED_SCANCODE 0xAA

#define RIGHT_SHIFT_SCANCODE 0x36
#define RIGHT_SHIFT_RELEASED_SCANCODE 0xB6

#define LEFT_CONTROL_SCANCODE 0x1D
#define LEFT_CONTROL_RELEASED_SCANCODE 0x9D

#define CAPS_LOCK_SCANCODE 0x3A
#define CAPS_LOCK_RELEASED_SCANCODE 0xBA

#define L_SCANCODE 0x26

#define LAST_PRESSED_SCAN_CODE 0x58

#define TAB_PRESSED_SCANCODE 0x0F

#define LEFT_ALT_PRESSED_SCANCODE 0x38
#define LEFT_ALT_RELEASED_SCANCODE 0xB8

#define F1_PRESSED_SCANCODE 0x3B
#define F3_PRESSED_SCANCODE 0x3D

/* Terminal */
#define TERMINAL_BUFFER_SIZE 128

void keyboard_init(void);
void keyboard_handler(void);

int32_t terminal_open(const uint8_t* filename);
int32_t terminal_close(int32_t fd);
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

#endif /* _KEYBOARD_H */
