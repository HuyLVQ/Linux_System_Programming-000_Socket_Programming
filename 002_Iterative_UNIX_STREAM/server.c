#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <signal.h>

const char *SOCKNAME = "/tmp/concurrent_server";
struct sockaddr_un server_addr;
char buf[100];
ssize_t num_read;
int sfd, cfd;
static int child_count = 1;


void server_setup()
{
    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1)
    {
        perror("Server Socket Creation failed!");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for client to bind\n");
    
    if (remove(SOCKNAME) == -1 && errno != ENOENT)              // Remove the occupied SOCKNAME
    {
        perror("Removed failed");
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_un));                        // Reset all member of the Socket Address
    server_addr.sun_family = AF_UNIX;                                           // Specify using UNIX Domain
    strncpy(server_addr.sun_path, SOCKNAME, sizeof(server_addr.sun_path) - 1);  // Define the Socket file path

    if(bind(sfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_un)) == -1)   // Return if binding failed
    {
        perror("Binding Failed");
        exit(EXIT_FAILURE);
    }
}

static void server_handle_clients(void)
{
    if (listen(sfd, 5) == -1)                               // Listen to the Connection Inquiry from Client
    {
        perror("Listen Failed");
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, SIG_IGN);

    for(;;)
    {
        cfd = accept(sfd, NULL, NULL);
        if(cfd == -1)
        {
            perror("Acception failed");
        }

        child_count += 1;

        switch(fork())
        {
            case -1:
                perror("Could not Create Child");
                close(cfd);
                break;
            case 0:
                printf("Child Created!\n");
                close(sfd);

                while((num_read = read(cfd, buf, 100)) > 0)
                {
                    char count_str[32];
                    int len = sprintf(count_str, "Client #%d says: ", child_count);
                    write(STDOUT_FILENO, count_str, len);
                    write(STDOUT_FILENO, buf, num_read);
                }
                
                close(cfd);
                exit(EXIT_SUCCESS);

            default:
                close(cfd);
                break;

        }
    }
}

int main()
{
    printf("---Server Begin---\n");
    server_setup();
    server_handle_clients();
    return 0;
}