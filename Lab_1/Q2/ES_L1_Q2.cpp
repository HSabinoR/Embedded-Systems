#include "mbed.h"

// Blinking rate in milliseconds
#define BLINKING_RATE     500ms
int time_count = 1;

int main()
{
    // Initialise the digital pin LED1/2/3 as an output
    DigitalOut led1(LED1);
    DigitalOut led2(LED2);
    DigitalOut led3(LED3);

    while (true) {
        // turns on after 1s and turns off after 1s
        if (time_count == 2 || time_count ==4) {
            led1 = !led1;
        }

        // turns on after 2s and turns off after 2s
        if (time_count == 4) {
            led3 = !led3;
        }
        // turns on after 500ms and turns off after 500ms
        led2 = !led2;

        ThisThread::sleep_for(BLINKING_RATE);

        if (time_count >= 4) {
            time_count = 0; // resets counter
        }
        time_count++;
    }
}
