#ifndef MEM_H_
#define MEM_H_

#include <stddef.h>
#include <string.h>

typedef struct
{
    // Probably in Kb?
    size_t mem_total;
    size_t mem_free;
    size_t mem_cached;
    size_t buffers;
    int status;
} meminfo;

static meminfo
mem_usage()
{
    char buf[4096];
    meminfo retval = {0};
    FILE* minfo = fopen("/proc/meminfo", "r");

    if (!minfo)
        goto error;

    size_t read = fread(buf, 1, 4096, minfo);
    if (!read)
        goto error_needs_close;

    if (fclose(minfo) != 0)
        goto error;

    char* memtotal = strstr(buf, "MemTotal:");
    char* scan_head = memtotal + strlen("MemTotal:");
    retval.mem_total = atoi(scan_head);

    memtotal = strstr(buf, "MemFree:");
    scan_head = memtotal + strlen("MemFree:");
    retval.mem_free = atoi(scan_head);

    memtotal = strstr(buf, "Buffers:");
    scan_head = memtotal + strlen("Buffers:");
    retval.buffers = atoi(scan_head);

    memtotal = strstr(buf, "Cached:");
    scan_head = memtotal + strlen("Cached:");
    retval.mem_cached = atoi(scan_head);

    return retval;

error_needs_close:
    fclose(minfo);
error:
    retval.status = 1;
    return retval;
}

static int
get_free_mem(meminfo mi)
{
    // It would be very, very weird if this turned out negative.
    size_t f = mi.mem_total - mi.mem_free - mi.buffers - mi.mem_cached;

    // Round up:
    size_t rem = f % 1000;
    return (int)(f / 1000) + (rem >= 5 ? 1 : 0);
}

#endif // MEM_H_
