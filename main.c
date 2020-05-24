#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define die(e) do { fprintf(stderr, "%s\n", e); exit(EXIT_FAILURE); } while (0);

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

static exec_script_ret bar[ARRAY_COUNT(entries)];

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

int
main()
{
    // Init bar; set all strings to 0 and statuses to OK.
    for (unsigned int i = 0; i < ARRAY_COUNT(entries); ++i)
    {
        bar[i].status = 0;
        bar[i].output[0] = 0;
    }

    // exec_script_ret out = exec_script("echo $BROWSER");
    for (int i = 0; i < ARRAY_COUNT(entries); ++i)
    {
        bar[i] = exec_script(entries[i].command);
        // exec_script_ret out = exec_script(entries[i].command);
        // printf("Output [%d]: \"%s\"\n", i, out.output);
    }

    for (int i = 0; i < ARRAY_COUNT(entries); ++i)
    {

    }

    // TODO: Out of range!
    char buffer[4096];
    buffer[0] = 0;
    strcat(buffer, "xsetroot -name ' ");
    // TODO: Escape "'"
    for (int i = 0; i < ARRAY_COUNT(entries); ++i)
    {
        if (bar[i].output[0] == 0)
        {
            printf("%d: SKIPPING\n", i);
            continue;
        }

        printf("%d: %s'%s'\n",
               i,
               entries[i].head ? entries[i].head : "",
               bar[i].output);

        strcat(buffer, entries[i].head ? entries[i].head : "");
        strcat(buffer, bar[i].output);

        if (i < ARRAY_COUNT(entries) - 1)
            strcat(buffer, "   "); // TODO: Custom spearator!
    }
    strcat(buffer, " '");

    // printf("OUTPUT: '%s'\n", buffer);
    exec_script(buffer);

    return 0;
}
