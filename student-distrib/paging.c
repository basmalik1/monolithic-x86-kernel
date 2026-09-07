#include "paging.h"

#include "lib.h"
#include "x86_desc.h"

/* paging_init
 *   DESCRIPTION: Initializes the paging
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Initializes the paging
 */
void paging_init() {
  // actually initalize stuff
  /*In our address space 0-4Kb is in 4Kb pages and should only have one
   * present page: the video memory page (see VIDEO_MEM_INDEX),
   * all others should be marked as not present.
   * These need to be populated so that they point to the 0-4KB in physical memory
   */
  uint32_t index;
  for (index = 0; index < 1024; index++) {  // 1024 entries in page table
    // Zero out the page table entry
    page_table[index].val = 0;
    // Set the address bits for the entry,
    page_table[index].address_low = index;
    // Set the present bit to zero for the rest (redundant I think, but more readable)
    // Set read/write to 1, this should be the default from what I can tell
    page_table[index].read_write = 1;
  }
  // Set the page table entry for the video memory
  page_table[VIDEO_MEM_INDEX].user_supervisor = 0;  // dissallow user access to regular video memory
  page_table[VIDEO_MEM_INDEX].present = 1;

  // user version of video memory for terminal 1
  page_table[VIDEO_MEM_INDEX + 1].address_low = VIDEO_MEM_INDEX;  // map to same physical address
  page_table[VIDEO_MEM_INDEX + 1].user_supervisor = 1;            // allow user access
  page_table[VIDEO_MEM_INDEX + 1].present = 1;                    // set present bit

  // user version of video memory for terminal 2
  page_table[VIDEO_MEM_INDEX + 2].address_low = VIDEO_MEM_INDEX + 2;  // map to same physical address
  page_table[VIDEO_MEM_INDEX + 2].user_supervisor = 1;                // allow user access
  page_table[VIDEO_MEM_INDEX + 2].present = 1;                        // set present bit

  // user version of video memory for terminal 3
  page_table[VIDEO_MEM_INDEX + 3].address_low = VIDEO_MEM_INDEX + 3;  // map to same physical address
  page_table[VIDEO_MEM_INDEX + 3].user_supervisor = 1;                // allow user access
  page_table[VIDEO_MEM_INDEX + 3].present = 1;                        // set present bit

  // kenel version of video memory for terminal (current)
  page_table[VIDEO_MEM_INDEX + 4].address_low = VIDEO_MEM_INDEX + 1;  // map to same physical address
  page_table[VIDEO_MEM_INDEX + 4].user_supervisor = 0;                // allow user access
  page_table[VIDEO_MEM_INDEX + 4].present = 1;                        // set present bit

  /*
   * From the page directory entries, we only initialize two for now. the first one
   * will map to the physical address of the page table entries (since we want 1-1 mapping
   * of virtual to physical for now). Then we also set it to having 4KB pages (page size bit = 0)
   *
   * The second page directory entry is mapped to a 4MB page (with page size bit = 1)
   * where the offset just points to the 4Mb in physical memory and we mark it as present
   *
   */
  // Init the first page to zero
  page_directory[0].val = 0;
  page_directory[0].pd_4kb.address_31_12 = ((uint32_t)page_table) >> 12;
  page_directory[0].pd_4kb.present = 1;
  page_directory[0].pd_4kb.user_supervisor = 1;  // set control on 4kb basis
  page_directory[0].pd_4kb.read_write = 1;       // allow read/write
  // Any other bits we need to set by default?

  // Init the second page
  page_directory[1].val = 0;

  // Set it 4MB (where our kernel is) (0x400000 >> 22 = 1)
  page_directory[1].pd_4mb.address_31_22 = 1;
  // Set page_size = 1, indicating 4MB pages
  page_directory[1].pd_4mb.page_size = 1;
  // Set it to present
  page_directory[1].pd_4mb.present = 1;
  // sets page register, and then enables paging

  // set up userspace page directory (index 33)
  page_directory[32].pd_4mb.address_31_22 = 0xFF;  // will be set by execute syscall
  page_directory[32].pd_4mb.page_size = 1;
  page_directory[32].pd_4mb.present = 1;
  page_directory[32].pd_4mb.read_write = 1;
  page_directory[32].pd_4mb.user_supervisor = 1;

  asm(
      "movl $[page_directory], %%eax \n \
      movl %%eax, %%cr3 \n              \
                                        \
      movl %%cr4, %%eax \n              \
      orl $0x00000010, %%eax \n         \
      movl %%eax, %%cr4 \n              \
                                        \
      movl %%cr0, %%eax \n              \
      orl $0x80000001, %%eax \n         \
      movl %%eax, %%cr0" : : : "eax", "cc");
}
