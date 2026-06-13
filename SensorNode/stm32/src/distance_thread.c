#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "shared_data.h"

LOG_MODULE_REGISTER(hc_sr04, LOG_LEVEL_INF);

/* Definicja nowej kolejki wiadomości dla czujnika odległości */
K_MSGQ_DEFINE(distance_msgq, sizeof(struct distance_data_msg), 5, 4);

static int process_hc_sr04(const struct device *dev) {
    struct sensor_value dist_val;

    int rc = sensor_sample_fetch(dev);
    if(rc < 0) {
        LOG_ERR("Błąd pobierania danych HC-SR04 (kod: %d).", rc);
        return rc;
    }

    sensor_channel_get(dev, SENSOR_CHAN_DISTANCE, &dist_val);

    struct distance_data_msg msg;
    /* sensor_value_to_double konwertuje wartość na metry */
    msg.distance = sensor_value_to_double(&dist_val);

    k_msgq_put(&distance_msgq, &msg, K_NO_WAIT);

    return 0;
}

static void hc_sr04_thread_entry(void *p1, void *p2, void *p3) {
    /* Pobieramy urządzenie za pomocą jego etykiety z Device Tree (hc_sr04_0) */
    const struct device *const hc_sr04_dev = DEVICE_DT_GET(DT_NODELABEL(hc_sr04_0));

    if(!device_is_ready(hc_sr04_dev)) {
        LOG_ERR("Urzadzenie %s nie jest gotowe do pracy! Sprawdź binding w Device Tree.", 
                hc_sr04_dev ? hc_sr04_dev->name : "NULL");
        return;
    }

    LOG_INF("Rozpoczynam odpytywanie czujnika HC-SR04...");

    while(1) {
        process_hc_sr04(hc_sr04_dev);
        /* Odpytujemy rzadziej niż MPU, np. co 200 ms */
        k_msleep(200); 
    }
}

/* Uruchamiamy wątek z priorytetem 4 */
K_THREAD_DEFINE(hc_sr04_thread_id, 1024, hc_sr04_thread_entry, NULL, NULL, NULL, 4, 0, 0);