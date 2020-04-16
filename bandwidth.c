#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

const char* range_prefixes = "BKMGTPE";
int volatile run = 1;

void sighandler(int sig)
{
    run = 0;
}

int read_bytes(const char* path)
{
    static char buf[128];
    static long filesize;

    FILE* file = fopen(path, "rb");
    if (!file)
        return -1;
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    fread(buf, 1, filesize, file);
    fclose(file);
    return atoi(buf);
}

int is_up(const char* path)
{
    static char first_char;
    FILE* file = fopen(path, "rb");
    if (!file)
        return -1;
    fread(&first_char, 1, 1, file);
    fclose(file);
    return first_char == 'u';
}

int auto_range(unsigned int* value)
{
    int range = 0;
    while (*value > 1024)
    {
        *value >>= 10;
        ++range;
    }
    return range;
}


int main(int argc, char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s interface [sleep]", argv[0]);
        return 1;
    }

    const char* interface = argv[1];
    int sleep_sec = 5;
    if (argc > 2)
        sleep_sec = atoi(argv[2]);

    char oper_path[256];
    snprintf(oper_path, sizeof(oper_path), "/sys/class/net/%s/operstate", interface);

    char stat_path[256];
    snprintf(stat_path, sizeof(stat_path), "/sys/class/net/%s/statistics/_x_bytes", interface);
    int rt_pos = strlen(stat_path) - 8;

    signal(SIGINT, sighandler);
    signal(SIGKILL, sighandler);
    signal(SIGABRT, sighandler);

    unsigned int last_rx = 0, last_tx = 0;
    unsigned int curr_rx = 0, curr_tx = 0;
    unsigned int rx = 0, tx = 0;
    unsigned int rx_range = 0, tx_range = 0;
    int up = 0;
    while (run)
    {
        up = is_up(oper_path);
        if (up == -1)
        {
            goto not_found;
        }
        else if (up == 0)
        {
            printf("%s down\n", interface);
            fflush(stdout);
            goto cont;
        }

        stat_path[rt_pos] = 'r';
        curr_rx = read_bytes(stat_path);
        stat_path[rt_pos] = 't';
        curr_tx = read_bytes(stat_path);
        if (curr_rx == -1 || curr_tx == -1)
            goto not_found;

        rx = (curr_rx - last_rx)/sleep_sec;
        tx = (curr_tx - last_tx)/sleep_sec;

        rx_range = auto_range(&rx);
        tx_range = auto_range(&tx);

        printf("↓%d%c ↑%d%c\n", rx, range_prefixes[rx_range], tx, range_prefixes[tx_range]);
        fflush(stdout);

        last_rx = curr_rx;
        last_tx = curr_tx;

    cont:
        sleep(sleep_sec);
    }

    return 0;

not_found:
    fprintf(stderr, "%s not found\n", interface);
    return 2;
}
