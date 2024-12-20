#define trace_enabled 1
#define trace_disabled 0
#define trace_fork 2

#define MAX_BUFFER_SIZE 36384
extern char trace_buffer[MAX_BUFFER_SIZE];
extern int buffer_index;