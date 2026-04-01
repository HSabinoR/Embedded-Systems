#include "mbed.h"
#include <cstdint>
#include <string>

#include "arm_book_lib.h"

#include "display.h"
#include "motor.h"
#include "gate.h"

Timer run_timer;
chrono::microseconds alarm_time = 0ms;
chrono::microseconds prev_time = 0ms;
chrono::microseconds sys_time = 0ms;
chrono::microseconds prev_sys_time = 0ms;
#define NUMBER_OF_KEYS                           5
#define KEYPAD_NUMBER_OF_ROWS                    4
#define KEYPAD_NUMBER_OF_COLS                    4
#define DEBOUNCE_KEY_TIME_MS                    40
#define TIME_INCREMENT_MS                       10
#define BLINKING_ALARM_TIME                  200ms
#define DISPLAY_UPDATE_TIME                    30s
#define EVENT_NAME_MAX_LENGTH                   14
#define EVENT_MAX_STORAGE                        5

typedef enum {
    ALARM_ARMED,
    ALARM_TRIGGERED,
    ALARM_DEACTIVATED
} alarmState_t;

alarmState_t alarmState = ALARM_ARMED;

UnbufferedSerial uartUSB(USBTX, USBRX, 115200);

// Pinouts //
AnalogIn LM35(A1);
AnalogIn Pot(A0);
DigitalIn MQ2(PE_12);
DigitalOut alarm(PE_10);

DigitalOut green_led(LED1);
DigitalOut blue_led(LED2);
DigitalOut red_led(LED3);

DigitalOut keypadRowPins[KEYPAD_NUMBER_OF_ROWS] = {PB_3, PB_5, PC_7, PA_15};
DigitalIn keypadColPins[KEYPAD_NUMBER_OF_COLS]  = {PB_12, PB_13, PB_15, PC_6};


// Sensor Variables //
float LM35_TempC = 0.0f;
float Pot_Reading = 0.0f;
float Prev_Pot_Reading = 0.0f;
float TempThreshold = 33.0f;

bool gas_detected = false;
bool temp_high = false;

// Matrix Keypad Varibles //
char codeSequence[NUMBER_OF_KEYS]   = { '1', '8', '0', '5', '5' };
char keyPressed[NUMBER_OF_KEYS] = { '0', '0', '0', '0', '0' };
int incorrectAttempts = 0;
static bool messagePrinted = false;
char key = '\0';

typedef enum {
    MATRIX_KEYPAD_SCANNING,
    MATRIX_KEYPAD_DEBOUNCE,
    MATRIX_KEYPAD_KEY_HOLD_PRESSED
} matrixKeypadState_t;

matrixKeypadState_t matrixKeypadState;
int accumulatedDebounceMatrixKeypadTime = 0;
int matrixKeypadCodeIndex = 0;
char matrixKeypadLastKeyPressed = '\0';
char matrixKeypadIndexToCharArray[] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D',
};

// Event Logger variables //
int event_array_index = 0;
int total_events = 0;
bool event_logged = false;

typedef struct systemEvent {
    int timestamp_ms;
    char typeOfEvent[EVENT_NAME_MAX_LENGTH];
} systemEvent_t;

systemEvent_t arrayOfStoredEvents[EVENT_MAX_STORAGE];

bool blocked = false;

// Functions //
void hardware_Init(void); // Initializes all the components

// Sensor Reading Functions
void readInputs(void);
float read_LM35(void);
float read_Potentiometer(void);
void  read_MQ2(void);

// Keypad Functions
void matrixKeypadInit(void);
char matrixKeypadUpdate(void);
char matrixKeypadScan(void);

// Main Fucntions + Misc.
void  systemNormal(void);
void  uartPrint(const char* str);
void  Tasks(void);

// Alarm Functions
void alarm_management(void);

// Alarm Triggered Functions //
void checkAlarmTrigger(void); 
void displayAlarmTriggered(void);

// Alarm Deactivation Functions//
void handleAlarmTriggered(void); 
void handleBlinking(void); 
void handleCodeEntry(void); 
void correctCodeEntry(void);
void incorrectCodeEntry(void);
void blockSystem(void);

// Alarm Deactivated Functions //
void handleAlarmDeactivated(void);
void showWaitingMessage(void); 
void checkRearmCondition(void);

