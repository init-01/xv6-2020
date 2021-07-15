#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
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
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
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

  backtrace();

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
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

  if(argint(0, &pid) < 0)
    return -1;
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

uint64
sys_sigalarm(void)
{
  int ticks;
  uint64 handler;

  if(argint(0, &ticks) < 0)
    return -1;
  if(argaddr(1, &handler) < 0)
    return -1;

  struct proc *p = myproc();
  acquire(&p->lock);
  p->alarm_ticks = ticks;
  p->alarm_handler = handler;
  release(&p->lock);
  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->handler_on = 0;
  p->trapframe->epc = p->userframe->pc;
  p->trapframe->ra  = p->userframe->ra;
  p->trapframe->sp  = p->userframe->sp;
  p->trapframe->gp  = p->userframe->gp;
  p->trapframe->tp  = p->userframe->tp;
  p->trapframe->t0  = p->userframe->t0;
  p->trapframe->t1  = p->userframe->t1;
  p->trapframe->t2  = p->userframe->t2;
  p->trapframe->s0  = p->userframe->s0;
  p->trapframe->s1  = p->userframe->s1;
  p->trapframe->a0  = p->userframe->a0;
  p->trapframe->a1  = p->userframe->a1;
  p->trapframe->a2  = p->userframe->a2;
  p->trapframe->a3  = p->userframe->a3;
  p->trapframe->a4  = p->userframe->a4;
  p->trapframe->a5  = p->userframe->a5;
  p->trapframe->a6  = p->userframe->a6;
  p->trapframe->a7  = p->userframe->a7;
  p->trapframe->s2  = p->userframe->s2;
  p->trapframe->s3  = p->userframe->s3;
  p->trapframe->s4  = p->userframe->s4;
  p->trapframe->s5  = p->userframe->s5;
  p->trapframe->s6  = p->userframe->s6;
  p->trapframe->s7  = p->userframe->s7;
  p->trapframe->s8  = p->userframe->s8;
  p->trapframe->s9  = p->userframe->s9;
  p->trapframe->s10 = p->userframe->s10;
  p->trapframe->s11 = p->userframe->s11;
  p->trapframe->t3  = p->userframe->t3;
  p->trapframe->t4  = p->userframe->t4;
  p->trapframe->t5  = p->userframe->t5;
  p->trapframe->t6  = p->userframe->t6;
  release(&p->lock);
  return 0;
}