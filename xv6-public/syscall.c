#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "syscall.h"
#include "trace.h"

#define dump_max 10 // Buffer size
#define MAX_NAME_LEN 64

// #define MAX_BUFFER_SIZE 1638400  // or however large you want the buffer to be


char trace_buffer[MAX_BUFFER_SIZE];
int buffer_index = 0;

void itoa(int num, char* str) {
    int i = 0;
    int is_negative = 0;
    
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    while (num != 0) {
        int digit = num % 10;
        str[i++] = digit + '0';
        num = num / 10;
    }

    if (is_negative)
        str[i++] = '-';

    str[i] = '\0';

    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

void append_to_buffer(const char* str) {
    int len = strlen(str);

    // Ensure there is enough space in the buffer, leaving room for the null terminator
    if (buffer_index + len < MAX_BUFFER_SIZE - 1) {
        memmove(trace_buffer + buffer_index, str, len); // Copy the string content
        buffer_index += len;
        trace_buffer[buffer_index] = '\0'; // Null-terminate the buffer
    } else {
        cprintf("TRACE BUFFER FULL: Unable to log additional trace data.\n");
    }
}


void format_trace_message(char* trace_msg, int max_len, int pid, char* procName, char* syscall, int retval) {
   int len = max_len;
   
   safestrcpy(trace_msg, "\x1b[35mTRACE: pid = ", len);
   itoa(pid, trace_msg + strlen(trace_msg));
   safestrcpy(trace_msg + strlen(trace_msg), " | command name = ", len - strlen(trace_msg));
   safestrcpy(trace_msg + strlen(trace_msg), procName, len - strlen(trace_msg));
   safestrcpy(trace_msg + strlen(trace_msg), " | syscall = ", len - strlen(trace_msg));
   safestrcpy(trace_msg + strlen(trace_msg), syscall, len - strlen(trace_msg));
   if ( (strncmp(syscall, "exit", 4) != 0)){
    safestrcpy(trace_msg + strlen(trace_msg), " | return value = ", len - strlen(trace_msg));
   itoa(retval, trace_msg + strlen(trace_msg));
   }
   safestrcpy(trace_msg + strlen(trace_msg), "\e[0m\n", len - strlen(trace_msg));
}

extern int e_flag;
extern int s_flag;
extern int f_flag;

int e_flag = -1;;
int s_flag = 0;
int f_flag = 0;

// int record_index=0;
// int record_count=1;
struct trace_record {
    int pid;
    char name[MAX_NAME_LEN];   
    char syscall[MAX_NAME_LEN]; 
    int retval;
};

static struct trace_record record[dump_max];
int record_index = 0;  // Keep as is
int record_count = 0;  // Change from 1 to 0

void record_trace(int id, char* name, char* sys_call, int return_val) {
    // Save the record
    record[record_index].pid = id;
    safestrcpy(record[record_index].name, name, MAX_NAME_LEN);
    safestrcpy(record[record_index].syscall, sys_call, MAX_NAME_LEN);
    record[record_index].retval = return_val;
    
    // Update indices
    record_index = (record_index + 1) % dump_max;
    if(record_count < dump_max)
        record_count++;
}

void print_traced_records() {
    int i;
    int idx;
    
    // Calculate starting point to print from oldest to newest
    int start = (record_index - record_count + dump_max) % dump_max;
    
    for(i = 0; i < record_count; i++) {
        idx = (start + i) % dump_max;
        cprintf("\e[35mTRACE: pid = %d | process name = %s | syscall = %s | return val = %d\e[0m\n",
                record[idx].pid,
                record[idx].name,
                record[idx].syscall,
                record[idx].retval);
    }
}
// User code makes a system call with INT T_SYSCALL.
// System call number in %eax.
// Arguments on the stack, from the user call to the C
// library system call function. The saved user %esp points
// to a saved program counter, and then the first argument.

// Fetch the int at addr from the current process.
int
fetchint(uint addr, int *ip)
{
  if(addr >= proc->sz || addr+4 > proc->sz)
    return -1;
  *ip = *(int*)(addr);
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including nul.
int
fetchstr(uint addr, char **pp)
{
  char *s, *ep;

  if(addr >= proc->sz)
    return -1;
  *pp = (char*)addr;
  ep = (char*)proc->sz;
  for(s = *pp; s < ep; s++)
    if(*s == 0)
      return s - *pp;
  return -1;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  return fetchint(proc->tf->esp + 4 + 4*n, ip);
}

// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size bytes.  Check that the pointer
// lies within the process address space.
int
argptr(int n, char **pp, int size)
{
  int i;

  if(argint(n, &i) < 0)
    return -1;
  if(size < 0 || (uint)i >= proc->sz || (uint)i+size > proc->sz)
    return -1;
  *pp = (char*)i;
  return 0;
}

// Fetch the nth word-sized system call argument as a string pointer.
// Check that the pointer is valid and the string is nul-terminated.
// (There is no shared writable memory, so the string can't change
// between this check and being used by the kernel.)
int
argstr(int n, char **pp)
{
  int addr;
  if(argint(n, &addr) < 0)
    return -1;
  return fetchstr(addr, pp);
}

extern int sys_chdir(void);
extern int sys_close(void);
extern int sys_dup(void);
extern int sys_exec(void);
extern int sys_exit(void);
extern int sys_fork(void);
extern int sys_fstat(void);
extern int sys_getpid(void);
extern int sys_kill(void);
extern int sys_link(void);
extern int sys_mkdir(void);
extern int sys_mknod(void);
extern int sys_open(void);
extern int sys_pipe(void);
extern int sys_read(void);
extern int sys_sbrk(void);
extern int sys_sleep(void);
extern int sys_unlink(void);
extern int sys_wait(void);
extern int sys_write(void);
extern int sys_uptime(void);
extern int sys_trace(void);
extern int sys_tracedump(void);
extern int sys_setEFlag(void);
extern int sys_setSFlag(void);
extern int sys_setFFlag(void);
extern int sys_outputtrace(void);

int sys_tracedump() {
    print_traced_records();
    return 0;
}


static int (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
[SYS_trace]   sys_trace,
[SYS_tracedump] sys_tracedump,
[SYS_setEFlag] sys_setEFlag, 
[SYS_setSFlag] sys_setSFlag,
[SYS_setFFlag] sys_setFFlag,
[SYS_outputtrace]  sys_outputtrace,
};

static char* syscall_names[] = {
    [SYS_fork] "fork",
    [SYS_exit] "exit",
    [SYS_wait] "wait",
    [SYS_pipe] "pipe",
    [SYS_read] "read",
    [SYS_kill] "kill",
    [SYS_exec] "exec",
    [SYS_fstat] "fstat",
    [SYS_chdir] "chdir",
    [SYS_dup] "dup",
    [SYS_getpid] "getpid",
    [SYS_sbrk] "sbrk",
    [SYS_sleep] "sleep",
    [SYS_uptime] "uptime",
    [SYS_open] "open",
    [SYS_write] "write",
    [SYS_mknod] "mknod",
    [SYS_unlink] "unlink",
    [SYS_link] "link",
    [SYS_mkdir] "mkdir",
    [SYS_close] "close",
    [SYS_trace] "trace",
    [SYS_tracedump] "tracedump",
    [SYS_setEFlag] "setEFlag", 
    [SYS_setSFlag] "setSFlag",
    [SYS_setFFlag] "setFFlag",
    [SYS_outputtrace] "outputtrace",

};

void
syscall(void)
{
  int num;
  char procName[64];
  struct proc* curr_proc = proc;

  safestrcpy(procName, curr_proc->name, sizeof(procName));

  int traceOn = (curr_proc->trace & trace_enabled);

  num = proc->tf->eax;

if(e_flag == -1 || (e_flag == SYS_trace || e_flag == SYS_exit)) 
  {

    if(!((s_flag == 1 && proc->tf->eax < 0 ) || (f_flag == 1 && proc->tf->eax >= 0 ))) {

    if (num == SYS_trace) {
            char trace_msg[512];
          format_trace_message(trace_msg, sizeof(trace_msg), proc->pid, procName, syscall_names[num], curr_proc->trace);
          cprintf("%s", trace_msg);
          append_to_buffer(trace_msg);
     }
    else if ((num == SYS_exit) && traceOn) {

          char trace_msg[512];
          format_trace_message(trace_msg, sizeof(trace_msg), proc->pid, procName, syscall_names[num], proc->tf->eax);
          cprintf("%s", trace_msg);
          append_to_buffer(trace_msg);
        record_trace(curr_proc->pid, procName, syscall_names[num], 0);
        proc->tf->eax = syscalls[num]();
    }
    }}


  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    proc->tf->eax = syscalls[num]();

  if (traceOn){

    if(e_flag != -1) {

        if(num == e_flag+1) {

 
          if(!((s_flag == 1 && proc->tf->eax == -1 ) || (f_flag == 1 && proc->tf->eax != -1 ))) {


            char trace_msg[512];
          format_trace_message(trace_msg, sizeof(trace_msg), proc->pid, procName, syscall_names[num], proc->tf->eax);
          cprintf("%s", trace_msg);
          append_to_buffer(trace_msg);
          }
        }

    } else {
    if (s_flag == 1) {
      if(proc->tf->eax != -1) {
          char trace_msg[512];
          format_trace_message(trace_msg, sizeof(trace_msg), proc->pid, procName, syscall_names[num], proc->tf->eax);
          cprintf("%s", trace_msg);
          append_to_buffer(trace_msg);

        }
    }
    else if (f_flag == 1) {
      if(proc->tf->eax == -1) {

          char trace_msg[512];
          format_trace_message(trace_msg, sizeof(trace_msg), proc->pid, procName, syscall_names[num], proc->tf->eax);
          cprintf("%s", trace_msg);
          append_to_buffer(trace_msg);
        }
    }
    else {

        char trace_msg[512];
        format_trace_message(trace_msg, sizeof(trace_msg), proc->pid, procName, syscall_names[num], proc->tf->eax);
        cprintf("%s", trace_msg);
        append_to_buffer(trace_msg);
            }
    }
    }

  } else {
    cprintf("%d %s: unknown sys call %d\n",
            proc->pid, procName, num);
    proc->tf->eax = -1;
  }

  if(traceOn || num == SYS_trace)
{  
  record_trace(curr_proc->pid,procName,syscall_names[num],curr_proc->tf->eax);
}
}

int sys_setEFlag(void) {
    argint(0, &e_flag);
    return 0;
}

int sys_setSFlag(void) {
    argint(0, &s_flag);
    return 0;
}

int sys_setFFlag(void) {
    argint(0, &f_flag);
    return 0;
}