#include "fs.h"

static void* fs_img_start = (void*)32256;

struct fd_ops_t directory_jumptable = {
    .open = directory_open,
    .close = directory_close,
    .read = directory_read,
    .write = directory_write,
};

struct fd_ops_t file_jumptable = {
    .open = file_open,
    .close = file_close,
    .read = file_read,
    .write = file_write,
};

struct fd_ops_t terminal_jumptable = {
    .open = terminal_open,
    .close = terminal_close,
    .read = terminal_read,
    .write = terminal_write,
};

struct fd_ops_t rtc_jumptable = {
    .open = rtc_open,
    .close = rtc_close,
    .read = rtc_read,
    .write = rtc_write,
};

// Basic initialization for the file system, sets fds to 0
void fs_init(uint32_t boot_dev) {
  // int i;
  // for (i = 0; i < 8; i++) {
  //   file_descriptors[i].inode = -1;
  // }
  fs_img_start = (void*)boot_dev;
}

/* check_valid_fd
 *   DESCRIPTION: checks if the file descriptor is valid
 *   INPUTS: fd - file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: return 0 on success and -1 on failure
 *   SIDE EFFECTS: none
 */
int check_valid_fd(int32_t fd) {
  if (fd < 0 || fd >= 8) return -1;
  if (file_descriptors[fd].inode == -1) return -1;
  return 0;
}

// NOT IMPLEMENTED
/* directory_write
 *   DESCRIPTION: writes to the directory
 *   INPUTS: fd - file descriptor
 *           buf - a pointer to the buffer
 *           nbytes - number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: return -1 for failure
 *   SIDE EFFECTS: none
 */
int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes) {
  return -1;
}

// Opens the directory
/* directory_open
 *   DESCRIPTION: opens the directory
 *   INPUTS: filename - the name of the file
 *   OUTPUTS: none
 *   RETURN VALUE: return -1 for failure, otherwise return the file descriptor
 *   SIDE EFFECTS: sets the file descriptor
 */
int32_t directory_open(const uint8_t* filename) {
  int fd;
  dentry_t dentry;
  // Could not find the directory
  if (read_dentry_by_name(filename, &dentry) == -1) {  // checks if file is null as well
    return -1;                                         // Could not find the directory
  }
  if (dentry.file_type != 1) {
    return -1;  // Not a directory
  }
  // Find an fd
  if ((fd = get_free_fd()) == -1) {
    return -1;  // No free fd
  }
  file_descriptors[fd].file_table_ptr = &directory_jumptable;
  // Copy over/init info
  file_descriptors[fd].inode = dentry.inode;
  file_descriptors[fd].file_pos = 0;
  file_descriptors[fd].flags = 0;
  return fd;
}

/* directory_close
 *   DESCRIPTION: closes the directory
 *   INPUTS: fd - file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: return 0 for success and -1 for failure
 *   SIDE EFFECTS: closes the directory
 */
int32_t directory_close(int32_t fd) {
  // Set as not in use
  //   printf("closing fd: %d", fd);
  if (check_valid_fd(fd) == -1) return -1;  // invalid fd
  file_descriptors[fd].inode = -1;
  return 0;
}

/* directory_read
 *   DESCRIPTION: reads the directory
 *   INPUTS: fd - file descriptor
 *           buf - a pointer to the buffer
 *           nbytes - number of bytes to read
 *   OUTPUTS: data copied to the buffer
 *   RETURN VALUE: return -1 for failure, otherwise return the number of bytes read
 *   SIDE EFFECTS: none
 */
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes) {
  int ss_len;
  int str_len;
  // Read one dentry and copy it over
  dentry_t dentry;
  if (check_valid_fd(fd) == -1) return -1;  // invalid fd

  if (read_dentry_by_index(file_descriptors[fd].file_pos, &dentry) == -1) {
    return -1;  // Could not find the index
  }
  if (buf == NULL && nbytes != 0) {
    return -1;  // cant write non-zero bytes to a null buffer
  }
  if (nbytes < 0) {
    return -1;  // cant write negative bytes
  }
  ss_len = nbytes;  // max length of filename
  if (ss_len > 32) ss_len = 32;
  str_len = strlen((int8_t*)dentry.file_name);
  if (ss_len > str_len) ss_len = str_len;

  if (ss_len != 0) {  // only copy if there is something to copy
    memcpy((int8_t*)buf, &(dentry.file_name), ss_len);
  }
  file_descriptors[fd].file_pos++;
  return ss_len;
}

