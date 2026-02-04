#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sys/signal.h>
#include <sys/timerfd.h>


void children_task(char* buf)
{
    write(STDOUT_FILENO, buf, strlen(buf));
}