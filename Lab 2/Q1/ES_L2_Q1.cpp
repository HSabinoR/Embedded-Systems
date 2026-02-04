#include "mbed.h"
#include "arm_book_lib.h"

int pscd[4] = {5, 3, 2, 4};

DigitalIn buttons[] = {DigitalIn(D7), DigitalIn(D6), DigitalIn(D5), DigitalIn(D4), DigitalIn(D3), DigitalIn(D2)};

int read_button() {
    for (int i = 0; i < 6; i++) {
        if (buttons[i]) {
            return i;
        }
    }
    return -1;
}

int main() {
    DigitalOut green_led(LED1);
    DigitalOut blue_led(LED2);
    DigitalOut red_led(LED3);

    int entered_code[4];

    for (int i = 0; i < 6; i++) {
        buttons[i].mode(PullDown);
    }

    green_led = OFF;
    red_led = OFF;

    while (true) {

        for (int i = 0; i < 4; i++) {
            int value;
            do {
                value = read_button();
            } while (value == -1);

            entered_code[i] = value;
            ThisThread::sleep_for(300ms);
        }

        // Compare codes
        bool correct = true;
        for (int i = 0; i < 4; i++) {
            if (entered_code[i] != pscd[i]) {
                correct = false;
                break;
            }
        }

        if (correct) {
            green_led = ON;
        } else {
            red_led = ON;
        }

        break;
    }
}
