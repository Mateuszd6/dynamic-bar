#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// TODO: Replace die!
#define die(e) do { fprintf(stderr, "%s\n", e); exit(EXIT_FAILURE); } while (0);

#define ARRAY_COUNT(ARR) ((int)(sizeof(ARR) / sizeof((ARR)[0])))

typedef struct
{
    char const* command;
    int refresh;
} bar_entry;

// bar_entry is used for configs, entry has just more stuff and is used by the
// program.
typedef struct
{
    char const* command;
    int refresh;
    int sec_to_refresh;
} entry;

typedef struct
{
    int status; // 0 on success, otherwise error and output is not set!
    char output[124]; // Any reasonable output for status bar should fit.
} exec_script_ret;

#include "config.h"

#define NENTRIES (ARRAY_COUNT(entries))

static entry g_entries[NENTRIES];
static exec_script_ret g_bar[NENTRIES];

// Non-zero means an update is required.
static char g_work_queue[NENTRIES];

static pthread_mutex_t work_mutex;
static pthread_cond_t work_cv;

static exec_script_ret
exec_script(char const* command)
{
    int link[2];
    pid_t pid;
    exec_script_ret retval;

    if (pipe(link) == -1)
        die("pipe");

    if ((pid = fork()) == -1)
        die("fork");

    if(pid == 0)
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
        int nbytes = read(link[0], retval.output, 124 - 1);
        retval.output[nbytes] = 0;

        char* newline = strchr(retval.output, '\n');
        if (newline)
            *newline = 0;

        wait(0);
    }

    return retval;
}

static void
update_xroot()
{
    // TODO: Out of range!
    char buffer[4096];
    buffer[0] = 0;
    strcat(buffer, "xsetroot -name ' ");
    // TODO: Escape "'" ??
    for (int i = 0; i < NENTRIES; ++i)
    {
        if (g_bar[i].output[0] == 0)
        {
            /// printf("%d: SKIPPING\n", i);
            continue;
        }

        // printf("%d: '%s'\n", i, g_bar[i].output);
        strcat(buffer, g_bar[i].output);

        if (i < NENTRIES - 1)
            strcat(buffer, "   "); // TODO: Custom spearator!
    }
    strcat(buffer, " '");
    exec_script(buffer);
}

void*
job(void* arg)
{
    pthread_mutex_lock(&work_mutex);
    goto repeat_scan;
    for (;;)
    {
        pthread_cond_wait(&work_cv, &work_mutex);
    repeat_scan:
        /* printf("Thread %ld begins scanning:\n    ", (long)arg);
        printf("{ ");
        for (int i = 0 ; i < NENTRIES; ++i)
            printf("%d ", g_work_queue[i]);
        printf("}\n");
        */

        for (int i = 0 ; i < NENTRIES; ++i)
        {
            if (g_work_queue[i] != 0)
            {
                g_work_queue[i] = 0;
                // printf("Thread %ld calculates %dth entry\n", (long)arg, i);
                pthread_cond_broadcast(&work_cv);
                pthread_mutex_unlock(&work_mutex);

                exec_script_ret outp = exec_script(g_entries[i].command);

                pthread_mutex_lock(&work_mutex);
                // printf("Thread %ld finished. Printing!\n", (long)arg);
                g_bar[i] = outp; // Update i'th entry.
                g_work_queue[i] = 0; // To be sure.
                update_xroot();
                // TODO: Now there is good time to output new version of the bar.
                goto repeat_scan; // keep scanning until there is no entry left.
            }

        }

        // Always reached with mutex; will wait for more jobs in the first
        // statement of next loop iteration.
        // printf("Thread %ld is done!\n", (long)arg);
    }

    return 0;
}

int
main()
{
    for (unsigned int i = 0; i < NENTRIES; ++i)
    {
        g_entries[i].command = entries[i].command;
        g_entries[i].refresh = entries[i].refresh;
        g_entries[i].sec_to_refresh = g_entries[i].refresh;
    }

    // Init bar; set all strings to 0 and statuses to OK.
    for (unsigned int i = 0; i < NENTRIES; ++i)
    {
        g_bar[i].status = 0;
        g_bar[i].output[0] = 0;
    }

    for (int i = 0 ; i < NENTRIES; ++i)
        g_work_queue[i] = 1;

    pthread_mutex_init(&work_mutex, 0);
    pthread_cond_init (&work_cv, 0);

    // Set up working threads.
    pthread_t thread[NTHREADS];
    pthread_attr_t attr;

    // pthread_mutex_lock(&work_mutex);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for(int i = 0; i < NTHREADS; i++)
    {
        // printf("Main: creating thread %d\n", i);
        if (pthread_create(&thread[i], &attr, job, (void*)((long)i)))
            die("pthread_create");
    }

    // This will start all workers and they will set bar values for the first
    // time.
    pthread_cond_broadcast(&work_cv);
    // pthread_mutex_unlock(&work_mutex);

    int last_slept = 0;
    int to_sleep_default = 3600; // To limit sleep time.
    int to_sleep = to_sleep_default;
    for (;;)
    {
        pthread_mutex_lock(&work_mutex);
        printf("SCHEDULER HAS WOKEN UP!\n");
        for (int i = 0; i < NENTRIES; ++i)
        {
            printf("  %d - %d\n", i, g_entries[i].sec_to_refresh);
            g_entries[i].sec_to_refresh -= last_slept;
            if (g_entries[i].sec_to_refresh <= 0)
            {
                g_entries[i].sec_to_refresh = g_entries[i].refresh;
                g_work_queue[i] = 1;
                printf("    TASK %d is scheduled after %d sec\n",
                       i, g_entries[i].refresh);
            }

            if (g_entries[i].sec_to_refresh < to_sleep)
                to_sleep = g_entries[i].sec_to_refresh;
        }
        pthread_cond_broadcast(&work_cv);
        pthread_mutex_unlock(&work_mutex);

        printf("SLEEPING FOR: %d\n", to_sleep);
        sleep(to_sleep);
        last_slept = to_sleep;
        to_sleep = to_sleep_default;
    }

    return 0;
}
