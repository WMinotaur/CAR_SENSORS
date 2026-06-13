#include "MotorController.h"
#include "driver/gpio.h"
#include <cstdlib> 

MotorController::MotorController(int pin_in1, int pin_in2, int pin_pwm, ledc_channel_t pwm_channel)
    : _pin_in1(pin_in1), _pin_in2(pin_in2), _pin_pwm(pin_pwm), _pwm_channel(pwm_channel) {

    // 1. Konfiguracja zwykłych pinów GPIO do ustalania kierunku (IN1, IN2)
    gpio_reset_pin((gpio_num_t)_pin_in1);
    gpio_set_direction((gpio_num_t)_pin_in1, GPIO_MODE_OUTPUT);
    
    gpio_reset_pin((gpio_num_t)_pin_in2);
    gpio_set_direction((gpio_num_t)_pin_in2, GPIO_MODE_OUTPUT);

    // Zatrzymujemy silnik na starcie
    gpio_set_level((gpio_num_t)_pin_in1, 0);
    gpio_set_level((gpio_num_t)_pin_in2, 0);

    // 2. Konfiguracja sprzętowego timera dla PWM (LEDC)
    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.timer_num = LEDC_TIMER_0;
    timer_conf.duty_resolution = LEDC_TIMER_10_BIT; // Rozdzielczość 10 bitów (wartości 0-1023)
    timer_conf.freq_hz = 1000;                      // Częstotliwość 1 kHz (standard dla silników)
    timer_conf.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&timer_conf);

    // 3. Konfiguracja kanału PWM przypisanego do konkretnego pinu (ENA/ENB)
    ledc_channel_config_t channel_conf = {};
    channel_conf.gpio_num = _pin_pwm;
    channel_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    channel_conf.channel = _pwm_channel;
    channel_conf.timer_sel = LEDC_TIMER_0;
    channel_conf.duty = 0; // Wypełnienie 0 = zatrzymany
    channel_conf.hpoint = 0;
    ledc_channel_config(&channel_conf);
}

void MotorController::setSpeed(int speed) {
    // Zabezpieczenie przed podaniem wartości spoza zakresu -100 do 100
    if (speed > 100) speed = 100;
    if (speed < -100) speed = -100;

    // Ustalanie kierunku
    if (speed > 0) {
        gpio_set_level((gpio_num_t)_pin_in1, 1);
        gpio_set_level((gpio_num_t)_pin_in2, 0);
    } else if (speed < 0) {
        gpio_set_level((gpio_num_t)_pin_in1, 0);
        gpio_set_level((gpio_num_t)_pin_in2, 1);
    } else {
        // Hamowanie
        gpio_set_level((gpio_num_t)_pin_in1, 0);
        gpio_set_level((gpio_num_t)_pin_in2, 0);
    }

    // Skalowanie prędkości z procentów (0-100) na rozdzielczość bitową timera (0-1023)
    int duty = (abs(speed) * 1023) / 100;
    
    // Wysłanie nowej prędkości do sprzętu
    ledc_set_duty(LEDC_LOW_SPEED_MODE, _pwm_channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, _pwm_channel);
}