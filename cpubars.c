#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef COL_HIGH
#define COL_HIGH "orange"
#endif
#ifndef COL_MAX
#define COL_MAX "red"
#endif

#ifdef NOPANGO
static const char* bar_chars[] = {"▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
#else
static const char* bar_chars[] = {"▁", "▂", "▃", "▄", "▅", "▆", "<span foreground=\""COL_HIGH"\">▇</span>", "<span foreground=\""COL_MAX"\">█</span>"};
#endif
static const int bar_chars_count = 8;
static const char* stat_path = "/proc/stat";
static int volatile run = 1;

struct cpu_time{
    unsigned long long total;
    unsigned long long busy;
};

struct jiffies{
    unsigned long long usr;
    unsigned long long nic;
    unsigned long long sys;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
};

void sighandler(int sig)
{
    switch(sig)
    {
        case SIGINT:
        case SIGKILL:
        case SIGABRT:
            run = 0;
        default:
            break;
    }
}

//parse a cpu line from /proc/stat and calc total, busy jiffies
void parse_line(const char* line, struct cpu_time* out_time)
{
    static struct jiffies jif;
    memset(&jif, 0, sizeof(struct jiffies));
    sscanf(line, "cp%*s %llu %llu %llu %llu %llu %llu %llu %llu", &jif.usr, &jif.nic, &jif.sys, &jif.idle, &jif.iowait, &jif.irq, &jif.softirq, &jif.steal);

    out_time->total = jif.usr + jif.nic + jif.sys + jif.idle + jif.iowait + jif.irq + jif.softirq + jif.steal;
    out_time->busy = out_time->total - jif.idle - jif.iowait;
}

int get_cpu_times(struct cpu_time* out_times)
{
    FILE* file = fopen(stat_path, "rb");
    static int count = 0;
    static char buf[256];
    while (fgets(buf, sizeof(buf), file) != 0)
    {
        if (strncmp("cpu", buf, 3) == 0)
        {
            if (out_times)
                parse_line(buf, out_times++);
            ++count;
        }
    }
    fclose(file);
    return count;
}

int main(int argc, char** argv)
{
    int sleep_sec = 1;
    if (argc > 1)
        sleep_sec = atoi(argv[1]);

    int cpu_count = get_cpu_times(0);
    const int cpu_time_size = sizeof(struct cpu_time)*cpu_count;
    struct cpu_time* curr = malloc(cpu_time_size);
    struct cpu_time* last = malloc(cpu_time_size);
    memset(last, 0, cpu_time_size);

    int overall_perc = 0;

    signal(SIGINT, sighandler);
    int i;
    int bar_char_idx;
    while (run)
    {
        get_cpu_times(curr);
        printf("%llu%% ", ((curr[0].busy - last[0].busy) * 100)/(curr[0].total - last[0].total));
        for (i=1; i<cpu_count; ++i)
        {
            bar_char_idx = ((curr[i].busy - last[i].busy) * (bar_chars_count-1))/(curr[i].total - last[i].total);
            printf("%s", bar_chars[bar_char_idx]);
        }
        printf("\n");
        fflush(stdout);

        memcpy(last, curr, cpu_time_size);
        sleep(sleep_sec);
    }

    free(curr);
    free(last);

    return 0;
}
