#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>

const char *SOCKNAME = "/tmp/server_sock";          // The Socket file path's parent directory must be existed - here is /tmp/
int sfd, cfd;                                       // PlaceHolder for the intended Server and Client Socket
struct sockaddr_un server_addr;                     // PlaceHolder for the Socket address
ssize_t num_read;
char buf[100];


void setup_server(void)
{   
    sfd = socket(AF_UNIX, SOCK_STREAM, 0);          // Creating a Stream Socket (TCP) using UNIX Domain
    if (sfd == -1)                                  // Return if the Creation failed
    {
        perror("Socket Failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for client to bind");
    
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


void handle_client(void)
{
    if (listen(sfd, 5) == -1)                               // Listen to the Connection Inquiry from Client
    {
        perror("Listen Failed");
        exit(EXIT_FAILURE);
    }
    
    // --- Loop indefinitely --- //
    for(;;)
    {
        cfd = accept(sfd, NULL, NULL);                      // Accept the Connection Inquiry and create the corresponding Client Socket Instance
        if(cfd == -1)
        {
            perror("Acception failed");
        }

        while((num_read = read(cfd, buf, 100)) > 0)         // Print the received messages from Client Socket Instance to STDOUT
        {
            write(STDOUT_FILENO, buf, num_read);
        }
    }

}


void exit_server(void)
{
    remove(SOCKNAME);
}

int main()
{
    printf("Begin of Server\n ===========\n");
    setup_server();
    handle_client();
    exit_server();
    return 0;
}