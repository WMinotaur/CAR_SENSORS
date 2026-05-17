#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>


#include "shared_data.h"

LOG_MODULE_REGISTER(sensor, LOG_LEVEL_INF);


K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_data_msg), 5, 4);



static int process_mpu6050(const struct device *dev) {
    struct sensor_value temperature;
    struct sensor_value accel[3];
    struct sensor_value gyro[3];

    int rc = sensor_sample_fetch(dev);
    if(rc < 0) {
        LOG_ERR("Błąd pobierania danych (kod: %d). Sprawdź przewody!", rc);
        return rc;
    }

    sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
    sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro);
    sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temperature);

    struct sensor_data_msg msg;
    msg.temp = sensor_value_to_double(&temperature);
    for (int i = 0; i < 3; i++) {
        msg.accel[i] = sensor_value_to_double(&accel[i]);
        msg.gyro[i] = sensor_value_to_double(&gyro[i]);
    }

    k_msgq_put(&sensor_msgq, &msg, K_NO_WAIT);



    return 0;
}

static void sensor_thread_entry(void *p1, void *p2, void *p3) {
    const struct device *const mpu6050 = DEVICE_DT_GET_ONE(invensense_mpu6050);

    if(!device_is_ready(mpu6050)) {
        LOG_ERR("Urzadzenie %s nie jest gotowe do pracy!", mpu6050 ? mpu6050->name : "NULL");
        return;
    }

    LOG_INF("Rozpoczynam odpytywanie czujnika MPU6050 (Tryb Polling)...");

    while(1) {
        process_mpu6050(mpu6050);
        k_msleep(500);
    }
}

K_THREAD_DEFINE(sensor_thread_id, 2048, sensor_thread_entry, NULL, NULL, NULL, 3, 0, 0);