/* file_open
 *   DESCRIPTION: opens the file
 *   INPUTS: filename - the name of the file
 *   OUTPUTS: none
 *   RETURN VALUE: return -1 for failure, otherwise return the file descriptor
 *   SIDE EFFECTS: sets the file descriptor
 */
int32_t file_open(const uint8_t* filename) {
  int fd;
  dentry_t dentry;
  // Could not find the directory
  if (read_dentry_by_name(filename, &dentry) == -1) {
    return -1;
  }
  if (dentry.file_type != 2) {
    return -1;  // Not a file
  }
  // Find an fd
  if ((fd = get_free_fd()) == -1) {
    return -1;  // No free fd
  }
  //   printf("Got fd: %d\n", fd);
  file_descriptors[fd].file_table_ptr = &file_jumptable;
  // Copy over/init info
  file_descriptors[fd].inode = dentry.inode;
  // printf("Returning inode: %d\n", dentry.inode);
  file_descriptors[fd].file_pos = 0;
  file_descriptors[fd].flags = 0;
  return fd;
}

/* file_close
 *   DESCRIPTION: closes the file
 *   INPUTS: fd - file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: return 0 for success and -1 for failure
 *   SIDE EFFECTS: closes the file
 */
int32_t file_close(int32_t fd) {
  //   printf("closing fd: %d", fd);
  if (check_valid_fd(fd) == -1) return -1;  // invalid fd

  file_descriptors[fd].inode = -1;
  return 0;
}

/* file_read
 *   DESCRIPTION: reads the file
 *   INPUTS: fd - file descriptor
 *           buf - a pointer to the buffer
 *           nbytes - number of bytes to read
 *   OUTPUTS: data copied to the buffer
 *   RETURN VALUE: return -1 for failure, otherwise return the number of bytes read
 *   SIDE EFFECTS: none
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes) {
  // fd is not valid
  int cur_inode_num, bytes_read;
  if (check_valid_fd(fd) == -1) return -1;  // invalid fd

  if (buf == NULL) return -1;
  if (nbytes <= 0) return -1;
  // Get the inode from the fd
  cur_inode_num = file_descriptors[fd].inode;
  // printf("Reading inode #%d\n", cur_inode_num);
  // Read the data, data block by data block
  bytes_read = read_data(cur_inode_num, file_descriptors[fd].file_pos, buf, nbytes);
  // Update the position in the file descriptor
  file_descriptors[fd].file_pos += bytes_read;
  // return
  // printf("Bytes read: %d, nbytes: %d, fpos: %d\n", bytes_read, nbytes, file_descriptors[fd].file_pos);
  return bytes_read;
}

// NOT IMPLEMENTED
/* file_write
 *   DESCRIPTION: writes to the file
 *   INPUTS: fd - file descriptor
 *           buf - a pointer to the buffer
 *           nbytes - number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: return -1 for failure
 *   SIDE EFFECTS: none
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes) {
  return -1;
}

/* read_dentry_by_name
 *   DESCRIPTION: reads the dentry by name
 *   INPUTS: fname - the name of the file
 *           dentry - a pointer to the dentry
 *   OUTPUTS: data copied to the dentry
 *   RETURN VALUE: return -1 for failure, otherwise return 0
 *   SIDE EFFECTS: none
 */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry) {
  int i, ss_len;
  // Get the init boot block
  bootblock_t* bootblk = ((bootblock_t*)fs_img_start);
  if (fname == NULL) return -1;   // Null check
  if (dentry == NULL) return -1;  // Null check
  ss_len = strlen((int8_t*)fname);
  if (ss_len > 32) ss_len = 32;
  // iterate over the direntries
  for (i = 0; i < 63; i++) {
    // If found, copy over the info
    // printf("Checking Filename:%s vs. %s\n", fname, bootblk.dentries[i].file_name);
    //  printf("Checking Filename:%d vs. %s\n", ss_len, bootblk.dentries[i].file_name);
    if (strncmp((int8_t*)fname, (int8_t*)bootblk->dentries[i].file_name, ss_len) == 0) {
      int file_name_len = 0;
      int j;
      for (j = 0; j < 32; j++) {  // check to make sure the file name is the same length
        if (bootblk->dentries[i].file_name[j] != '\0') {
          file_name_len++;
        } else {
          break;
        }
      }
      if (file_name_len != ss_len) {
        continue;
      }
      strncpy((int8_t*)&(dentry->file_name), (int8_t*)&(bootblk->dentries[i].file_name), ss_len);
      dentry->file_type = bootblk->dentries[i].file_type;
      dentry->inode = bootblk->dentries[i].inode;
      return 0;
    }
  }
  // Else return 0
  return -1;
}

