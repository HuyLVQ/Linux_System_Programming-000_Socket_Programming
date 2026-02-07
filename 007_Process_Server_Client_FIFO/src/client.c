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

int main()
{
    char res[200];

    snprintf(res, 200, "[CLIENT] Hello from Client with PID %d\n", getpid());
    write(STDOUT_FILENO, res, strlen(res));

    int server_fifo_fd = open(SERVER_FIFO_PATH, O_WRONLY);
    sem_t *server_fifo_sema = sem_open(SERVER_SEMA_PATH, 0);

    char buf[10];
    ssize_t n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
    buf[n > 0 ? n : 0] = '\0';

    struct request send_request;
    send_request.pid = getpid();
    send_request.number = atoi(buf);

    printf("[CLIENT] Establishing connection ...\n");
    sem_wait(server_fifo_sema);
    write(server_fifo_fd, &send_request, sizeof(send_request));
    sem_post(server_fifo_sema);
    sleep(3);

    snprintf(res, CLIENT_SEMA_PATH_NAMESIZE, CLIENT_SEMA_PATH_TEMPLATE, getpid());
    sem_t *client_fifo_sema = sem_open(res, 0);

    snprintf(res, CLIENT_FIFO_PATH_NAMESIZE, CLIENT_FIFO_PATH_TEMPLATE, getpid());
    int client_fifo_fd = open(res, O_RDONLY);

    printf("[CLIENT] Connection established ...\n");

    for(;;)
    {
        printf("[CLIENT] Type in any number to send to the Server ...\n");
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
        buf[n > 0 ? n : 0] = '\0';
        read(STDIN_FILENO, buf, strlen(buf));

        struct request send_request;
        send_request.pid = getpid();
        send_request.number = atoi(buf);

        sem_wait(server_fifo_sema);
        write(server_fifo_fd, &send_request, sizeof(send_request));
        sem_post(server_fifo_sema);

        struct response rcv_response;
        sem_wait(client_fifo_sema);
        read(client_fifo_fd, &rcv_response, sizeof(rcv_response));
        sem_post(client_fifo_sema);

        
        char numbuf[200];
        snprintf(numbuf, 200, "[CLIENT] Server has sent: %d", rcv_response.number);
        write(STDOUT_FILENO, numbuf, strlen(numbuf));
    }
}