// Event Log Functions //
void eventlog_updater(const char* eventName); 
void printEventLog(void);
void log_Event(void);

// Motor Functions // 
void update_Motor_activity_on_Display(void);
void keypad_Gate_control(void);

// Main Loop //
int main() {
    hardware_Init();

    // Main System Loop //
    uartPrint("Starting system!\r\n");
    while (!blocked) {
        Tasks();
        delay(10);
    }
}

void Tasks() {
    readInputs();
    alarm_management();
    motorControlUpdate();
    keypad_Gate_control();

    update_Motor_activity_on_Display();

    if (key != '\0' && key == '#') {
        printEventLog();
    }

    if (alarmState == ALARM_ARMED) {
        systemNormal();
    }
    
}

void hardware_Init() {
    // Component Init // 
    run_timer.start();
    
    displayInit( DISPLAY_CONNECTION_I2C_PCF8574_IO_EXPANDER );
    
    matrixKeypadInit();
    motorControlInit();
    gateInit();
    
    red_led = OFF;
    green_led = OFF;
    blue_led = OFF;
    
    displayCharPositionWrite(0,0);
    displayStringWrite("HOME SECURITY SYSTEM");
}

void alarm_management() {
    switch (alarmState) {

    case ALARM_ARMED:
        checkAlarmTrigger();
        break;
    case ALARM_TRIGGERED:
        handleAlarmTriggered();
        break;
    case ALARM_DEACTIVATED:
        handleAlarmDeactivated();
        break;
    }
}

// #### Alarm Triggered Functions #### //
void checkAlarmTrigger() {
    if (temp_high || gas_detected) {
        log_Event();

        alarmState = ALARM_TRIGGERED;
        alarm = ON;

        displayAlarmTriggered();
    }
}

void displayAlarmTriggered() {
    uartPrint("ALARM TRIGGERED\r\n");
    uartPrint("Enter 4-Digit Code to Deactivate\r\n");
    
    clearDisplay();
    displayCharPositionWrite (0,1);
    if (temp_high) {
        displayStringWrite("High Temp Detected!");
    } else if (gas_detected) {
        displayStringWrite("Gas Detected!");
    }
    
    displayCharPositionWrite (0,2);
    displayStringWrite("Enter Code To");

    displayCharPositionWrite (0,3);
    displayStringWrite("Deactivate!");
}
////////////////////////////////////////

// #### Alarm Deactivation Functions #### //
void handleAlarmTriggered() {
    handleBlinking();
    handleCodeEntry();
}

void handleBlinking() {
    alarm_time = run_timer.elapsed_time();
    if ((alarm_time - prev_time) > BLINKING_ALARM_TIME) {
        red_led = !red_led;
        if (gas_detected) {
            blue_led = !blue_led;
        } else {
            blue_led = OFF;
        }
        if (temp_high) {
            green_led = !green_led;
        } else {
            green_led = OFF;
        }
        prev_time = alarm_time;
    }
}

void handleCodeEntry() {
    if (key == '\0') {
        return;
    }
    char buffer[3] = {key, '\n'};

    if (key != '\0') {

        char buffer[3] = { key, '\n'};
        uartPrint("Key Entered: ");
        uartPrint(buffer);

        keyPressed[matrixKeypadCodeIndex++] = key;
        
        if (matrixKeypadCodeIndex == NUMBER_OF_KEYS) {

            matrixKeypadCodeIndex = 0;
            if (strncmp(keyPressed, codeSequence, NUMBER_OF_KEYS) == 0) {
                correctCodeEntry();
            } else {
                incorrectCodeEntry();
            }
        }
    }

}

void correctCodeEntry() {
    alarmState = ALARM_DEACTIVATED;
    alarm = OFF;
    incorrectAttempts = 0;
    red_led = OFF;
    green_led = OFF;
    blue_led = OFF;

    uartPrint("Correct Code - Alarm Deactivated\r\n");
}

void incorrectCodeEntry() {
    incorrectAttempts++;

    uartPrint("Incorrect Code\r\n");

    if (incorrectAttempts >= 3) {
        blockSystem();
    }
}

void blockSystem() {
    red_led = ON;
    blue_led = ON;
    green_led = ON;
    uartPrint("Too Many Incorrect Attempts - LED ON\r\n");
    
    clearDisplay();
    displayCharPositionWrite (1,1);
    displayStringWrite("Too Many Attempts!");

    displayCharPositionWrite (1,2);
    displayStringWrite("!!System Blocked!!");
    blocked = true;
}

