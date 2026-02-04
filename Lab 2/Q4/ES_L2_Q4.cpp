#include "mbed.h"
#include "arm_book_lib.h"

Timer lockdown_timer;

int pscd[4] = {5, 3, 2, 4};

DigitalIn buttons[] = {DigitalIn(D7), DigitalIn(D6), DigitalIn(D5), DigitalIn(D4), DigitalIn(D3), DigitalIn(D2)};

bool system_done = false;

// Blinking rate in milliseconds
#define BLINKING_RATE_1 200ms
#define BLINKING_RATE_2 500ms

void blink_led(DigitalOut& led, chrono::milliseconds rate) {
    led = ON;
    ThisThread::sleep_for(rate);
    led = OFF;
    ThisThread::sleep_for(rate);
}

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

    bool correct = true;
    while (!system_done) {
        green_led = OFF;
        blue_led = OFF;
        red_led = OFF;

        for (int tries = 1; tries<=3;) {
            for (int i = 0; i < 4; i++) {
                int value;
                do {
                    value = read_button();
                } while (value == -1);

                entered_code[i] = value;
                ThisThread::sleep_for(300ms);
            }

            // Compare codes
            correct = true;
            for (int i = 0; i < 4; i++) {
                if (entered_code[i] != pscd[i]) {
                    correct = false;
                    break;
                }
            }

            if (correct) {
                green_led = ON;
                break;
            } else {
                blink_led(red_led, BLINKING_RATE_1);
                tries++;
            }        
        }

        if (!correct) {
            // Warning period
            for (int i = 0; i <= 30; i++) { // 30 secs / (500ms * 2) = 30x blinks
                blink_led(blue_led, BLINKING_RATE_2);
            }

            for (int i = 0; i < 4; i++) {
                int value;
                do {
                    value = read_button();
                } while (value == -1);

                entered_code[i] = value;
                ThisThread::sleep_for(300ms);
            }

            // Compare codes
            correct = true;
            for (int i = 0; i < 4; i++) {
                if (entered_code[i] != pscd[i]) {
                    correct = false;
                    break;
                }
            }
        }

        // Lock Down mode. Can't use sleep_for() as it blocks the code from running.
        if (!correct && !system_done) {
            red_led = ON;
            chrono::microseconds blink_prev_time = 0ms;
            int admin_pscd[4] = {1, 2, 3, 4};
            int entered_code_admin[4];
            int admin_index = 0;

            lockdown_timer.reset();
            lockdown_timer.start();
            while (!correct) {
                if (lockdown_timer.elapsed_time() - blink_prev_time >= 500ms) {
                    blue_led = !blue_led;
                    blink_prev_time = lockdown_timer.elapsed_time();
                }

                int value = read_button();
                if (value != -1) {
                    entered_code_admin[admin_index++] = value;
                    ThisThread::sleep_for(300ms);

                    if (admin_index == 4) {
                        correct = true;
                        for (int i = 0; i < 4; i++) {
                            if (entered_code_admin[i] != admin_pscd[i]) {
                                correct = false;
                                break;
                            }
                        }
                        admin_index = 0;
                    }
                }

                if (lockdown_timer.elapsed_time() >= 60s) {
                    lockdown_timer.reset();
                    green_led = ON;
                    blue_led  = ON;
                    red_led   = ON;
                    system_done = true;
                }
            }
        }
    }
}
