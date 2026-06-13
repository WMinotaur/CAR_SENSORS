#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "shared_data.h"

LOG_MODULE_REGISTER(can_thread, LOG_LEVEL_INF);

const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_can_primary));
CAN_MSGQ_DEFINE(can_rx_msgq, 10);

void can_thread_entry(void) {
  if(!device_is_ready(can_dev)) {
    LOG_ERR("Kontroler CAN niegotowy!");
    return;
  }

  int start_err = can_start(can_dev);
  if(start_err != 0) {
    LOG_ERR("Blad podczas startu kontrolera CAN: %d", start_err);
    return;
  }

  const struct can_filter esp_reply_filter = {.id = 0x321, .mask = CAN_STD_ID_MASK, .flags = 0};
  const struct can_filter esp_heartbeat_filter = {.id = 0x7FF, .mask = CAN_STD_ID_MASK, .flags = 0};

  can_add_rx_filter_msgq(can_dev, &can_rx_msgq, &esp_reply_filter);
  can_add_rx_filter_msgq(can_dev, &can_rx_msgq, &esp_heartbeat_filter);

  LOG_INF("STM32 CAN: Gotowy do dzialania.");


  struct can_frame tx_frame = {.id = 0x123,.dlc = 0, .flags = 0};
  struct can_frame tx_frame_dist = {.id = 0x124, .dlc = 2, .flags = 0};

  while(1) {
    struct sensor_data_msg sensor_data;
    if(k_msgq_get(&sensor_msgq, &sensor_data, K_NO_WAIT) == 0) {
      LOG_INF("accel: %f, %f, %f | gyro: %f, %f, %f | temp: %f", (double)sensor_data.accel[0], (double)sensor_data.accel[1],
              (double)sensor_data.accel[2], (double)sensor_data.gyro[0], (double)sensor_data.gyro[1], (double)sensor_data.gyro[2],
              (double)sensor_data.temp);
      int16_t temp_scaled = (int16_t)(sensor_data.temp * 100.0f);
      int16_t accel_x_scaled = (int16_t)(sensor_data.accel[0] * 100.0f);
      tx_frame.dlc = 4;                                
      tx_frame.data[0] = (temp_scaled >> 8) & 0xFF;    
      tx_frame.data[1] = temp_scaled & 0xFF;          
      tx_frame.data[2] = (accel_x_scaled >> 8) & 0xFF;
      tx_frame.data[3] = accel_x_scaled & 0xFF;
      int err = can_send(can_dev, &tx_frame, K_MSEC(10), NULL, NULL);
      if(err == 0) {
        LOG_INF(">>> Wysłano odczyty MPU6050 (CAN_ID: 0x123)");
      } else {
        LOG_ERR(">>> Blad wysylania na szynę CAN (kod: %d)", err);
      }
    }
    struct distance_data_msg dist_data;
    if(k_msgq_get(&distance_msgq, &dist_data, K_NO_WAIT) == 0) {
      LOG_INF("Dystans: %f m", (double)dist_data.distance);
      
      /* Konwersja metrów na centymetry do wysłania po CAN (zaokrąglone do int16_t) */
      uint16_t dist_cm = (uint16_t)(dist_data.distance * 100.0f);
      
      tx_frame_dist.data[0] = (dist_cm >> 8) & 0xFF;
      tx_frame_dist.data[1] = dist_cm & 0xFF;
      
      int err = can_send(can_dev, &tx_frame_dist, K_MSEC(10), NULL, NULL);
      if(err == 0) {
        LOG_INF(">>> Wysłano odczyty HC-SR04 (CAN_ID: 0x124)");
      } else {
        LOG_ERR(">>> Blad wysylania HC-SR04 na szynę CAN (kod: %d)", err);
      }
    }
    k_msleep(10);
  }
}

K_THREAD_DEFINE(can_thread_id, 2048, can_thread_entry, NULL, NULL, NULL, 7, 0, 0);
