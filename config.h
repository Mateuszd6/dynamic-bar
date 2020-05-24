#ifndef CONFIG_H_
#define CONFIG_H_

static bar_entry entries[] =
{
    { "~/.i3/md/wifi.sh", 30 },
    { "~/.i3/md/updates.sh", 3600 },
    { "~/.i3/md/mail.sh", 300 },
    { "~/.i3/md/disc.sh", 3600 },

    // TODO: Optimize and call directly from C
    { "~/.i3/md/memory.sh", 5 },
    { "~/.i3/cpu-usage", 5 },
    { "~/.i3/temp", 5 },
    { "~/.i3/battery-info.sh", 60 },

    { "~/.i3/md/volume.sh 5", 30 },
    { "echo \" `date '+%d %b %H:%M'`\"", 5 },
};

// Number of threads.
#define NTHREADS 1

#endif // CONFIG_H_
