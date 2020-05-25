#ifndef CONFIG_H_
#define CONFIG_H_

#define BAR_BEFORE_FIRST " "
#define BAR_AFTER_LAST " "
#define BAR_SEPARATOR "   "

static bar_entry entries[] =
{
    { BUILTIN(B_TIMEDATE, " %s"), 5 },
    { SCRIPT("~/.i3/md/volume.sh 5"), 30 },

    { SCRIPT("~/.i3/battery-info.sh"), 60 },
    { BUILTIN(B_TEMP, " %.1f°C"), 5 },
    { BUILTIN(B_CPU, " %.2f%%"), 5 },
    { BUILTIN(B_MEM, " %dM"), 5 },

    { SCRIPT("~/.i3/md/disc.sh"), 3600 },
    { SCRIPT("~/.i3/md/mail.sh"), 300 },
    { SCRIPT("~/.i3/md/updates.sh"), 3600 },
    { SCRIPT("~/.i3/md/wifi.sh"), 30 },
};

// Number of threads.
#define NTHREADS 1

#endif // CONFIG_H_