/* read_dentry_by_index
 *   DESCRIPTION: reads the dentry by index
 *   INPUTS: index - the index of the file
 *           dentry - a pointer to the dentry
 *   OUTPUTS: data copied to the dentry
 *   RETURN VALUE: return -1 for failure, otherwise return 0
 *   SIDE EFFECTS: none
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {
  if (index < 0 || index >= 63) return -1;
  bootblock_t* bootblk = ((bootblock_t*)fs_img_start);
  strncpy((int8_t*)&(dentry->file_name), (int8_t*)&(bootblk->dentries[index].file_name), 32);
  dentry->file_type = bootblk->dentries[index].file_type;
  dentry->inode = bootblk->dentries[index].inode;
  // printf("Copying over: %s\n", dentry->file_name);
  return 0;
}
// Read data from a given inode block
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {
  int i, start, bytes_read, cur_data_block, len, n_blocks;
  void* data_block_ptr;
  // Need to get #inodes
  int n_inodes = ((bootblock_t*)fs_img_start)->n_inodes;
  // Get the pointer to the inode from the fs_img_start (+1 since first is boot block);
  inode_t* cur_inode = ((inode_t*)(fs_img_start + 4096 * (inode + 1)));
  if (offset >= cur_inode->length) return 0;
  // Edge case for the first/last block
  start = offset % 4096;
  // printf("lengthof file: %d\n", cur_inode.length);
  n_blocks = (cur_inode->length + 4095) / 4096;
  bytes_read = 0;
  for (i = offset / 4096; i < n_blocks; i++) {
    if (length == 0) break;
    cur_data_block = cur_inode->data_blocks[i];
    // printf("Copying from data block: %d\n", cur_data_block);
    data_block_ptr = fs_img_start + 4096 * (n_inodes + 1 + cur_data_block) + start;
    // If not copying entire block (for starting block only)
    len = 4096 - start;
    // If not copying entire block
    if (len > length) {
      len = length;
    }
    // only copy to the end of the file
    if (i * 4096 + len + start > cur_inode->length) {
      len = cur_inode->length - (i * 4096 + start);
    }
    // printf("Copying %d bytes\n", len);
    // Copy
    memcpy(buf, data_block_ptr, len);
    // Accounting
    bytes_read += len;
    length -= len;
    buf += len;
    // No longer edge casing start block
    start = 0;
  }
  return bytes_read;
}

/* get_free_fd
 *   DESCRIPTION: gets a free file descriptor
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: return the free file descriptor, -1 if none
 *   SIDE EFFECTS: none
 */
int32_t get_free_fd() {
  int i;
  for (i = 2; i < 8; i++) {
    if (file_descriptors[i].inode == -1) {
      return i;
    }
  }
  return -1;
}
