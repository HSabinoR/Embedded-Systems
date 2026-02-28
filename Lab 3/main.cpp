#include "mbed.h"
#include "arm_book_lib.h"
#include <cstdio>

Timer monitoring_timer;
chrono::microseconds prev_time = 0ms;

DigitalIn simulate_gas_bt(D2);
DigitalIn gas_state_bt(D3);
DigitalIn temp_state_bt(D4);
DigitalIn simulate_temp_bt(D5);
DigitalIn reset_alarm_bt(D6);
DigitalIn monitoring_bt(D7);

DigitalOut green_led(LED1);
DigitalOut blue_led(LED2);
DigitalOut red_led(LED3);

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

bool temp_alarm_state = false;
bool gas_alarm_state = false;
bool monitoring_state = false;

bool debounce(DigitalIn &button);
void init_inputs();
void init_outputs();
void monitoring_mode();
void reset_alarm();
void check_sim_state_bts();
void UART_Print(int c);

int main() {
    init_inputs();
    init_outputs();
    monitoring_timer.start();

    while(true) {
        check_sim_state_bts();

        if (debounce(gas_state_bt)) {
            UART_Print(1);
        }
        
        if (debounce(temp_state_bt)) {
            UART_Print(2);
        }

        if (debounce(monitoring_bt)) {
            monitoring_state = !monitoring_state;
            UART_Print(3);
            prev_time = monitoring_timer.elapsed_time();
        }

        if (monitoring_state) {
            monitoring_mode();
        }

        if (debounce(reset_alarm_bt)) {
            reset_alarm();
        }
    }
}

bool debounce(DigitalIn &button) {
    if (button) {
        ThisThread::sleep_for(20ms);
        if (button) {
            while (button);
            return true;
        }
    }
    return false;
}

void init_inputs() {
    simulate_gas_bt.mode(PullDown);
    gas_state_bt.mode(PullDown);
    temp_state_bt.mode(PullDown);
    simulate_temp_bt.mode(PullDown);
    reset_alarm_bt.mode(PullDown);
    monitoring_bt.mode(PullDown);
}

void init_outputs() {
    green_led = OFF;
    blue_led = OFF;
    red_led = OFF;
}

void monitoring_mode() {
    if (monitoring_timer.elapsed_time() - prev_time >= 2s) {
        UART_Print(1);
        UART_Print(2);
        prev_time = monitoring_timer.elapsed_time();
    }
}

void reset_alarm() {
    temp_alarm_state = false;
    gas_alarm_state = false;

    UART_Print(4);
}

void check_sim_state_bts() {
    if (debounce(simulate_gas_bt)) {
        gas_alarm_state = !gas_alarm_state;
    } 
    if (debounce(simulate_temp_bt)) {
        temp_alarm_state = !temp_alarm_state;
    }
}

void UART_Print(int c) {
    switch (c) {
        case 1:
            if (gas_alarm_state) uartUsb.write("Gas Alarm Activated: true\r\n", 28);
            else uartUsb.write("Gas Alarm Activated: false\r\n", 29);
            break;
        case 2:
            if (temp_alarm_state) uartUsb.write("Temp Alarm Activated: true\r\n", 30);
            else uartUsb.write("Temp Alarm Activated: false\r\n", 30);
            break;
        case 3:
            if (monitoring_state) uartUsb.write("Monitoring Mode Activated!\r\n", 29);
            else uartUsb.write("Monitoring Mode Deactivated!\r\n", 31);
            break;
        case 4:
            uartUsb.write("Alarms reset!\r\n", 16);
            break;
        
    }
}
