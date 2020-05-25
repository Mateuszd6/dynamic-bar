#ifndef UTIL_H_
#define UTIL_H_

static void
die(const char *errstr, ...)
{
    va_list ap;

    va_start(ap, errstr);
    vfprintf(stderr, errstr, ap);
    va_end(ap);
    exit(1);
}

#define ARRAY_COUNT(ARR) ((int)(sizeof(ARR) / sizeof((ARR)[0])))

#endif // UTIL_H_
