#ifndef EXAMPLE_MREMAP_H
#define EXAMPLE_MREMAP_H

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>

#define __base_mremap(__op, _orig_mem, _orig_size, _new_size, _flags, ...) \
	({ \
	 	void *____addr = mremap((_orig_mem), (_orig_size), (_new_size), (_flags), __VA_ARGS__); \
	 	if(____addr == MAP_FAILED) { \
	 		perror("mremap"); \
	 		_exit(EXIT_FAILURE); \
	 	} \
	 	printf("MREMAP(" __op "): %p -> %p\n", (_orig_mem), ____addr); \
	 	____addr; \
	 })

#define __s_assert_nr_pages(x) \
	_Static_assert( \
			__builtin_types_compatible_p(__typeof__((x)), int) && (x) > 0, \
			#x " must be integer and greater than 0")

#define __mremap_move(mem, cur_nr_pages) \
	({ \
		__s_assert_nr_pages(cur_nr_pages); \
	 	void *__mem_move_to__ = (mem) - 2 * PAGE_SIZE; \
	 	void *__new_addr__ = __base_mremap ( \
	 			"moved", \
	 			(mem), \
	 			cur_nr_pages * PAGE_SIZE, \
	 			cur_nr_pages * PAGE_SIZE, \
	 			MREMAP_FIXED | MREMAP_MAYMOVE, \
	 			__mem_move_to__); \
	 	assert(__mem_move_to__ == __new_addr__); \
	 	__new_addr__; \
	})

/* pay attention here:
 *
 * from man mremap:
 *
 * "[...] After completion, any access to the range  specified  by  old_ad‐
 * dress  and  old_size will result in a page fault.  The page fault
 * will be handled by a userfaultfd(2) handler if the address is  in
 * a  range  previously  registered with userfaultfd(2).  Otherwise,
 * the kernel allocates a zero-filled page to handle the fault."
 * 
 * I initially thought that page frames were pointed by two different PTEs 
 * belonging to the same process, instead, the old mapping will get new
 * physical memory eventually.
 */
#define __mremap_move_dontunmap(mem, cur_nr_pages) \
	({ \
	 	__s_assert_nr_pages(cur_nr_pages); \
	 	void *__mem_move_to__ = (mem) - 2 * PAGE_SIZE; \
	 	void *__new_addr__ = __base_mremap ( \
	 			"moved+dontunmap", \
	 			(mem), \
	 			cur_nr_pages * PAGE_SIZE, \
	 			cur_nr_pages * PAGE_SIZE, \
	 			MREMAP_FIXED | MREMAP_MAYMOVE | MREMAP_DONTUNMAP, \
	 			__mem_move_to__); \
	 	assert(__mem_move_to__ == __new_addr__); \
	 	__new_addr__; \
	})

#define __mremap_shrink_or_extend(mem, cur_nr_pages, new_nr_pages) \
	({ \
	 	__s_assert_nr_pages(cur_nr_pages); \
	 	__s_assert_nr_pages(new_nr_pages); \
	 	void *__new_addr__ = __base_mremap ( \
	 			"shrink-or-extend", \
	 			(mem), \
	 			cur_nr_pages * PAGE_SIZE, \
	 			new_nr_pages * PAGE_SIZE, \
	 			MREMAP_MAYMOVE, \
	 			NULL); \
	 	__new_addr__; \
	})

#endif
