#pragma once 

#include <string>
#include "esp_err.h"
#include "esp_event.h"

class WiFiManager{
    public:
    // Constructor
    WiFiManager();

    // Function for launching WiFI network
    bool startAP (const std::string& ssid, const std::string& password);

    private:
    // Static function that handles system events (required by ESP-IDF because of C language)
    static void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

};