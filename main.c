#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// TODO: Replace die!
#define die(e) do { fprintf(stderr, "%s\n", e); exit(EXIT_FAILURE); } while (0);

#define NTHREADS 2

#define ARRAY_COUNT(ARR) ((int)(sizeof(ARR) / sizeof((ARR)[0])))

typedef struct
{
    char const* command;
    char const* head;
    int refresh;
} bar_entry;

typedef struct
{
    int status; // 0 on success, otherwise error and output is not set!
    char output[124]; // Any reasonable output for status bar should fit.
} exec_script_ret;

static bar_entry entries[] =
{
    { "~/.i3/md/wifi.sh", "", 10 },
    { "~/.i3/md/updates.sh", "", 10 },
    { "~/.i3/md/mail.sh", "", 10 },
    { "~/.i3/md/disc.sh", "", 10 },
    { "~/.i3/md/memory.sh", "", 10 },
    { "~/.i3/cpu-usage", "", 10 },
    { "~/.i3/temp", "", 10 },
    { "~/.i3/battery-info.sh", "", 10 },
    { "~/.i3/md/volume.sh 5", "", 10 },
    { "echo \" `date '+%d %b %H:%M'`\"", "", 10 },
};

static exec_script_ret g_bar[ARRAY_COUNT(entries)];

// Non-zero means an update is required.
static volatile char g_work_queue[ARRAY_COUNT(entries)];

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
    for (int i = 0; i < ARRAY_COUNT(entries); ++i)
    {
        if (g_bar[i].output[0] == 0)
        {
            printf("%d: SKIPPING\n", i);
            continue;
        }

        printf("%d: %s'%s'\n",
               i,
               entries[i].head ? entries[i].head : "",
               g_bar[i].output);

        strcat(buffer, entries[i].head ? entries[i].head : "");
        strcat(buffer, g_bar[i].output);

        if (i < ARRAY_COUNT(entries) - 1)
            strcat(buffer, "   "); // TODO: Custom spearator!
    }
    strcat(buffer, " '");
    exec_script(buffer);
}

void*
job(void* arg)
{
    pthread_mutex_lock(&work_mutex);
    for (;;)
    {
        pthread_cond_wait(&work_cv, &work_mutex);
    repeat_scan:
        printf("Thread %ld begins scanning:\n    ", (long)arg);
        printf("{ ");
        for (int i = 0 ; i < ARRAY_COUNT(entries); ++i)
            printf("%d ", g_work_queue[i]);
        printf("}\n");

        for (int i = 0 ; i < ARRAY_COUNT(entries); ++i)
        {
            if (g_work_queue[i] != 0)
            {
                g_work_queue[i] = 0;
                printf("Thread %ld calculates %dth entry\n", (long)arg, i);
                pthread_cond_broadcast(&work_cv);
                pthread_mutex_unlock(&work_mutex);

                exec_script_ret outp = exec_script(entries[i].command);
                sleep(1);

                pthread_mutex_lock(&work_mutex);
                printf("Thread %ld finished. Printing!\n", (long)arg);
                g_bar[i] = outp; // Update i'th entry.
                g_work_queue[i] = 0; // To be sure.
                update_xroot();
                // TODO: Now there is good time to output new version of the bar.
                goto repeat_scan; // If found an entry, repeat the fullscan.
            }

        }

        // Always reached with mutex; will wait for more jobs in the first
        // statement of next loop iteration.
        printf("Thread %ld is done!\n", (long)arg);
    }

    // TODO: Use this to notify workers
    // int pthread_cond_broadcast(pthread_cond_t *cond);

    return 0;
}

int
main()
{
    // Init bar; set all strings to 0 and statuses to OK.
    for (unsigned int i = 0; i < ARRAY_COUNT(entries); ++i)
    {
        g_bar[i].status = 0;
        g_bar[i].output[0] = 0;
    }

    for (int i = 0 ; i < ARRAY_COUNT(entries); ++i)
        g_work_queue[i] = 1;

    pthread_mutex_init(&work_mutex, 0);
    pthread_cond_init (&work_cv, 0);

    // Set up working threads.
    pthread_t thread[NTHREADS];
    pthread_attr_t attr;

    // TODO: Copypaste and is it a thing?
    // Initialize and set thread detached attribute
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for(int i = 0; i < NTHREADS; i++)
    {
        printf("Main: creating thread %d\n", i);
        if (pthread_create(&thread[i], &attr, job, (void*)((long)i)))
        {
            // TODO: pthread_create returns an errno, use it!
            die("pthread_create");
        }
    }

    for (;;)
    {
        pthread_mutex_lock(&work_mutex);
        printf("REFRESHING ALL!\n");

        for (int i = 0; i < ARRAY_COUNT(entries); ++i)
            g_work_queue[i] = 1;

        pthread_cond_broadcast(&work_cv);
        pthread_mutex_unlock(&work_mutex);
        sleep(10);
    }

    return 0;
}
