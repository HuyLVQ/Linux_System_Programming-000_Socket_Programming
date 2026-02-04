#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>

void set_up_signal_handler();
void child_action(int sig);
void parent_action(int sig);

typedef struct {
    int value;
    bool has_data;
} SharedData;

SharedData *shared;
pid_t child_pid;

void child_handler(int sig) {
    // We just set a flag or do minimal work
    // Handlers should strictly take one 'int' argument
    char* buf = "[CHILD] Has received a Signal\n";
    write(STDOUT_FILENO, buf, strlen(buf));
}

void parent_handler(int sig) {
    // Acknowledgement handler
    char* buf = "[PARENT]    Has received a Signal\n";
    write(STDOUT_FILENO, buf, strlen(buf));
}  

int main() {
    // Setup Shared Memory
    shared = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, 
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    shared->has_data = false;

    struct sigaction sa_child = {0};
    sa_child.sa_handler = child_handler;
    sigemptyset(&sa_child.sa_mask);
    sigaction(SIGUSR2, &sa_child, NULL); 

    struct sigaction sa_parent = {0};
    sa_parent.sa_handler = parent_handler;
    sigemptyset(&sa_parent.sa_mask);
    sigaction(SIGUSR1, &sa_parent, NULL); 

    signal(SIGCHLD, SIG_IGN); 

    child_pid = fork();

    if (child_pid == 0) {
        // CHILD PROCESS
        while(1) {
            pause(); // Wait for signal from parent
            if (shared->has_data) {
                printf("[CHILD] Read: %d\n", shared->value);
                shared->has_data = false;
                kill(getppid(), SIGUSR1); // Tell parent we're done
            }
        }
    } else {
        // PARENT PROCESS
        char buf[100]; // Fixed from char *buf[100]
        while(1) {
            printf("[PARENT]    Enter a number: ");
            if (fgets(buf, sizeof(buf), stdin)) {
                shared->value = atoi(buf);
                shared->has_data = true;
                kill(child_pid, SIGUSR2); // Tell child data is ready
                pause(); // Wait for child to finish processing
            }
        }
    }
    return 0;
}