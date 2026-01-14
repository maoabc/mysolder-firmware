
#ifndef __SLEEP_DETECTION_H
#define __SLEEP_DETECTION_H

#include <zephyr/kernel.h>

struct app;
int sleep_detection_init(struct app *app);

#endif
