#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <sys/wait.h>
#include <sys/timerfd.h>

sig_atomic_t got_checkin = false;
pid_t child_pid;
int status;

void watchdog_handler(int sig, siginfo_t *si, void *ucontext)
{   
    if(sig == SIGUSR1)
    {
        if(si->si_pid == child_pid)
        {
            got_checkin = true;
            char *buf = "Child has locked in!\n";
            write(STDOUT_FILENO, buf, strlen(buf));
        }
    } 
    else if (sig == SIGALRM)
    {

        if(kill(child_pid, 0) == -1) 
        {
            write(STDOUT_FILENO, "Child is gone. Parent exiting.\n", 31);
            exit(EXIT_SUCCESS);
        }

        if(!got_checkin)
        {
            char *buf = "Child hasn't locked in -- Killing the child!\n";
            write(STDOUT_FILENO, buf, strlen(buf));
            kill(child_pid, SIGKILL);
        }
        got_checkin = false;
        alarm(1);

        
    }
}



int main()
{
    struct sigaction sa_watchdog = {0};
    sigemptyset(&sa_watchdog.sa_mask);
    sa_watchdog.sa_sigaction = watchdog_handler;
    sa_watchdog.sa_flags = SA_SIGINFO;

    sigaction(SIGALRM, &sa_watchdog, NULL);
    sigaction(SIGUSR1, &sa_watchdog, NULL);

    signal(SIGCHLD, SIG_IGN);

    char *buf;

    switch(child_pid = fork())
    {
        case -1:
            perror("Failed to create any child\n");
            exit(EXIT_FAILURE);
        case 0:
            buf = "[CHILD]    Child created!\n";
            write(STDOUT_FILENO, buf, strlen(buf));
            while(1)
            {
                // buf = "[CHILD]    Child still says Hello!\n";
                // write(STDOUT_FILENO, buf, strlen(buf));
                kill(getppid(), SIGUSR1);
                usleep(100000);
            }
            exit(EXIT_SUCCESS);

        default:
            sprintf(buf, "[PARENT]    Currently monitoring children process of PID %d!\n", child_pid);
            write(STDOUT_FILENO, buf, strlen(buf));

            alarm(1);
            while(1)
            {
                pause();
            }
            exit(EXIT_SUCCESS);

    }
    

    exit(EXIT_SUCCESS);
}