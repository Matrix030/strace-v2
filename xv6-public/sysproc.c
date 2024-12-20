#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"
#include "trace.h"  // For trace_buffer
#include "stat.h"   // For T_FILE


int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_trace(void)
{
  int trace_arg;
  argint(0, &trace_arg);

  trace(trace_arg);

  return 0;
}


extern int sys_open(void);
extern int sys_write(void);
extern int sys_close(void);

int
sys_outputtrace(void)
{
  char *user_buf;
  int buf_size;

  // Get user-space buffer pointer and size
  if (argptr(0, &user_buf, sizeof(char *)) < 0 || argint(1, &buf_size) < 0)
    return -1;

  // Check if the provided buffer size is sufficient
  if (buf_size < strlen(trace_buffer) + 1) // +1 for null terminator
    return -1;

  // Copy the trace buffer content to user space
  if (copyout(proc->pgdir, (uint)user_buf, trace_buffer, strlen(trace_buffer) + 1) < 0)
    return -1;

  return 0;
}
