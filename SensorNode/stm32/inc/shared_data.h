#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <zephyr/kernel.h>

struct sensor_data_msg {
  float temp;
  float accel[3];
  float gyro[3];
};

 struct hcsr04_data_msg {
  double distance_m;
};

extern struct k_msgq sensor_msgq;

#endif /* SHARED_DATA_H */
