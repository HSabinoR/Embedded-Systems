#include "mbed.h"
#include "arm_book_lib.h"

UnbufferedSerial uartUSB(USBTX, USBRX, 115200);

AnalogIn LM35(A1);
AnalogIn Pot(A0);
DigitalIn MQ2(PE_12);
DigitalOut alarm(PE_10);

float LM35_TempC = 0.0f;
float Pot_Reading = 0.0f;
float Prev_Pot_Reading = 0.0f;
float TempThreshold = 50.0f;

bool gas_detected = false;
bool temp_high = false;

float read_LM35(void);
float read_Potentiometer(void);
void  read_MQ2(void);
void  change_threshold(void);
bool  sensor_checks(void);
void  systemNormal(void);
void  uartPrint(const char* str);
void  Tasks(void);

int main() {
    while (true) {
        Tasks();
        delay(100);
    }
}

void Tasks() {
    read_LM35();
    read_MQ2();
    read_Potentiometer();

    bool alarm_active = sensor_checks();

    if (!alarm_active) {
        change_threshold();
        systemNormal();
    }
}

float read_LM35(void) {
    float adc = LM35.read();
    LM35_TempC = adc * 330.0f;

    temp_high = (LM35_TempC > TempThreshold);
    return LM35_TempC;
}

void read_MQ2(void) {
    gas_detected = !MQ2.read();
}

float read_Potentiometer(void) {
    float raw = Pot.read() * 100.0f;
    Pot_Reading = roundf(raw);   // 1°C steps
    return Pot_Reading;
}

void change_threshold(void) {
    if (Pot_Reading != Prev_Pot_Reading) {
        Prev_Pot_Reading = Pot_Reading;
        TempThreshold = Pot_Reading;
    }
}

bool sensor_checks(void) {
    static bool prev_alarm_state = false;
    bool alarm_now = temp_high || gas_detected;

    if (alarm_now != prev_alarm_state) {
        prev_alarm_state = alarm_now;

        char text[96];

        if (alarm_now) {
            alarm = true;
            snprintf(text, sizeof(text),
                "Buzzer ON - Cause:\n"
                "\tHigh Temp: %s\n"
                "\tGas Detected: %s\r\n",
                temp_high ? "TRUE" : "FALSE",
                gas_detected ? "TRUE" : "FALSE");
        } else {
            alarm = false;
            snprintf(text, sizeof(text),
                "Buzzer OFF - System Normal\r\n");
        }

        uartPrint(text);
    }

    return alarm_now;
}

void systemNormal(void) {
    char text[96];
    snprintf(text, sizeof(text),
        "Temp: %.1f C | Threshold: %.1f | System Normal\r\n",
        LM35_TempC, TempThreshold);

    uartPrint(text);
}

void uartPrint(const char* str) {
    uartUSB.write(str, strlen(str));
}
