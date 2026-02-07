#include <sys/types.h>
#include <sys/stat.h>

#define SERVER_FIFO_PATH "/tmp/server_fifo"
#define CLIENT_FIFO_PATH_TEMPLATE "/tmp/client_fifo.%d"
#define CLIENT_FIFO_PATH_NAMESIZE (sizeof(CLIENT_FIFO_PATH_TEMPLATE) + 20)

#define SERVER_SEMA_PATH "/server"
#define CLIENT_SEMA_PATH_TEMPLATE "/sema_%d"
#define CLIENT_SEMA_PATH_NAMESIZE (sizeof(CLIENT_SEMA_PATH_TEMPLATE) + 20)

struct request
{
    pid_t pid;
    int number;
};

struct response
{
    int number;
};