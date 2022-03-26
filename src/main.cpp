#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

#define STATE_MAIN 1
// #define STATE_OPTION 1
#define STATE_TIMEBOMB_READY_TIME 2
#define STATE_TIMEBOMB_READY_PASSWORD 3
#define STATE_TIMEBOMB_READY_CONFRIM 4
#define STATE_TIMEBOMB_START_BOMB 5
#define STATE_TIMEBOMB_RUN_BOMB 6
#define STATE_TIMEBOMB_CLEAR_PASSWORD 7
#define STATE_TIMEBOMB_CLEAR_BOMB 8
#define STATE_TIMEBOMB_CLEAR_RESULT 9
#define STATE_TIMEBOMB_EXPLOSION_BOMB 10

#define STATE_TIMEBOMB_NO_MATCH_PASSWORD 500

#define MAX_PASSWORD_LENGTH 6
#define MAX_TIME_LENGTH 6

#define LCD_MODULE 0x27
#define CLK 2
#define DIO 3

// KeyPad Setting ====================================================================
const byte rows = 4; //four rows
const byte cols = 4; //three columns
char keys[rows][cols] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};
byte rowPins[rows] = {9, 10, 11, 12}; //connect to the row pinouts of the keypad
byte colPins[cols] = {5, 6, 7, 8}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );
// ===================================================================================

LiquidCrystal_I2C lcd(LCD_MODULE,16,2);

static int state = STATE_MAIN;
static unsigned char time[MAX_TIME_LENGTH];
static unsigned char password[MAX_PASSWORD_LENGTH];
static unsigned char confirm[MAX_PASSWORD_LENGTH];

void resetState() {
    memset(time, 0, MAX_TIME_LENGTH);
    memset(password, 0, MAX_PASSWORD_LENGTH);
    memset(confirm, 0, MAX_PASSWORD_LENGTH);
}

int passwordConfirm( char key ) {
    static int initConfirm = 0;
    if ( key == '#' ) {
        resetState();
        initConfirm = 0;
        return STATE_MAIN;
    } else if ( initConfirm < MAX_PASSWORD_LENGTH ) {
        confirm[initConfirm] = key - '0';
        Serial.print("Set confirm: [");
        for ( int i = 0; i < MAX_PASSWORD_LENGTH; ++i ) {
            Serial.print(((String)confirm[i]) + ",");
        }
        Serial.println("]");
        ++initConfirm;
    } else {
        for (int i = 0; i < MAX_PASSWORD_LENGTH; ++i) {
            if ( password[i] != confirm[i] ){
                return STATE_TIMEBOMB_NO_MATCH_PASSWORD;
            }
        }

        initConfirm = 0;
        return STATE_TIMEBOMB_READY_CONFRIM;
    }
    return state;
}

int setPassword( char key ) {
    static int initPassword = 0;
    if ( key == '#' ) {
        resetState();
        initPassword = 0;
        return STATE_MAIN;
    } else if ( initPassword < MAX_PASSWORD_LENGTH ) {
        password[initPassword] = key - '0';
        Serial.print("Set Password: [");
        for ( int i = 0; i < MAX_PASSWORD_LENGTH; ++i ) {
            Serial.print(((String)password[i]) + ",");
        }
        Serial.println("]");
        ++initPassword;
    } else {
        initPassword = 0;
        return STATE_TIMEBOMB_READY_CONFRIM;
    }
    return state;
}

int setTime( char key ) {
    static int initTime = 0;
    if ( (int)key > 47 && (int)key < 58 ) {
        if ( initTime < MAX_TIME_LENGTH ) {
            time[initTime] = key - '0';
            Serial.print("Set Time: [");
            for ( int i = 0; i < MAX_TIME_LENGTH; ++i ) {
                Serial.print(((String)time[i]) + ",");
            }
            Serial.println("]");
            ++initTime;
        } else {
            initTime = 0;
            return STATE_TIMEBOMB_READY_PASSWORD;
        }
    } else if ( key == '*' ) {
        resetState();
        initTime = 0;
        return STATE_MAIN;
    } else if ( key == '#' ) {
        memset(time, 0, MAX_TIME_LENGTH);
        initTime = 0;
    }
    return state;
}

void displayPrint( int state ) {
    lcd.clear();
    switch (state) {
        case STATE_MAIN:
            lcd.setCursor(0, 0);
            lcd.print("TDF BOMB SELECT TO MODE");
            lcd.setCursor(0, 1);
            lcd.print("1. TIME BOMB");
            break;
        
        case STATE_TIMEBOMB_READY_TIME:
            lcd.setCursor(0, 0);
            lcd.print("SET TIME [#]BACK [*]CLEAR");
            lcd.setCursor(0, 1);
            lcd.print((String)time[0] + (String)time[1] + ":"
            + (String)time[2] + (String)time[3] + ":"
            + (String)time[4] + (String)time[5]);
            break;
        case STATE_TIMEBOMB_READY_PASSWORD:
            lcd.setCursor(0, 0);
            lcd.print("SET PASSWORD [#]BACK");
            lcd.setCursor(0, 1);
            lcd.print("[" + (String)((password[0]+'0') + (password[1]+'0') +
            (password[2]+'0') + (password[3]+'0') +
            (password[4]+'0') + (password[5]+'0')) + "]");
            break;
        case STATE_TIMEBOMB_READY_CONFRIM:
            lcd.setCursor(0, 0);
            lcd.print("PASSWORD CONFRIM");
            lcd.setCursor(0, 1);
            lcd.print("[" + (String)((confirm[0]+'0') + (confirm[1]+'0') +
            (confirm[2]+'0') + (confirm[3]+'0') +
            (confirm[4]+'0') + (confirm[5]+'0')) + "]");
            break;
        default:
            break;
    }
}

int keyboardScanner( char key ) {
    if ( key ) {
        if ( state == STATE_MAIN ) {
            switch (key) {
                case '1':
                    return STATE_TIMEBOMB_READY_TIME;
                default:
                    break;
            }
        } else if ( state == STATE_TIMEBOMB_READY_TIME ) {
            state = setTime( key );
        } else if ( state == STATE_TIMEBOMB_READY_PASSWORD ) {
            state = setPassword( key );
        } else if ( state == STATE_TIMEBOMB_READY_CONFRIM ) {
            state = passwordConfirm( key );
        } else if ( state == STATE_TIMEBOMB_NO_MATCH_PASSWORD ) {
            delay(3000);
            memset( confirm, 0, MAX_PASSWORD_LENGTH );
            state = STATE_TIMEBOMB_READY_CONFRIM;
        }
    }
    return state;
}

void setup() {
    lcd.init();
    lcd.backlight();
    Serial.begin(9600);
    Serial.println("Ready to BOMB");
}

void loop() {
    state = keyboardScanner( keypad.getKey() );
    displayPrint( state );
    delay(100);
}