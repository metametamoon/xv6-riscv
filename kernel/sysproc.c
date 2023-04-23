#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"
#include "lock_consts.h"
#include "pr_msg.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


#define n 256

struct {
    struct spinlock main_lock;
    short owned[n];
    struct sleeplock sleeplocks[n];
} syslocks;

void init() {
    initlock(&syslocks.main_lock, "main_lock");
    for (int i = 0; i < n; ++i) {
        syslocks.owned[i] = 0;
        initsleeplock(&syslocks.sleeplocks[i], "sleep_lock");
    }
}


int check_index(int index) {
  if (index < 0 || index >= n) {
    return -1;
  } else {
    return 0;
  }
}

int syslock(int request, int index) {
    acquire(&syslocks.main_lock);
    if (request == LK_OPEN) {
        for (int i = 0; i < n; ++i) {
            if (syslocks.owned[i] == 0) {
                syslocks.owned[i] = 1;
                release(&syslocks.main_lock);
                return i;
            }
        }
        release(&syslocks.main_lock);
        return -1;
    } else if (request == LK_CLOSE) {
        if (check_index(index) == -1 || syslocks.owned[index] != 1) {
          release(&syslocks.main_lock);
          return -1;
        }
        syslocks.owned[index] = 0;
        release(&syslocks.main_lock);
        return 0;
    } else if (request == LK_ACQ) {
        if (check_index(index) == -1 || syslocks.owned[index] != 1) {
          release(&syslocks.main_lock);
          return -1;
        }
        release(&syslocks.main_lock);
        acquiresleep(&syslocks.sleeplocks[index]);
        return 0;

    } else if (request == LK_REL) {
        if (check_index(index) == -1 || syslocks.owned[index] != 1) {
          release(&syslocks.main_lock);
          return -1;
        }
        release(&syslocks.main_lock);
        releasesleep(&syslocks.sleeplocks[index]);
        return 0;
    }
    return -1; // unknown request type
}


uint64 sys_lock(void) {
  int request;
  int index;

  argint(0, &request);
  argint(1, &index);

  return syslock(request, index);
}


uint64 sys_dmesg(void) {
  char* buff;
  argaddr(0, (uint64*) &buff);
  int max_length;
  argint(1, &max_length);
  return dmesg(buff, max_length);
}