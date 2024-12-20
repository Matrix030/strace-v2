#include "types.h"
#include "stat.h"
#include "user.h"

int main() {
    printf(1, "Parent: Starting program\n");

    int parent_pid = getpid(); // Get the parent process PID
    int i; // Declare the loop variable outside the for loop

    for (i = 0; i < 4; i++) {
        int pid = fork();

        if (pid < 0) {
            // Fork failed
            printf(1, "Fork failed at iteration %d\n", i);
            exit();
        } else if (pid == 0) {
            // Child process
            printf(1, "Child %d: My PID: %d, Parent PID: %d\n", i + 1, getpid(), parent_pid, "\n");
            exit(); // Exit child process to avoid creating more children
        }
        // Parent process continues the loop to create more children
    }

    // Wait for all child processes to finish
    for (i = 0; i < 4; i++) {
        wait();
    }

    printf(1, "Parent: All child processes finished. My PID: %d\n", parent_pid);
    exit();
}
