#include <stdio.h>
#include <unistd.h>
int main(void)
{
    puts("Phase 15 sleeper started; waiting for SIGTERM.");
    (void)sleep(60000);
    puts("Phase 15 sleeper woke naturally.");
    return 0;
}
