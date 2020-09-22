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

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// Stores reference count of pa instead of va <-> pa convergen.
// only stores 37bit instead of 44 bit of physical address, but 0~PHYSTOP is 32bit so doesn't matter.
struct {
  struct spinlock lock;
  pagetable_t reftable;
} kref;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
  initlock(&kref.lock, "kref");
  kref.reftable = (pagetable_t)kalloc();
  memset(kref.reftable, 0, PGSIZE);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
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

  // Memory reference counting for CoW
  if(dec_ref(pa) > 0)
    return;

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

int inc_ref(void* pa){
  pte_t *rte;
  uint refcnt;
  
  if(kref.reftable == 0)
    return 0;

  acquire(&kref.lock);
  rte = walk(kref.reftable, (uint64)pa, 1);
  if(rte == 0){
    release(&kref.lock);
    return 0;
  }
  if((*rte & PTE_V) == 0){
    refcnt = 1;
    *rte = REF2RTE(1) | PTE_V | PTE_U;
  }
  else{
    refcnt = RTE2REF(*rte) + 1;
    *rte = REF2RTE(refcnt) | PTE_V | PTE_U;
  }
  release(&kref.lock);
  return refcnt;
}

int dec_ref(void* pa){
  pte_t *rte;
  uint refcnt;

  if(kref.reftable == 0)
    return 0;

  acquire(&kref.lock);
  rte = walk(kref.reftable, (uint64)pa, 0);
  if((rte == 0) || ((*rte & PTE_V) == 0)){
    *rte = 0;
    release(&kref.lock);
    return 0;
  }

  refcnt = RTE2REF(*rte) - 1;
  if(refcnt <= 0){
    *rte = 0;
  }
  else{
    *rte = REF2RTE(refcnt) | PTE_V | PTE_U;
  }
  release(&kref.lock);
  return refcnt;
}

int see_ref(void *pa){
  pte_t *rte;
  uint refcnt;

  if(kref.reftable == 0)
    return -1;

  acquire(&kref.lock);
  rte = walk(kref.reftable, (uint64)pa, 0);
  if((rte == 0) || ((*rte & PTE_V) == 0)){
    release(&kref.lock);
    return 0;
  }

  refcnt = RTE2REF(*rte);
  release(&kref.lock);
  return refcnt;
}