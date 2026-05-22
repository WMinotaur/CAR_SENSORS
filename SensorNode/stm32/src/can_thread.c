#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "shared_data.h"

LOG_MODULE_REGISTER(can_thread, LOG_LEVEL_INF);

#if DT_HAS_CHOSEN(zephyr_canbus)
#define CAN_CHOSEN_NODE DT_CHOSEN(zephyr_canbus)
#elif DT_HAS_CHOSEN(zephyr_can_primary)
#define CAN_CHOSEN_NODE DT_CHOSEN(zephyr_can_primary)
#else
#error "No CAN controller chosen in devicetree"
#endif

const struct device *const can_dev = DEVICE_DT_GET(CAN_CHOSEN_NODE);
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

  /* IDs used by the ESP node (listener/sender examples): */
  uint32_t const DATA_ID = 0x100;
  uint32_t const HEARTBEAT_ID = 0x7FF;

  const struct can_filter esp_data_filter = {.id = DATA_ID, .mask = CAN_STD_ID_MASK, .flags = 0};
  const struct can_filter esp_heartbeat_filter = {.id = HEARTBEAT_ID, .mask = CAN_STD_ID_MASK, .flags = 0};

  can_add_rx_filter_msgq(can_dev, &can_rx_msgq, &esp_data_filter);
  can_add_rx_filter_msgq(can_dev, &can_rx_msgq, &esp_heartbeat_filter);

  LOG_INF("STM32 CAN: Gotowy do dzialania.");

  struct can_frame tx_frame = {.id = DATA_ID, .dlc = 0, .flags = 0};

  /* heartbeat sending state (send every 1000 ms) */
  uint64_t last_heartbeat_ms = 0;
  uint32_t const heartbeat_interval_ms = 1000;

  while(1) {
    struct sensor_data_msg sensor_data;
    if(k_msgq_get(&sensor_msgq, &sensor_data, K_FOREVER) == 0) {
      // LOG_INF("accel: %f, %f, %f | gyro: %f, %f, %f | temp: %f", (double)sensor_data.accel[0], (double)sensor_data.accel[1],
      //         (double)sensor_data.accel[2], (double)sensor_data.gyro[0], (double)sensor_data.gyro[1], (double)sensor_data.gyro[2],
      //         (double)sensor_data.temp);
      /* Pack temp and 3-axis accel into 8 bytes (int16_t each, temp first) */
      int16_t temp_scaled = (int16_t)(sensor_data.temp * 100.0f);
      int16_t accel_x_scaled = (int16_t)(sensor_data.accel[0] * 100.0f);
      int16_t accel_y_scaled = (int16_t)(sensor_data.accel[1] * 100.0f);
      int16_t accel_z_scaled = (int16_t)(sensor_data.accel[2] * 100.0f);

      tx_frame.id = DATA_ID;
      tx_frame.dlc = 8;
      /* Preserve big-endian style used earlier (high byte first) */
      tx_frame.data[0] = (temp_scaled >> 8) & 0xFF;
      tx_frame.data[1] = temp_scaled & 0xFF;
      tx_frame.data[2] = (accel_x_scaled >> 8) & 0xFF;
      tx_frame.data[3] = accel_x_scaled & 0xFF;
      tx_frame.data[4] = (accel_y_scaled >> 8) & 0xFF;
      tx_frame.data[5] = accel_y_scaled & 0xFF;
      tx_frame.data[6] = (accel_z_scaled >> 8) & 0xFF;
      tx_frame.data[7] = accel_z_scaled & 0xFF;

      int err = can_send(can_dev, &tx_frame, K_MSEC(10), NULL, NULL);
      // if(err == 0) {
      //   LOG_INF(">>> Wysłano odczyty MPU6050 (CAN_ID: 0x123)");
      // } else {
      //   LOG_ERR(">>> Blad wysylania na szynę CAN (kod: %d)", err);
      // }

      /* Send heartbeat periodically with 8-byte timestamp (microseconds) */
      uint64_t now_ms = k_uptime_get();
      if((now_ms - last_heartbeat_ms) >= heartbeat_interval_ms) {
        last_heartbeat_ms = now_ms;
        uint64_t ts_us = now_ms * 1000ULL;
        struct can_frame hb = {.id = HEARTBEAT_ID, .dlc = 8, .flags = 0};
        /* little-endian timestamp to match typical ESP printing */
        for(int i = 0; i < 8; i++) {
          hb.data[i] = (ts_us >> (8 * i)) & 0xFF;
        }
        int hberr = can_send(can_dev, &hb, K_MSEC(10), NULL, NULL);
        // if(hberr == 0) {
        //   LOG_INF(">>> Wysłano heartbeat (CAN_ID: 0x%03x)", HEARTBEAT_ID);
        // } else {
        //   LOG_ERR(">>> Blad wysylania heartbeat (kod: %d)", hberr);
        // }
      }
    }
  }
}

K_THREAD_DEFINE(can_thread_id, 2048, can_thread_entry, NULL, NULL, NULL, 7, 0, 0);
