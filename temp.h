#ifndef TEMP_H_
#define TEMP_H_

static float
get_temp()
{
    // TODO: There is considerably more than one file to check.
    FILE* file = fopen("/sys/class/thermal/thermal_zone8/temp", "r");
    if (!file)
        die("fopen");

    int temp;
    fscanf(file, "%d", &temp);
    fclose(file);

    return ((float)temp) / 1000;
}

#endif // TEMP_H_
