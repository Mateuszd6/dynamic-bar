#ifndef CONFIG_H_
#define CONFIG_H_

#define BAR_BEFORE_FIRST " "
#define BAR_AFTER_LAST " "
#define BAR_SEPARATOR "   "

// Usually builtin will be fast, the rest will be slow. In doubt just mark an
// entry as slow, it will be fine. It is just an optimization trick.
static bar_entry entries[] =
{
    { BUILTIN(B_TIMEDATE, " %s"), 5, ET_FAST},
    { SCRIPT("~/.i3/md/volume.sh 5"), 30, ET_FAST },

    { SCRIPT("~/.i3/battery-info.sh"), 60, ET_SLOW },
    { BUILTIN(B_TEMP, " %.1f°C"), 5, ET_FAST },
    { BUILTIN(B_CPU, " %.2f%%"), 5, ET_FAST },
    { BUILTIN(B_MEM, " %dM"), 5, ET_FAST },

    { SCRIPT("~/.i3/md/disc.sh"), 3600, ET_SLOW },
    { SCRIPT("~/.i3/md/mail.sh"), 300, ET_SLOW },
    { SCRIPT("~/.i3/md/updates.sh"), 3600, ET_SLOW },
    { SCRIPT("~/.i3/md/wifi.sh"), 30, ET_SLOW },
};

#endif // CONFIG_H_
