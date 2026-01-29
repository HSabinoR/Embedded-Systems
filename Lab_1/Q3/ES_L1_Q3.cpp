#include "mbed.h"

// Blinking rate in milliseconds
#define BLINKING_RATE     200ms

void blink_led(DigitalOut& led) {
    led = true;
    ThisThread::sleep_for(BLINKING_RATE);
    led = false;
    ThisThread::sleep_for(BLINKING_RATE);
}

int main()
{
    // Initialise the digital pin LED1/2/3 as an output
    DigitalOut led1(LED1);
    DigitalOut led2(LED2);
    DigitalOut led3(LED3);

    DigitalOut* led_array[] = {&led1, &led2, &led3};

    while (true) {
        for (int i = 0; i <= 2; i++) {
            blink_led(*led_array[i]);
        }

        for (int i = 1; i > 0; i--) {
            blink_led(*led_array[i]);
        }
    }
}
