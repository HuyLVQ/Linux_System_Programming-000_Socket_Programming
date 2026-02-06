#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include <sys/signal.h>
#include <sys/epoll.h>

#define SIG_CHECKIN (SIGRTMIN + 1)
#define MAX_CHILD 100
#define WATCHDOG_PRED_S 2
#define CHILD_PROCESS_PRED_US 50000

/// Spawns multiple child, the number of child is specified by user input
/// All the child Process are active only all of them are succesfully initialized to ensure a synchronized period across all the children.---> Use SIGUSR2
/// Monitor them periodically by using SIGALARM, check every 1s. The child process always send the 'active signal' SIG_CHECKIN (a real-time signal as it is queueable - not a single event missed) every 0.5s to check in.
/// Restarts them if they crash --> Send the SIGCONT to the child processes who send SIGCHLD
/// Can pause / resume / terminate the desired processes by typing "-pause <PID order> // -resume <PID order> // -terminate <PID order>"


pid_t child_pid[MAX_CHILD];
int child_num;
volatile sig_atomic_t *has_checked_in;
volatile sig_atomic_t last_checkin_pid = -1;
volatile sig_atomic_t sigalrm_flag = false;


int find_the_corresponding_order(pid_t pid)
{   
    int i = 0;
    for(i = 0; i < child_num; i++)
    {
        if(pid == child_pid[i])
        {
            break;
        }
    }
    return i;
}



void signal_handler(int sig, siginfo_t *siginfo, void *ucontext)
{
    if (sig == SIGALRM)
    {
        sigalrm_flag = true;
    }
    else if(sig == SIG_CHECKIN)
    {
        last_checkin_pid = siginfo->si_pid;
        int idx = find_the_corresponding_order(last_checkin_pid);
        if (idx >= 0)
            has_checked_in[idx] = 1;
        last_checkin_pid = -1;
    }
}

int main(int argc, char **argv)
{
    child_num = atoi(argv[1]);
    has_checked_in = (sig_atomic_t*)calloc(child_num, sizeof(sig_atomic_t));

    struct sigaction sa = {0};
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = signal_handler;

    sigset_t prev_mask, int_mask;
    sigemptyset(&int_mask);
    sigaddset(&int_mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &int_mask, &prev_mask);

    if(sigaction(SIGALRM, &sa, NULL) == -1)
    {
        perror("Failed to modify handler of SIGALARM\n");
    }

    if(sigaction(SIG_CHECKIN, &sa, NULL) == -1)
    {
        perror("Failed to modify handler of SIGUSR1\n");
    }

    if(sigaction(SIGUSR2, &sa, NULL) == -1)
    {
        perror("Failed to modify handler of SIGUSR2\n");
    }

    signal(SIGCHLD, SIG_IGN);

    pid_t temp;

    for(int i = 0; i < child_num; i++)
    {
        switch(temp = fork())
        {
        case -1:
            exit(EXIT_FAILURE);

        case 0:
            printf("Child order %d with PID %d waiting for start signal\n", i, getpid());

            if(sigsuspend(&prev_mask) != -1)
            {
                perror("Suspend Mask\n");
            }

            printf("Child order %d with PID %d has startedl\n", i, getpid());
            
            union sigval val;
            val.sival_int = i;

            for(;;)
            {
                sigqueue(getppid(), SIG_CHECKIN, val);
                usleep(CHILD_PROCESS_PRED_US);
            }
            exit(EXIT_SUCCESS);

        default:
            child_pid[i] = temp;
            
            if(i == (child_num - 1))
            {
                for(int i = 0; i < child_num; i++)
                {
                    kill(child_pid[i], SIGUSR2);
                }

                alarm(WATCHDOG_PRED_S);
            }
        }
    }

    for(;;)
    {
        // if (last_checkin_pid != -1) {
        //     int idx = find_the_corresponding_order(last_checkin_pid);
        //     if (idx >= 0)
        //         has_checked_in[idx] = 1;
        //     last_checkin_pid = -1;
        // }

        if (sigalrm_flag) {
            for (int i = 0; i < child_num; i++) {
                if (!has_checked_in[i]) {
                    printf("Watchdog: child order %d (PID %d) missed heartbeat\n",
                        i, child_pid[i]);
                } else {
                    printf("Watchdog: child order %d (PID %d) checked in\n",
                        i, child_pid[i]);
                }
                has_checked_in[i] = 0;
            }
            alarm(WATCHDOG_PRED_S);
            sigalrm_flag = 0;
        }
        // usleep(100000);

        while (!sigalrm_flag && last_checkin_pid == -1)
            pause();
    }
    exit(EXIT_SUCCESS);
}