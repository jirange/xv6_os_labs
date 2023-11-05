// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);
void kfree_by_cpu_id(void *pa,int cpu_id);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct kmem kmems[NCPU];

void
kinit()
{
  for (int cpu = 0; cpu < NCPU; cpu++) {
      initlock(&kmems[cpu].lock, "kmem");
  }
  freerange(end, (void*)PHYSTOP);
}


void freerange(void *pa_start, void *pa_end)
{
  char *p = (char*)PGROUNDUP((uint64)pa_start);

  // 把空间均匀连续的分给各个CPU
  uint64 total_pages = ((uint64)pa_end - (uint64)pa_start) / PGSIZE;
  uint64 pages_per_cpu = total_pages / NCPU;
  uint64 extra_pages = total_pages % NCPU;

  for (int cpu = 0; cpu < NCPU; cpu++)
  {
    uint64 cpu_pages = pages_per_cpu;
    if (extra_pages > 0)
    {
      cpu_pages++;
      extra_pages--;
    }

    char *q = p;
    for (uint64 i = 0; i < cpu_pages; i++)
    {
      kfree_by_cpu_id(q, cpu);
      q += PGSIZE;
    }

    p = q;
  }
}


// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  struct kmem *kmp = &kmems[cpuid()];
  pop_off();

  acquire(&kmp->lock);
  r->next = kmp->freelist;
  kmp->freelist = r;
  release(&kmp->lock);
}

void
kfree_by_cpu_id(void *pa,int cpu_id)
{
  struct run *r;
  struct kmem *kmp = &kmems[cpu_id];

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmp->lock);
  r->next = kmp->freelist;
  kmp->freelist = r;
  release(&kmp->lock);
}


// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off();
  int cpu_id = cpuid();
  pop_off();
  
  struct kmem *kmp = &kmems[cpu_id];
  acquire(&kmp->lock);
  r = kmp->freelist;
  
    if(r){
		kmp->freelist = r->next;
		release(&kmp->lock);
		memset((char*)r, 5, PGSIZE); // fill with junk
		return (void*)r;
	}
	else{
		// Try to steal memory from other CPUs
		for (int i = 0; i < NCPU; i++)
		{
		  if (i == cpu_id) // Skip current CPU
			continue;

		  struct kmem *other_kmp = &kmems[i];
		  acquire(&other_kmp->lock);
		  r = other_kmp->freelist;
		  if (r)
		  {
			other_kmp->freelist = r->next;
			release(&other_kmp->lock);
			memset((char *)r, 5, PGSIZE); // fill with junk
			release(&kmp->lock);
			return (void *)r;
		  }else release(&other_kmp->lock);
		}
		release(&kmp->lock);
		return 0; // No free memory blocks available
	}
  return (void*)r;
}