////////////////////////////////////////

// #### Alarm Deactivated Functions #### //
void handleAlarmDeactivated() {
    showWaitingMessage();
    checkRearmCondition();
}

void showWaitingMessage() {
    if (!messagePrinted && (temp_high || gas_detected)) {
        uartPrint("Waiting for system to stabilise...\r\n");

        clearDisplay();
        displayCharPositionWrite(0,1);
        displayStringWrite("Waiting for reset...");

        messagePrinted = true;
    }
}

void checkRearmCondition()
{
    if (!temp_high && !gas_detected)
    {
        alarmState = ALARM_ARMED;

        uartPrint("System Re-Armed\r\n");

        clearDisplay();
        displayCharPositionWrite(0,1);
        displayStringWrite("System Re-Armed");

        messagePrinted = false;
        event_logged = false;
    }
}

////////////////////////////////////////

// #### Event Log Functions #### //
void eventlog_updater(const char* eventName) {
    if (event_array_index < EVENT_MAX_STORAGE) {

        arrayOfStoredEvents[event_array_index].timestamp_ms =
            chrono::duration_cast<chrono::milliseconds>( run_timer.elapsed_time() ).count();

        strncpy(arrayOfStoredEvents[event_array_index].typeOfEvent,
                eventName, EVENT_NAME_MAX_LENGTH);

        event_array_index = (event_array_index + 1) % EVENT_MAX_STORAGE;

        if (total_events < EVENT_MAX_STORAGE) {
            total_events++;
        }
    }
}


void log_Event() {
    if (!event_logged) {

        if (temp_high) {
            eventlog_updater("HIGH_TEMP");
        }

        if (gas_detected) {
            eventlog_updater("GAS_LEAK");
        }

        event_logged = true;
    }
}

void printEventLog() {

    char buffer[64];
    uartPrint("\r\n--- Event Log ---\r\n");

    for (int i = 0; i < total_events; i++) {

        int index = (event_array_index - total_events + i + EVENT_MAX_STORAGE) % EVENT_MAX_STORAGE;

        int ms = arrayOfStoredEvents[index].timestamp_ms;
        int seconds = (ms / 1000) % 60;
        int minutes = (ms / (1000 * 60)) % 60;
        int hours   = (ms / (1000 * 60 * 60)) % 24;
        int days    = ms / (1000 * 60 * 60 * 24);
        int ms_part = ms % 1000;

        snprintf(buffer, sizeof(buffer),
            "Time: %dd %02dh %02dm %02ds %03dms | Event: %s\r\n",
            days, hours, minutes, seconds, ms_part,
            arrayOfStoredEvents[index].typeOfEvent);

        uartPrint(buffer);
    }
}
////////////////////////////////////////////////

// #### Matric Keypad Functions #### //
void matrixKeypadInit() {
    matrixKeypadState = MATRIX_KEYPAD_SCANNING;
    int pinIndex = 0;
    for( pinIndex=0; pinIndex<KEYPAD_NUMBER_OF_COLS; pinIndex++ ) {
        (keypadColPins[pinIndex]).mode(PullUp);
    }
}

char matrixKeypadScan() {
    int row = 0;
    int col = 0;
    int i = 0;

    for( row=0; row<KEYPAD_NUMBER_OF_ROWS; row++ ) {

        for( i=0; i<KEYPAD_NUMBER_OF_ROWS; i++ ) {
            keypadRowPins[i] = ON;
        }

        keypadRowPins[row] = OFF;

        for( col=0; col<KEYPAD_NUMBER_OF_COLS; col++ ) {
            if( keypadColPins[col] == OFF ) {
                return matrixKeypadIndexToCharArray[row*KEYPAD_NUMBER_OF_ROWS + col];
            }
        }
    }
    return '\0';
}

