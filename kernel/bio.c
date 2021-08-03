// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define GLOBAL_BCACHELOCK

#define NBUCKET 13

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  //struct buf freelist;
  uint ticks[NBUF];
  struct spinlock locklist[NBUCKET];
  struct buf hashtable[NBUCKET];
} bcache;

static char locknames[NBUCKET][15];

static uint argmin(uint list[], uint len){
  //do something
  uint minidx = 0;
  uint minval = -1;
  for(int i = 0; i < len; i++){
    if(minval > list[i]){
      minval = list[i];
      minidx = i;
    }
  }
  return minidx;
}



void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  for(int i = 0; i < NBUCKET; i++){
    snprintf(locknames[i], i, "bcache_%d", i);
    initlock(&bcache.locklist[i], locknames[i]);
  }

  for(int i = 0; i < NBUCKET; i++){
    bcache.hashtable[i].prev = &bcache.hashtable[i];
    bcache.hashtable[i].next = &bcache.hashtable[i];
  }
  for(int i = 0; i < NBUF; i++){
    int hashval = i % NBUCKET;
    b = &bcache.buf[i];
    b->bufnum = i;
    bcache.ticks[i] = ticks;
    b->next = bcache.hashtable[hashval].next;
    b->prev = &bcache.hashtable[hashval];
    initsleeplock(&b->lock, "buffer");
    bcache.hashtable[hashval].next->prev = b;
    bcache.hashtable[hashval].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int hashval = blockno % NBUCKET;

  acquire(&bcache.locklist[hashval]);

  // Is the block already cached?
  for(b = bcache.hashtable[hashval].next; b != &bcache.hashtable[hashval]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      bcache.ticks[b->bufnum] = -1;
      b->refcnt++;
      release(&bcache.locklist[hashval]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // No? then first find empty buffer in current bucket
  for(b = bcache.hashtable[hashval].next; b != &bcache.hashtable[hashval]; b = b->next){
    if(b->refcnt == 0){
      bcache.ticks[b->bufnum] = -1;
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.locklist[hashval]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.locklist[hashval]);
  //Not exist? Use LRU buffer.
  uint bufnum = argmin(bcache.ticks, NBUF);
  if(bufnum == -1)
    panic("bget: no buffer");
  
  b = &bcache.buf[bufnum];
  uint hashval_new = b->blockno % NBUCKET;
  
  if(b->refcnt != 0)
    panic("bget: used buffer");
  
  acquire(&bcache.locklist[hashval_new]);
  bcache.ticks[bufnum] = -1;
  b->prev->next = b->next;
  b->next->prev = b->prev;
  release(&bcache.locklist[hashval_new]);
  acquire(&bcache.locklist[hashval]);
  b->next = bcache.hashtable[hashval].next;
  b->prev = &bcache.hashtable[hashval];
  bcache.hashtable[hashval].next->prev = b;
  bcache.hashtable[hashval].next = b;
  b->dev = dev;
  b->blockno = blockno;
  b->valid = 0;
  b->refcnt = 1;
  release(&bcache.locklist[hashval]);
  acquiresleep(&b->lock);
  return b;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint hashval = b->blockno % NBUCKET;

  acquire(&bcache.locklist[hashval]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    bcache.ticks[b->bufnum] = ticks;
  }
  
  release(&bcache.locklist[hashval]);
}

void
bpin(struct buf *b) {
  uint hashval = b->blockno % NBUCKET;
  acquire(&bcache.locklist[hashval]);
  b->refcnt++;
  release(&bcache.locklist[hashval]);
}

void
bunpin(struct buf *b) {
  uint hashval = b->blockno % NBUCKET;
  acquire(&bcache.locklist[hashval]);
  b->refcnt--;
  release(&bcache.locklist[hashval]);
}


