#ifndef TEMP_H_
#define TEMP_H_

// TODO: There is considerably more than one file to check. This one just works
//       for me on the machine this think was coded.
#define TEMPERATURE_FILE "/sys/class/thermal/thermal_zone8/temp"

typedef struct
{
    int status;
    float temp;
} cpu_temp;

static cpu_temp
get_temp()
{
    cpu_temp retval = {0};
    FILE* file = fopen(TEMPERATURE_FILE, "r");
    if (!file)
        goto error;

    int x;
    int scanned = fscanf(file, "%d", &x);
    if (fclose(file) != 0)
        goto error;

    if (scanned == EOF)
        goto error;

    retval.temp = ((float)x) / 1000;
    return retval;

error:
    retval.status = 1;
    return retval;
}

#endif // TEMP_H_
