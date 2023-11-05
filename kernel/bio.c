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

# define NBUCKETS 13
#define HASHFUNC(blockno) (blockno % NBUCKETS) // 哈希函数，根据块号决定哈希桶
#define NULL ((void*)0)


struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];

  struct buf hashbucket[NBUCKETS]; //每个哈希队列一个linked list及一个lock
} bcache;

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < NBUCKETS; i++){
    initlock(&bcache.lock[i], "bcache");
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }

  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
	int hashIndex =(uint64)b % NBUCKETS;
    b->next = b->prev = 0;
	
	b->next = bcache.hashbucket[hashIndex].next;
    b->prev = &bcache.hashbucket[hashIndex];
    initsleeplock(&b->lock, "buffer");
   bcache.hashbucket[hashIndex].next->prev = b;
    bcache.hashbucket[hashIndex].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int hashIndex = HASHFUNC(blockno);

  acquire(&bcache.lock[hashIndex]);

  // Is the block already cached?
  for(b = bcache.hashbucket[hashIndex].next; b != &bcache.hashbucket[hashIndex]; b = b->next){
  if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[hashIndex]);
      //release(&bcache.h[hindex].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
    for(b = bcache.hashbucket[hashIndex].next; b != &bcache.hashbucket[hashIndex]; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        release(&bcache.lock[hashIndex]);
        acquiresleep(&b->lock);
        return b;
      }
    }
 // 本桶里没空闲块了，去别的桶里拿
  for(int i=0;i<NBUCKETS;++i){
    if(i!=hashIndex){
      acquire(&bcache.lock[i]);
      for(b = bcache.hashbucket[i].prev; b != &bcache.hashbucket[i]; b = b->prev){
        
        if(b->refcnt==0){
          //founded!
          //把b从原来的里面删掉  双向链表删除结点
          b->prev->next = b->next;
          b->next->prev = b->prev;

          //把b插入到这个哈希桶中
          b->next = bcache.hashbucket[hashIndex].next;
          b->prev = &bcache.hashbucket[hashIndex];
          bcache.hashbucket[hashIndex].next->prev = b;
          bcache.hashbucket[hashIndex].next = b;

          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          release(&bcache.lock[i]);
          release(&bcache.lock[hashIndex]);
          acquiresleep(&b->lock);
          return b;
        }  
      }
      release(&bcache.lock[i]);
    }
  }
  panic("bget: no buffers");
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

  int index = HASHFUNC(b->blockno);
  //acquire(&bcache.lock);
  acquire(&bcache.lock[index]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[index].next;
    b->prev = &bcache.hashbucket[index];
    bcache.hashbucket[index].next->prev = b;
    bcache.hashbucket[index].next = b;
  }
  
  release(&bcache.lock[index]);
}

void
bpin(struct buf *b) {
  int index = HASHFUNC(b->blockno);
  acquire(&bcache.lock[index]);
  b->refcnt++;
  release(&bcache.lock[index]);
}

void
bunpin(struct buf *b) {
  int index = HASHFUNC(b->blockno);
  acquire(&bcache.lock[index]);
  b->refcnt--;
  release(&bcache.lock[index]);
}


