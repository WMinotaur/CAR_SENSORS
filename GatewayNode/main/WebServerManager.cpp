#include "WebServerManager.h"
#include "esp_log.h"
#include <cstring>
#include <cstdio>

static const char* TAG = "WEB_SRV";

extern void executeMotorCommand(const char* cmd);

//html code for the web page 
const char* html_page = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <meta charset="utf-8">
  <title>RC car controller</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background-color: #222; color: white; margin: 0; padding: 20px; 
           -webkit-user-select: none; user-select: none; }
    h2 { color: #4CAF50; font-size: 20px;}
    .container { display: flex; justify-content: space-between; align-items: center; max-width: 500px; margin: 30px auto; }
    .controls { display: flex; flex-direction: column; gap: 15px; }
    .btn { background-color: #4CAF50; border: none; color: white; padding: 15px; font-size: 24px; border-radius: 10px; cursor: pointer; width: 70px; height: 70px; touch-action: manipulation; -webkit-tap-highlight-color: transparent; }
    .btn:active { background-color: #3e8e41; transform: scale(0.90); }
    .center-display { font-size: 18px; font-weight: bold; padding: 15px; border: 2px solid #555; border-radius: 10px; background-color: #333; width: 120px;}
  </style>
</head>
<body>
  <h2>Control panel</h2>
  <div class="container">
    <div class="controls">
      <button class="btn" onmousedown="sendCommand('L+')" onmouseup="sendCommand('STOP')" onmouseleave="sendCommand('STOP')" ontouchstart="sendCommand('L+'); event.preventDefault();" ontouchend="sendCommand('STOP'); event.preventDefault();">+</button>
      <button class="btn" onmousedown="sendCommand('L-')" onmouseup="sendCommand('STOP')" onmouseleave="sendCommand('STOP')" ontouchstart="sendCommand('L-'); event.preventDefault();" ontouchend="sendCommand('STOP'); event.preventDefault();">-</button>
    </div>
    <div class="center-display">
      Distance:<br><span id="distance" style="color: #4CAF50; font-size: 28px;">---</span> cm
    </div>
    <div class="controls">
      <button class="btn" onmousedown="sendCommand('R+')" onmouseup="sendCommand('STOP')" onmouseleave="sendCommand('STOP')" ontouchstart="sendCommand('R+'); event.preventDefault();" ontouchend="sendCommand('STOP'); event.preventDefault();">+</button>
      <button class="btn" onmousedown="sendCommand('R-')" onmouseup="sendCommand('STOP')" onmouseleave="sendCommand('STOP')" ontouchstart="sendCommand('R-'); event.preventDefault();" ontouchend="sendCommand('STOP'); event.preventDefault();">-</button>
    </div>
  </div>
  <script>
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;
    function initWebSocket() { websocket = new WebSocket(gateway); websocket.onmessage = onMessage; }
    function onMessage(event) { document.getElementById('distance').innerHTML = event.data; }
    function sendCommand(command) { if(websocket.readyState === WebSocket.OPEN) { websocket.send(command); } }
    window.onload = initWebSocket;
  </script>
</body>
</html>
)rawliteral";

WebServerManager::WebServerManager() : server(NULL) {}

// HTTP GET request handler
esp_err_t WebServerManager::get_html_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handling data coming from WebSocket
esp_err_t WebServerManager::ws_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) return ESP_OK; // Handshake

  httpd_ws_frame_t ws_pkt = {};
  ws_pkt.type = HTTPD_WS_TYPE_TEXT;

  // Packet length check
  esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
  if (ret != ESP_OK) return ret;

  if (ws_pkt.len > 0) {
    // Allocating memory for message from browser
    uint8_t *buf = (uint8_t*)calloc(1,ws_pkt.len + 1);
    ws_pkt.payload = buf;

    // Downloading actual data
    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret == ESP_OK) {
      ESP_LOGI(TAG, "Steering signal: %s", ws_pkt.payload);
      
      // Sending command to main.cpp file
      executeMotorCommand((const char*)ws_pkt.payload);
    }
    free(buf);
  }
  return ret;
}

void WebServerManager::start() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();


  ESP_LOGI(TAG, "Launching server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {

    // Endpoint config "/" (Home screen)
    httpd_uri_t uri_get = {};
    uri_get.uri = "/";
    uri_get.method = HTTP_GET;
    uri_get.handler = get_html_handler;
    uri_get.user_ctx = this;
    httpd_register_uri_handler(server, &uri_get);

    // Endpoint config "/ws" (WebSockets)
    httpd_uri_t ws = {};
    ws.uri = "/ws";
    ws.method = HTTP_GET;
    ws.handler = ws_handler;
    ws.user_ctx = this;
    ws.is_websocket = true; // Flag saying it's a websocket
    httpd_register_uri_handler(server, &ws);
  }
}

// Function sending data from the sensor to the browser
void WebServerManager::broadcastDistance(int distance) {
  if (!server)return;

  char buff[16];
  snprintf(buff, sizeof(buff), "%d", distance);

  httpd_ws_frame_t ws_pkt = {};
  ws_pkt.payload = (uint8_t*)buff;
  ws_pkt.len = strlen(buff);
  ws_pkt.type = HTTPD_WS_TYPE_TEXT;

  // Sending frame asynchronously
  size_t clients = 8;
  int client_fds[8];
  if (httpd_get_client_list(server, &clients,client_fds) == ESP_OK) {
    for (size_t i = 0; i < clients; ++i) {
      httpd_ws_client_info_t client_info = httpd_ws_get_fd_info(server, client_fds[i]);
      if (client_info == HTTPD_WS_CLIENT_WEBSOCKET) {
        httpd_ws_send_frame_async(server, client_fds[i], &ws_pkt);
      }
    }
  }
}