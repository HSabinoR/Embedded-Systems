#include "mbed.h"
#include <cstdint>

// Blinking rate in milliseconds
#define BLINKING_RATE     200ms

void blink_leds_x5(DigitalOut led_arr[]) {
    for (int i = 0; i <= 4; i++) {
        for (int i = 0; i <= 2; i++) {
                led_arr[i] = true;
            }
        ThisThread::sleep_for(BLINKING_RATE);
        for (int i = 0; i <= 2; i++) {
            led_arr[i] = false;
        }
        ThisThread::sleep_for(BLINKING_RATE);
    }
}

int main()
{
    // Initialise the digital pin LED1/2/3 as an output
    DigitalOut led1(LED1);
    DigitalOut led2(LED2);
    DigitalOut led3(LED3);

    DigitalOut led_array[] = {led1, led2, led3};

    blink_leds_x5(led_array);

    led1 = true;
    led2 = false;
    led3 = false; 
}
