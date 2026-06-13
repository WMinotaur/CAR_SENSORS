#pragma once
#include "driver/ledc.h"

class MotorController {
public:
    // Konstruktor: podajesz piny kierunku, pin prędkości i numer kanału PWM (0-7)
    MotorController(int pin_in1, int pin_in2, int pin_pwm, ledc_channel_t pwm_channel);
    
    // Ustawia prędkość: od -100 (pełna wstecz) do 100 (pełna naprzód), 0 to stop
    void setSpeed(int speed);

private:
    int _pin_in1;
    int _pin_in2;
    int _pin_pwm;
    ledc_channel_t _pwm_channel;
};