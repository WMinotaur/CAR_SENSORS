# ESP32 RC Car Controller 

## Features
* **Access Point mode (SoftAP):** ESP32 creates it's own WiFi "ESP32_RC_CAR".
* **WebSocket Server:** Ensures low latency in transmission of car controlling packets and high sensor data refresh rate on WWW page.
* **Web Interface:** Responsive control panel written in HTML/JS.

## Architecture
* `main.cpp`: Program's entry point.
* `WiFiManager`: Class responsible for access point launch and initialization.
* `WebServerManager`: Class that manages HTTP server, routing URI and async WebSocket communication.

## Configuration and installation (ESP-IDF)
1. **Clone the repository**
2. **Turn on WebSockets:**
   * Go to`menuconfig`. If you're using VScode press `Ctrl + Shift + P` and go to `ESP-IDF: SDK Configuration Editor (menuconfig)`.
   * Search for `websocket`
   * Check the `Websocket server support` option and save.
3. **Building**
   * Build, flash and monitor by clicking on the fire icon.
4. **Connect to the WiFi**
	 * Connect to the `ESP32_RC_CAR_` WiFi with your mobile device. The password is "12345678"
5. **Go to 192.168.4.1 address in your browser** 
6. **Steer the car**
	 * When you use the control panel the following messages should appear in your serial monitor: <br/>
	 ======================================<br/>
	 
		 `I (14402) WEB_SRV: Steering signal: R+` <br/>
	`I (15212) WEB_SRV: Steering signal: STOP`<br/>
	`I (16192) WEB_SRV: Steering signal: L-`<br/>
	`I (16802) WEB_SRV: Steering signal: STOP`<br/>
	`I (17262) WEB_SRV: Steering signal: L+`<br/>
	`I (17802) WEB_SRV: Steering signal: STOP`<br/>
	`I (18482) WEB_SRV: Steering signal: R-`<br/>
	`I (19002) WEB_SRV: Steering signal: STOP`<br/>

		======================================<br/>
	 R+ - Righ wheel accelerates forwards<br/>
	 R- - Right wheel accelerates backwards<br/>
	 L+ - Left wheel accelerates forwards<br/>
	 L-   - Left wheel accelerates backwards<br/>
	 STOP - Acceleration stops<br/>
   