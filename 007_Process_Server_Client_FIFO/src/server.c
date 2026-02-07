#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/signal.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "fifoseq_num.h"


int initialize_server_client_fifo_and_sema(pid_t pid, int* _client_fifo_fd, sem_t** sema_list, int* _established_client_pid_list, int *pid_order)
{
    char result[200];
    snprintf(result, CLIENT_FIFO_PATH_NAMESIZE, CLIENT_FIFO_PATH_TEMPLATE, pid);
    if(mkfifo(result, 0666) == -1  && errno != EEXIST)
    {
        snprintf(result, 200, "[SERVER] Could not create FIFO for %d", pid);
        perror(result);
        return -1;
    }

    _client_fifo_fd[*pid_order] = open(result, O_WRONLY | O_NONBLOCK);
    _established_client_pid_list[*pid_order] = pid;

    snprintf(result, CLIENT_SEMA_PATH_NAMESIZE, CLIENT_SEMA_PATH_TEMPLATE, pid);
    sema_list[*pid_order] = sem_open(result, O_CREAT, 0666, 1);

    snprintf(result, 200, "[SERVER] Established FIFO for %d", pid);
    write(STDOUT_FILENO, result, strlen(result));

    snprintf(result, 200, "[SERVER] Established Semaphore for %d", pid);
    write(STDOUT_FILENO, result, strlen(result));

    (*pid_order) ++;
    return 0;
}

bool is_present(int arr[], int size, int value) {
    for (int i = 0; i < size; i++) {
        if (arr[i] == value) {
            return true;
        }
    }
    return false;
}

int which_index(int arr[], int size, int value) {
    for (int i = 0; i < size; i++) {
        if (arr[i] == value) {
            return i;
        }
    }
    return -1;
}

// void clean_up_handler(int sig, siginfo_t *siginfo, void *ucontext)
// {
//     char res[200];
//     int client_pid = siginfo.si_pid;

//     snprintf(res, CLIENT_SEMA_PATH_NAMESIZE, CLIENT_SEMA_PATH_TEMPLATE, client_pid);
//     sem_unlink(res);

//     snprintf(res, CLIENT_FIFO_PATH_NAMESIZE, CLIENT_FIFO_PATH_TEMPLATE, client_pid);
//     unlink(res);

//     kill(client_pid, SIGKILL);
// }

int main(int argc, char **argv)
{
    char res[100];
    int server_listening_port;
    int epoll_fd;
    struct epoll_event ev;
    struct epoll_event evlist[1];
    struct sigaction sa = {0};


    // sigemptyset(&sa.sa_mask);
    // sa.sa_flags = SA_SIGINFO;
    // sa.sa_sigaction = clean_up_handler;
    // sigaction(SIGUSR1, &sa, NULL);


    int CHILD_NUM = atoi(argv[1]);

    char client_fifo_list[CHILD_NUM][CLIENT_FIFO_PATH_NAMESIZE];
    int client_fifo_fd[CHILD_NUM];

    int *established_client_pid_list = (int*)calloc(CHILD_NUM, sizeof(int));
    int client_pid_idx = 0;

    sem_t *server_semaphore = sem_open(SERVER_SEMA_PATH, O_CREAT, 0666, 1);
    sem_t *client_semaphore_list[CHILD_NUM];

    if (mkfifo(SERVER_FIFO_PATH, 0666) == -1)
    {
        if (errno != EEXIST)
        {
            perror("[SERVER] mkfifo failed");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        printf("[SERVER] Listening FIFO created\n");
    }


    server_listening_port = open(SERVER_FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (server_listening_port == -1)
    {
        perror("[SERVER] Could not open Listening Port\n");
        exit(EXIT_FAILURE);
    }

    
    
    epoll_fd = epoll_create(1);
    ev.events = EPOLLIN;
    ev.data.fd = server_listening_port;

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_listening_port, &ev) == -1)
    {
        perror("[SERVER] Failed to add fifo_fd to epoll\n");
        exit(EXIT_FAILURE);
    }

    for(;;)
    {
        char *buf = "[SERVER] Waiting for incoming requests...\n";
        write(STDOUT_FILENO, buf, strlen(buf));

        epoll_wait(epoll_fd, evlist, 1, -1);

        sem_wait(server_semaphore);

        struct request received_request;
        read(server_listening_port, &received_request, sizeof(received_request));

        sem_post(server_semaphore);

        int index = which_index(established_client_pid_list, CHILD_NUM, received_request.pid);
        
        if(is_present(established_client_pid_list, CHILD_NUM, received_request.pid) == false)
        {
            char* template = "[SERVER] New connection detected, client PID: %d\n";
            snprintf(res, 100, template, received_request.pid);
            write(STDOUT_FILENO, res, strlen(res));

            initialize_server_client_fifo_and_sema(received_request.pid, client_fifo_fd, client_semaphore_list, established_client_pid_list, &client_pid_idx);            
        }
        else 
        {

            char* template = "[SERVER] Received request from children %dth\n";
            snprintf(res, 100, template, index);
            write(STDOUT_FILENO, res, strlen(res));
            
            int rcv_num = received_request.number + 1;

            sem_wait(client_semaphore_list[index]);
            write(client_fifo_fd[index], &rcv_num, sizeof(int));
            sem_post(client_semaphore_list[index]);
        }
    }

    free(established_client_pid_list);
    exit(EXIT_SUCCESS);
}

