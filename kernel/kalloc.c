// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

void freerange(void *pa_start, void *pa_end);
extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem {
  struct spinlock lock;
  struct run *freelist;
} kmem_all[NCPU];

char locknames[NCPU][15];
void kfree_percpu(void* pa, int cpu_id);


void
kinit()
{
  //initlock(&kmem.lock, "kmem");
  for(int i = 0; i < NCPU; i++){
    snprintf(locknames[i], 15, "kmem_cpu_%d", i);
    initlock(&kmem_all[i].lock, locknames[i]);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(int i = 0; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree_percpu(p, (i++)%NCPU);
}

void
kfree_percpu(void *pa, int cpu_id)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem_all[cpu_id].lock);
  r->next = kmem_all[cpu_id].freelist;
  kmem_all[cpu_id].freelist = r;
  release(&kmem_all[cpu_id].lock);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  push_off();
  int cpu_id = cpuid();
  pop_off();
  kfree_percpu(pa, cpu_id);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc_percpu(int cpu_id)
{
  struct run *r;

  acquire(&kmem_all[cpu_id].lock);
  r = kmem_all[cpu_id].freelist;
  if(r)
    kmem_all[cpu_id].freelist = r->next;
  release(&kmem_all[cpu_id].lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void *
kalloc()
{
  void *r = 0;
  push_off();
  int cpu_id = cpuid();
  pop_off();
  
  for(int i = 0; i < NCPU && !r; i++)
    r = kalloc_percpu((cpu_id + i) % NCPU);
  return r;
}
