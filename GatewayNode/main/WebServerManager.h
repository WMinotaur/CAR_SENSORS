#pragma once

#include <esp_http_server.h>
#include <string>

class WebServerManager {
    public:
    WebServerManager();
    void start();

    void broadcastDistance(int distance);

    private:
    httpd_handle_t server;

    // Handlers (They must be static for C library)
    static esp_err_t get_html_handler(httpd_req_t *req);
    static esp_err_t ws_handler(httpd_req_t *req);
};