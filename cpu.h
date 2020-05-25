#ifndef CPU_H_
#define CPU_H_

// From: https://stackoverflow.com/a/23376195
/*
     user  nice system idle    iowait irq  softirq steal guest guest_nice
cpu  74608 2520 24433  1117073 6176   4054 0       0     0     0

PrevIdle = previdle + previowait
Idle = idle + iowait

PrevNonIdle = prevuser + prevnice + prevsystem + previrq + prevsoftirq + prevsteal
NonIdle = user + nice + system + irq + softirq + steal

PrevTotal = PrevIdle + PrevNonIdle
Total = Idle + NonIdle

# differentiate: actual value minus the previous one
totald = Total - PrevTotal
idled = Idle - PrevIdle

CPU_Percentage = (totald - idled)/totald
*/

typedef struct
{
    size_t user;
    size_t nice;
    size_t system;
    size_t idle;
    size_t iowaits;
    size_t irq;

    // Coputed by us:
    size_t ctotal;
    size_t cidle;
} cpu_info;

static int first_computed = 0;
static cpu_info prev;
static cpu_info curr;

// This will wait for a second only the first time. It depends on a fact that
// status bar actually invokes this often - like in 5sec intervals. It makes
// getting cpu usae waay faster than through any script.
static float
get_cpu_usage()
{
start_over:
    {
        cpu_info info;
        FILE* file = fopen("/proc/stat", "r");
        if (!file)
            die("fopen");

        int scanned = fscanf(file, "cpu %16lu %16lu %16lu %16lu %16lu %16lu ",
                             &info.user, &info.nice,
                             &info.system, &info.idle,
                             &info.iowaits, &info.irq);
        fclose(file);


        if (scanned != 6)
            die("cpu");

        curr = info;
        curr.cidle = curr.idle + curr.iowaits;
        curr.ctotal = curr.cidle + curr.user + curr.nice + curr.system + curr.irq;

        if (!first_computed)
        {
            first_computed = 1;
            prev = curr;
            sleep(1);
            goto start_over;
        }

        ssize_t totald = curr.ctotal - prev.ctotal;
        ssize_t idled = curr.cidle - prev.cidle;

        float retval = ((float)(totald - idled)) / totald;
        return retval;
    }
}

#endif // CPU_H_
