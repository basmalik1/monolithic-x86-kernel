#ifndef _FS_H
#define _FS_H

#include "interrupts/keyboard.h"
#include "interrupts/rtc.h"
#include "lib.h"
#include "types.h"

int32_t directory_open(const uint8_t* filename);
int32_t directory_close(int32_t fd);
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes);
int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t file_open(const uint8_t* filename);
int32_t file_close(int32_t fd);
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t get_free_fd();
int check_valid_fd(int32_t fd);

// Basic initialization for the file system
void fs_init(uint32_t boot_dev);

// Defining d_entry, inodes, etc

typedef struct __attribute__((__packed__)) dentry_t {
  uint8_t file_name[32];  // bits 0-31
  uint32_t file_type;     // bits 32-35
  uint32_t inode;         // bits 36-39
  char reserved[24];      // bits 36-39
} dentry_t;

typedef struct __attribute__((__packed__)) bootblock_t {
  uint32_t n_dir_entries;
  uint32_t n_inodes;
  uint32_t n_data_blocks;
  uint32_t reserved[13];
  dentry_t dentries[63];
} bootblock_t;

typedef struct __attribute__((__packed__)) inode_t {
  uint32_t length;
  uint32_t data_blocks[127];
} inode_t;

// Read the dentry by the given file name
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

int32_t get_free_fd();
int32_t check_valid_fd(int32_t fd);

typedef struct fd_ops_t {
  int32_t (*open)(const uint8_t*);
  int32_t (*close)(int32_t);
  int32_t (*read)(int32_t, void*, int32_t);
  int32_t (*write)(int32_t, const void*, int32_t);
} fd_ops_t;

typedef struct __attribute__((__packed__)) fd_t {
  fd_ops_t* file_table_ptr;
  uint32_t inode;
  volatile uint32_t file_pos;
  uint32_t flags;
} fd_t;

extern fd_ops_t directory_jumptable;
extern fd_ops_t file_jumptable;
extern fd_ops_t terminal_jumptable;
extern fd_ops_t rtc_jumptable;

fd_t* file_descriptors;
#endif
