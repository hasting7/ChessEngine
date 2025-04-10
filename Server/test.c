#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#define NUM_CHILDREN 16

void process_task(int child_num, int write_fd) {
    // Do some work (simulated here by simply returning the child number)
    int result = child_num * 10;  // Example result of processing
    write(write_fd, &result, sizeof(result));  // Send result to parent via pipe
    exit(0);  // End child process
}

int main() {
    int pipe_fd[2];
    pipe(pipe_fd);  // Create the pipe

    pid_t pid;
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid = fork();
        if (pid == 0) {
            // Child process: perform task and send result to the parent
            close(pipe_fd[0]);  // Close read end of the pipe
            process_task(i, pipe_fd[1]);  // Perform task and write result
        }
    }

    // Parent process: read results from the pipe
    close(pipe_fd[1]);  // Close write end of the pipe
    for (int i = 0; i < NUM_CHILDREN; i++) {
        int result;
        read(pipe_fd[0], &result, sizeof(result));  // Read result from the pipe
        printf("Received result from child %d: %d\n", i, result);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < NUM_CHILDREN; i++) {
        wait(NULL);  // Wait for each child process to finish
    }

    close(pipe_fd[0]);  // Close the read end of the pipe
    return 0;
}

