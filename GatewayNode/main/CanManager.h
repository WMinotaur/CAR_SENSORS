#pragma once

#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"

class TwaiListener {
public:
    TwaiListener();
    void begin();

private:
    static constexpr int TX_GPIO = 4;
    static constexpr int RX_GPIO = 5;
    static constexpr uint32_t BITRATE = 500000;
    static constexpr uint32_t DATA_ID = 0x123;
    static constexpr int POLL_DEPTH = 200;

    struct ListenerData {
        twai_frame_t frame;
        uint8_t data[TWAI_FRAME_MAX_LEN];
    };

    twai_node_handle_t node_hdl;
    ListenerData* rx_pool;
    SemaphoreHandle_t free_pool_semaphore;
    SemaphoreHandle_t rx_result_semaphore;
    int write_idx;
    int read_idx;

    void taskLoop();

    static void taskWrapper(void* pvParameters);
    static bool IRAM_ATTR onErrorCallback(twai_node_handle_t handle, const twai_error_event_data_t *edata, void *user_ctx);
    static bool IRAM_ATTR onStateChangeCallback(twai_node_handle_t handle, const twai_state_change_event_data_t *edata, void *user_ctx);
    static bool IRAM_ATTR onRxCallback(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx);
};
