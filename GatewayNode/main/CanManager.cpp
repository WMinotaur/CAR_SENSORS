#include "CanManager.h"
#include "esp_log.h"
#include <cstdio>

static const char* TAG = "TwaiListener";

TwaiListener::TwaiListener() : 
    node_hdl(nullptr), 
    rx_pool(nullptr), 
    free_pool_semaphore(nullptr), 
    rx_result_semaphore(nullptr), 
    write_idx(0), 
    read_idx(0) 
{
}

void TwaiListener::begin() {
    ESP_LOGI(TAG, "Initializing TWAI Listen Only Mode...");

    free_pool_semaphore = xSemaphoreCreateCounting(POLL_DEPTH, POLL_DEPTH);
    rx_result_semaphore = xSemaphoreCreateCounting(POLL_DEPTH, 0);
    assert(free_pool_semaphore != nullptr);
    assert(rx_result_semaphore != nullptr);

    rx_pool = new ListenerData[POLL_DEPTH]();
    assert(rx_pool != nullptr);
    
    for (int i = 0; i < POLL_DEPTH; i++) {
        rx_pool[i].frame.buffer = rx_pool[i].data;
        rx_pool[i].frame.buffer_len = sizeof(rx_pool[i].data);
    }
    ESP_LOGI(TAG, "Buffer initialized: %d slots", POLL_DEPTH);

    twai_onchip_node_config_t node_config = {}; 
    
    node_config.io_cfg.tx = (gpio_num_t)TX_GPIO;
    node_config.io_cfg.rx = (gpio_num_t)RX_GPIO;
    node_config.io_cfg.quanta_clk_out = GPIO_NUM_NC;
    node_config.io_cfg.bus_off_indicator = GPIO_NUM_NC;
    node_config.bit_timing.bitrate = BITRATE;
    node_config.timestamp_resolution_hz = 1000000;
    node_config.tx_queue_depth = 10;

    ESP_ERROR_CHECK(twai_new_node_onchip(&node_config, &node_hdl));
    

    twai_mask_filter_config_t data_filter = {};
    data_filter.id = DATA_ID;
    data_filter.mask = 0x7F0;
    data_filter.is_ext = false;
    
    ESP_ERROR_CHECK(twai_node_config_mask_filter(node_hdl, 0, &data_filter));

    twai_event_callbacks_t callbacks = {}; 
    callbacks.on_rx_done = onRxCallback;
    callbacks.on_error = onErrorCallback;
    callbacks.on_state_change = onStateChangeCallback;
    
    ESP_ERROR_CHECK(twai_node_register_event_callbacks(node_hdl, &callbacks, this));

    ESP_ERROR_CHECK(twai_node_enable(node_hdl));
    xTaskCreate(taskWrapper, "twai_listener_task", 4096, this, 5, nullptr);
    
    ESP_LOGI(TAG, "TWAI listener initialized and running in background.");
}

void TwaiListener::taskLoop() {
    while (true) {
        if (xSemaphoreTake(rx_result_semaphore, portMAX_DELAY) == pdTRUE) {
            twai_frame_t *frame = &rx_pool[read_idx].frame;
            
            ESP_LOGI(TAG, "RX: timestamp %llu, %lx [%d] %02x %02x %02x %02x %02x %02x %02x %02x", 
                     frame->header.timestamp, frame->header.id, frame->header.dlc, 
                     frame->buffer[0], frame->buffer[1], frame->buffer[2], frame->buffer[3], 
                     frame->buffer[4], frame->buffer[5], frame->buffer[6], frame->buffer[7]);
            
            read_idx = (read_idx + 1) % POLL_DEPTH;
            xSemaphoreGive(free_pool_semaphore);
        }
    }
}


void TwaiListener::taskWrapper(void* pvParameters) {
    TwaiListener* instance = static_cast<TwaiListener*>(pvParameters);
    instance->taskLoop();
}

bool IRAM_ATTR TwaiListener::onRxCallback(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx) {
    BaseType_t woken;
    TwaiListener* instance = static_cast<TwaiListener*>(user_ctx); // Rzutowanie

    if (xSemaphoreTakeFromISR(instance->free_pool_semaphore, &woken) != pdTRUE) {
        ESP_EARLY_LOGI(TAG, "Pool full, dropping frame");
        return (woken == pdTRUE);
    }
    
    if (twai_node_receive_from_isr(handle, &instance->rx_pool[instance->write_idx].frame) == ESP_OK) {
        instance->write_idx = (instance->write_idx + 1) % POLL_DEPTH;
        xSemaphoreGiveFromISR(instance->rx_result_semaphore, &woken);
    }
    return (woken == pdTRUE);
}

bool IRAM_ATTR TwaiListener::onErrorCallback(twai_node_handle_t handle, const twai_error_event_data_t *edata, void *user_ctx) {
    ESP_EARLY_LOGW(TAG, "bus error: 0x%lx", (long unsigned int)edata->err_flags.val);
    return false;
}

bool IRAM_ATTR TwaiListener::onStateChangeCallback(twai_node_handle_t handle, const twai_state_change_event_data_t *edata, void *user_ctx) {
    const char *twai_state_name[] = {"error_active", "error_warning", "error_passive", "bus_off"};
    ESP_EARLY_LOGI(TAG, "state changed: %s -> %s", twai_state_name[edata->old_sta], twai_state_name[edata->new_sta]);
    return false;
}
