#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "WiFiManager.h"
#include "WebServerManager.h"
#include "MotorController.h"
#include "CanManager.h"
#include <cstring>

static const char *TAG = "MAIN_APP: ";
//WebServerManager webServer;


TwaiListener canListener;


/*
// Left motor initialization (IN1=14, IN2=27, ENA=13, Kanał PWM = 0)
MotorController leftMotor(14, 27, 13, LEDC_CHANNEL_0);

// Right motor initialization (IN3=25, IN4=26, ENB=33, Kanał PWM = 1)
MotorController rightMotor(25, 26, 33, LEDC_CHANNEL_1);

// Function called by server, when command from browser appears
void executeMotorCommand(const char* cmd) {
    // Standard speed: 100% (if the car is too quick, set it to e.g 80)
    int speed = 80; 

    if (strcmp(cmd, "L+") == 0) {
        leftMotor.setSpeed(speed);
    } 
    else if (strcmp(cmd, "L-") == 0) {
        leftMotor.setSpeed(-speed);
    } 
    else if (strcmp(cmd, "R+") == 0) {
        rightMotor.setSpeed(speed);
    } 
    else if (strcmp(cmd, "R-") == 0) {
        rightMotor.setSpeed(-speed);
    } 
    else if (strcmp(cmd, "STOP") == 0) {
        leftMotor.setSpeed(0);
        rightMotor.setSpeed(0);
    }
}
    
*/
extern "C" void app_main(void)
{
    WiFiManager wifi;
    wifi.startAP("ESP32_RC_CAR", "12345678");
    //webServer.start();
    
    ESP_LOGI(TAG, "System running!");

    canListener.begin();

    int simulated_distance = 100;

    while (true) {
        //webServer.broadcastDistance(simulated_distance);
        simulated_distance--;
        if(simulated_distance < 10) simulated_distance = 100;

        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}