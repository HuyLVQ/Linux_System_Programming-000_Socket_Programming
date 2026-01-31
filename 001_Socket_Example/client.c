#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>

const char *SOCKNAME = "/tmp/server_sock";          // The Socket file path's parent directory must be existed - here is /tmp/
struct sockaddr_un server_addr;
int sfd;
int num_read;
char buf[100];

void client_setup()
{
    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sfd == -1)
    {
        perror("Server Instance Creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_un));                        // Reset all member of the Socket Address
    server_addr.sun_family = AF_UNIX;                                           // Specify using UNIX Domain
    strncpy(server_addr.sun_path, SOCKNAME, sizeof(server_addr.sun_path) - 1);  // Define the Socket file path

    if(connect(sfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) == -1)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
}

void server_handle(void)
{
    while((num_read = read(STDIN_FILENO, buf, 100)) > 0)
    {
        write(sfd, buf, num_read);
    }
}

int main(void)
{
    printf("Connecting to Server");
    client_setup();
    server_handle();
    exit(EXIT_SUCCESS);
    return 0;
}