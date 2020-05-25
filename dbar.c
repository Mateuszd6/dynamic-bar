#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#include "util.h"

typedef enum
{
    E_SCRIPT,
    E_BUILTIN,
} entry_t;

typedef enum
{
    ET_SLOW,
    ET_FAST,
} exectime_t;

typedef enum
{
    B_MEM,
    B_TIMEDATE,
    B_CPU,
    B_TEMP,
} builtin_t;

typedef struct
{
    char const* command; // In case of builtin type command is a fmt string.
    entry_t type;
    builtin_t builtin;
    int refresh;
    exectime_t etime;
} bar_entry;

typedef struct
{
    int status; // 0 on success, otherwise error and output is not set!
    char output[124]; // Any reasonable output for status bar should fit.
} exec_script_ret;

#define SCRIPT(CMD) CMD, E_SCRIPT, 0
#define BUILTIN(BI, FMT) FMT, E_BUILTIN, BI

#include "config.h"

#include "cpu.h"
#include "mem.h"
#include "temp.h"

// bar_entry is used for configs, entry has just more stuff and is used by the
// program.
static int sec_to_refresh[ARRAY_COUNT(entries)];

#define NENTRIES (ARRAY_COUNT(entries))
static exec_script_ret g_bar[NENTRIES];

static exec_script_ret
exec_script(char const* command)
{
    int link[2];
    pid_t pid;
    exec_script_ret retval = {0};

    if (pipe(link) == -1)
        die("pipe");

    if ((pid = fork()) == -1)
        die("fork");

    if (pid == 0)
    {
        dup2(link[1], STDOUT_FILENO);
        close(link[0]);
        close(link[1]);

        execl("/bin/sh", "sh", "-c", command, (char *)0);
        die("execl");
    }
    else
    {
        close(link[1]);

    try_read:
        {
            int nbytes = read(link[0], retval.output, 124 - 1);
            if (nbytes == -1)
            {
                // It's highly possible that we got an interrupt here, so retry.
                if (errno == EINTR)
                    goto try_read;

                // TODO: Handle other kind of error?
            }

            retval.output[nbytes] = 0;
        }

        char* newline = strchr(retval.output, '\n');
        if (newline)
            *newline = 0;

        wait(0);
    }

    return retval;
}

static exec_script_ret
eval_entry(bar_entry const* entry)
{
    switch(entry->type)
    {
    case E_SCRIPT:
        return exec_script(entry->command);

    case E_BUILTIN:
    {
        exec_script_ret retval = {0};
        strcat(retval.output, "NOT IMPLEMENTED");
        switch(entry->builtin)
        {
        case B_MEM:
        {
            meminfo mi = mem_usage();
            sprintf(retval.output, entry->command, get_free_mem(mi));
        } break;

        case B_CPU:
        {
            sprintf(retval.output, entry->command, get_cpu_usage() * 100);
        } break;

        case B_TEMP:
        {
            sprintf(retval.output, entry->command, get_temp());
        } break;

        case B_TIMEDATE:
        {
            time_t current_time = time(0);
            if (current_time == ((time_t)-1))
                die("time");

            struct tm t;
            localtime_r(&current_time, &t);

            char const* weekdays[] = {
                "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
            };

            char const* months[] = {
                "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
            };

            char buf[128];
            sprintf(buf, "%s %d %s %.2d:%.2d",
                    weekdays[t.tm_wday], t.tm_mday,
                    months[t.tm_mon], t.tm_hour, t.tm_min);
            sprintf(retval.output, entry->command, buf);
        } break;

        default:
            die("default");
            break;
        }

        return retval;
    }

    default:
        die("default");
    }

    // NOTREACHED
    exec_script_ret rd = {0};
    return rd;
}

static void
update_xroot()
{
    char buffer[4096]; // TODO: Out of range!
    buffer[0] = 0;
    strcat(buffer, "xsetroot -name '" BAR_BEFORE_FIRST);
    for (int i = NENTRIES - 1; i >= 0; --i)
    {
        if (g_bar[i].output[0] == 0)
            continue;

        strcat(buffer, g_bar[i].output);
        if (i != 0)
            strcat(buffer, BAR_SEPARATOR);
    }
    strcat(buffer, BAR_AFTER_LAST "'");
    exec_script(buffer);
}

static volatile sig_atomic_t got_sigusr1 = 0;
static volatile sig_atomic_t got_sigusr2 = 0;

static void
handle_sigusr1(int sig)
{
    (void)sig;
    got_sigusr1 = 1;
}

static void
handle_sigusr2(int sig)
{
    (void)sig;
    got_sigusr2 = 1;
}

int
main()
{
    struct sigaction sa1;
    sa1.sa_handler = handle_sigusr1;
    sigemptyset(&(sa1.sa_mask));
    sigaddset(&(sa1.sa_mask), SIGUSR1);
    sigaction(SIGUSR1, &sa1, NULL);

    struct sigaction sa2;
    sa2.sa_handler = handle_sigusr2;
    sigemptyset(&(sa2.sa_mask));
    sigaddset(&(sa2.sa_mask), SIGUSR2);
    sigaction(SIGUSR2, &sa2, NULL);

    for (int i = 0; i < NENTRIES; ++i)
    {
        // Init bar; set all strings to 0 and statuses to OK.
        sec_to_refresh[i] = 0;
        g_bar[i].status = 0;
        g_bar[i].output[0] = 0;
    }

    int last_slept = 0;
    int to_sleep_default = 3600; // To limit sleep time.
    int to_sleep = to_sleep_default;
    for (;;)
    {
        char needs_update[ARRAY_COUNT(entries)];
        memset(needs_update, 0, sizeof(char) * ARRAY_COUNT(entries));

        for (int i = 0; i < NENTRIES; ++i)
        {
            sec_to_refresh[i] -= last_slept;
            if (sec_to_refresh[i] <= 0)
            {
                sec_to_refresh[i] = entries[i].refresh;
                needs_update[i] = 1;
            }

            if (sec_to_refresh[i] < to_sleep)
                to_sleep = sec_to_refresh[i];
        }

        // Update only 'fast', and output them at once.
        for (int i = 0; i < NENTRIES; ++i)
            if (needs_update[i] && entries[i].etime == ET_FAST)
                g_bar[i] = eval_entry(entries + i);
        update_xroot();

        // Now update 'slow' entries. They may take a while, so it's worth to
        // update bar after each one.
        for (int i = 0; i < NENTRIES; ++i)
            if (needs_update[i] && entries[i].etime != ET_FAST)
            {
                g_bar[i] = eval_entry(entries + i);
                update_xroot();
            }

        got_sigusr1 = 0;
        got_sigusr2 = 0;
        sleep(to_sleep);
        if (got_sigusr1) // SIGUSR1 - refresh all
        {
            for (int i = 0; i < NENTRIES; ++i)
                sec_to_refresh[i] = 0;

            last_slept = 0;
            to_sleep = to_sleep_default;
            continue;
        }
        else if (got_sigusr2) // SIGUSR2 - refresh only fast onces.
        {
            for (int i = 0; i < NENTRIES; ++i)
                if (entries[i].etime == ET_FAST)
                    sec_to_refresh[i] = 0;

            last_slept = 0;
            to_sleep = to_sleep_default;
            continue;
        }

        last_slept = to_sleep;
        to_sleep = to_sleep_default;
    }

    return 0;
}