char matrixKeypadUpdate() {
    char keyDetected = '\0';
    char keyReleased = '\0';

    switch( matrixKeypadState ) {

    case MATRIX_KEYPAD_SCANNING:
        keyDetected = matrixKeypadScan();
        if( keyDetected != '\0' ) {
            matrixKeypadLastKeyPressed = keyDetected;
            accumulatedDebounceMatrixKeypadTime = 0;
            matrixKeypadState = MATRIX_KEYPAD_DEBOUNCE;
        }
        break;

    case MATRIX_KEYPAD_DEBOUNCE:
        if( accumulatedDebounceMatrixKeypadTime >=
            DEBOUNCE_KEY_TIME_MS ) {
            keyDetected = matrixKeypadScan();
            if( keyDetected == matrixKeypadLastKeyPressed ) {
                matrixKeypadState = MATRIX_KEYPAD_KEY_HOLD_PRESSED;
            } else {
                matrixKeypadState = MATRIX_KEYPAD_SCANNING;
            }
        }
        accumulatedDebounceMatrixKeypadTime =
            accumulatedDebounceMatrixKeypadTime + TIME_INCREMENT_MS;
        break;

    case MATRIX_KEYPAD_KEY_HOLD_PRESSED:
        keyDetected = matrixKeypadScan();
        if( keyDetected != matrixKeypadLastKeyPressed ) {
            if( keyDetected == '\0' ) {
                keyReleased = matrixKeypadLastKeyPressed;
            }
            matrixKeypadState = MATRIX_KEYPAD_SCANNING;
        }
        break;

    default:
        matrixKeypadInit();
        break;
    }
    return keyReleased;
}
///////////////////////////////////////////

// #### Sensor Functions #### //
float read_LM35() {
    float adc = LM35.read();
    LM35_TempC = adc * 330.0f;

    temp_high = (LM35_TempC > TempThreshold);
    return LM35_TempC;
}

void read_MQ2() {
    gas_detected = !MQ2.read();
}

float read_Potentiometer() {
    float raw = Pot.read() * 100.0f;
    Pot_Reading = roundf(raw);
    
    if (Pot_Reading != Prev_Pot_Reading) {
        Prev_Pot_Reading = Pot_Reading;
        TempThreshold = Pot_Reading;
    }

    return Pot_Reading;
}

void readInputs() {
    read_LM35();
    read_MQ2();

    key = matrixKeypadUpdate();
}
///////////////////////////////////////////

// #### Motor Functions #### //
void update_Motor_activity_on_Display() {
    if (updateDisplay_MotorActivity_flag) {
        updateDisplay_MotorActivity_flag = false;
        display_motor_activity();
    }
}

void keypad_Gate_control() {
    if (key != '\0') {
        switch(key) {
        case 'A':
            gateCloseButtonCallback();
            uartPrint("Close Button Pressed!\n");
            break;
        case 'B':
            gateOpenButtonCallback();
            uartPrint("Open Button Pressed!\n");
            break;
        case 'C':
            gateCloseLimitSwitchCallback();
            uartPrint("Close Limit Button Pressed!\n");
            break;
        case 'D':
            gateOpenLimitSwitchCallback();
            uartPrint("Open Limit Button Pressed!\n");
            break;
        }
    }
}

////////////////////////////////////////////

void systemNormal() {
    char text[96];
    snprintf(text, sizeof(text),
        "Temp: %.1f C | Threshold: %.1f | System Normal\r\n",
        LM35_TempC, TempThreshold);
    //uartPrint(text);

    char display_text[7] = "";
    if (key != '\0') {
        if (key == '4') {
            clearDisplay();

            displayCharPositionWrite (0,1);
            displayStringWrite("Temperature: ");

            sprintf(display_text, "%.0f / %.0f", LM35_TempC, TempThreshold);

            displayCharPositionWrite (13,1);
            displayStringWrite(display_text);
        }

        if (key == '5') {
            clearDisplay();
            displayCharPositionWrite (0,1);
            displayStringWrite("Gas Detected: ");

            sprintf(display_text, "%s", gas_detected ? "TRUE" : "FALSE");

            displayCharPositionWrite (15,1);
            displayStringWrite(display_text);
        }
    }  

    sys_time = run_timer.elapsed_time();
    if ( (sys_time - prev_sys_time) > DISPLAY_UPDATE_TIME) {
        clearDisplay();

        displayCharPositionWrite (0,1);
        displayStringWrite("Alarm Activated: ");

        sprintf(display_text, "%s", alarm ? "TRUE" : "FALSE");

        displayCharPositionWrite (11,2);
        displayStringWrite(display_text);
        prev_sys_time = sys_time;
    }

}

void uartPrint(const char* str) {
    uartUSB.write(str, strlen(str));
}
