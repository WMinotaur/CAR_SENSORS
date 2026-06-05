#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "shared_data.h"

/*
 * UWAGA: Zaktualizuj swój plik "shared_data.h", dodając do niego
 * poniższą strukturę dla danych z czujnika odległości:
 *
 * struct hcsr04_data_msg {
 *     double distance_m;
 * };
 */

LOG_MODULE_REGISTER(hcsr04_sensor, LOG_LEVEL_INF);

/* Definicja kolejki wiadomości dla czujnika HC-SR04 */
K_MSGQ_DEFINE(hcsr04_msgq, sizeof(struct hcsr04_data_msg), 5, 4);

static int process_hcsr04(const struct device *dev) {
  struct sensor_value distance;

  /* Pobranie próbki z czujnika */
  int rc = sensor_sample_fetch(dev);
  if(rc < 0) {
    LOG_ERR("Błąd pobierania danych (kod: %d). Sprawdź przewody!", rc);
    return rc;
  }

  /* Odczytanie wyliczonej odległości do struktury sensor_value */
  sensor_channel_get(dev, SENSOR_CHAN_DISTANCE, &distance);

  /* Przygotowanie wiadomości do wysłania w kolejce */
  struct hcsr04_data_msg msg;
  msg.distance_m = sensor_value_to_double(&distance);

  /* Logowanie informacyjne - wyświetla to co czujnik odczytał */
  LOG_INF("Odległość: %.3f m", msg.distance_m);

  /* Wysłanie danych do kolejki (bez blokowania) */
  k_msgq_put(&hcsr04_msgq, &msg, K_NO_WAIT);

  return 0;
}

static void hcsr04_thread_entry(void *p1, void *p2, void *p3) {
  /* Pobranie wskaźnika na urządzenie na podstawie "compatible" z Devicetree.
     Działa analogicznie do DEVICE_DT_GET_ONE z Twojego kodu. */
  //   const struct device *const hcsr04 = DEVICE_DT_GET(DT_NODELABEL(hc_sr04_0));

  //   if(!device_is_ready(hcsr04)) {
  //     LOG_ERR("Urzadzenie %s nie jest gotowe do pracy!", hcsr04 ? hcsr04->name : "NULL");
  //     return;
  //   }
  // const struct device *const hcsr04 = DEVICE_DT_GET(DT_NODELABEL(hc_sr04_0));
  //   LOG_INF("Rozpoczynam odpytywanie czujnika HC-SR04 (Tryb Polling)...");

  while(1) {
    // process_hcsr04(hcsr04);
    k_msleep(500); /* Próbkowanie co 500 ms */
  }
}

/* Utworzenie osobnego wątku dla czujnika odległości */
K_THREAD_DEFINE(hcsr04_thread_id, 2048, hcsr04_thread_entry, NULL, NULL, NULL, 3, 0, 0);
