#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_twai.h"         // Nowe API (funkcje węzła)
#include "esp_twai_onchip.h"  // Nowe API (tworzenie i konfiguracja węzła)

#define TX_GPIO_NUM    4 
#define RX_GPIO_NUM    5

static const char *TAG = "ESP32_CAN";

// Globalny uchwyt węzła
twai_node_handle_t can_node = NULL;

// --- Funkcja Wywoływana Przez Przerwanie (Gdy przyjdzie ramka) ---
static bool twai_rx_cb(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx) {
    uint8_t recv_buff[8];
    twai_frame_t rx_frame = {
        .buffer = recv_buff,
        .buffer_len = sizeof(recv_buff),
    };

    // Pobranie danych z bufora przerwania
    if (twai_node_receive_from_isr(handle, &rx_frame) == ESP_OK) {
        
        // Zwykłe ESP_LOGI mogłoby zaciąć przerwanie, używamy wersji "EARLY" dedykowanej dla ISR
        ESP_EARLY_LOGI(TAG, "Odebrano ramke! ID: 0x%lX", rx_frame.header.id);
        
        // Jeśli to wiadomość od STM32 (z ID 0x123)
        if (rx_frame.header.id == 0x123) {
            
            // Błyskawicznie odpowiadamy z poziomu przerwania!
            static const uint8_t ack_data[2] = {0xAA, 0xBB};
            static const twai_frame_t tx_msg = {
                .header.id = 0x321,             // ID odpowiedzi
                .buffer = (uint8_t *)ack_data,
                .buffer_len = sizeof(ack_data),
            };
            
            // Wysyłamy potwierdzenie na magistralę (timeout ustawiony na 0)
            twai_node_transmit(handle, &tx_msg, 0);
        }
    }
    
    return false; // Nie wybudzamy zadań o wyższym priorytecie
}

// --- Główny Program ---
void app_main(void) {
    ESP_LOGI(TAG, "Inicjalizacja TWAI v6.0 (Zdarzenia)...");

    // 1. Konfiguracja sprzętowa (500 kbps)
    twai_onchip_node_config_t node_config = {
        .io_cfg = {
            .tx = TX_GPIO_NUM,
            .rx = RX_GPIO_NUM,
            .quanta_clk_out = -1,     // Nie używamy
            .bus_off_indicator = -1,  // Nie używamy
        },
        .bit_timing = {
            .bitrate = 500000, // Prędkość zgodna z STM32
        },
        .fail_retry_cnt = 3,
        .tx_queue_depth = 10,
    };

    // 2. Utworzenie węzła
    ESP_ERROR_CHECK(twai_new_node_onchip(&node_config, &can_node));

    // 3. Konfiguracja filtra - zera oznaczają przepuszczanie wszystkiego (Accept All)
    twai_mask_filter_config_t mfilter_cfg = {
        .id = 0,
        .mask = 0,
        .is_ext = false,
    };
    ESP_ERROR_CHECK(twai_node_config_mask_filter(can_node, 0, &mfilter_cfg));

    // 4. Zarejestrowanie naszej funkcji nasłuchującej
    twai_event_callbacks_t cbs = {
        .on_rx_done = twai_rx_cb, // Podpinamy nasz twai_rx_cb
    };
    ESP_ERROR_CHECK(twai_node_register_event_callbacks(can_node, &cbs, NULL));

    // 5. Uruchomienie układu CAN
    ESP_ERROR_CHECK(twai_node_enable(can_node));
    ESP_LOGI(TAG, "TWAI (CAN) uruchomiony pomyslnie!");

    // 6. Pętla główna – wysyła sygnał życia (Heartbeat) co 5 sekund
    while (1) {
        uint8_t data[4] = {1, 2, 3, 4};
        twai_frame_t heartbeat_msg = {
            .header.id = 0x7FF,
            .buffer = data,
            .buffer_len = 4
        };
        
        twai_node_transmit(can_node, &heartbeat_msg, 0);
        ESP_LOGI(TAG, "Wyslano Heartbeat (ID: 0x7FF)");
        
        vTaskDelay(pdMS_TO_TICKS(5000)); 
    }
}