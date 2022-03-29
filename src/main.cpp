#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <TM1637Display.h>

#define STATE_MAIN 1
// #define STATE_OPTION 1
#define STATE_TIMEBOMB_READY_TIME 2
#define STATE_TIMEBOMB_READY_PASSWORD 3
#define STATE_TIMEBOMB_READY_CONFRIM 4
#define STATE_TIMEBOMB_START_BOMB 5
#define STATE_TIMEBOMB_RUN_BOMB 6
#define STATE_TIMEBOMB_CLEAR_PASSWORD 7
#define STATE_TIMEBOMB_CLEAR_BOMB 8
#define STATE_TIMEBOMB_CLEAR_BOMB_FAILED 9
#define STATE_TIMEBOMB_CLEAR_RESULT 10
#define STATE_TIMEBOMB_EXPLOSION_BOMB 11

#define STATE_TIMEBOMB_NO_MATCH_PASSWORD 500

#define MAX_PASSWORD_LENGTH 6
#define MAX_TIME_LENGTH 4

#define LCD_MODULE 0x27
#define CLK 2
#define DIO 3

#define SOUND_MODULE 4

// TM1637Display Setting ============================================================
const uint8_t SEG_DONE[] = {
	SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
	SEG_C | SEG_E | SEG_G,                           // n
	SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
	};

TM1637Display display(CLK, DIO);
uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
uint8_t blank[] = { 0x00, 0x00, 0x00, 0x00 };

// ===================================================================================

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

// LCD Setting =======================================================================
LiquidCrystal_I2C lcd(LCD_MODULE,16,2);
// ===================================================================================

int state = STATE_MAIN;
unsigned char time[MAX_TIME_LENGTH];
char password[MAX_PASSWORD_LENGTH] = {'-','-','-','-','-','-'};
char clear_password[MAX_PASSWORD_LENGTH] = {'-','-','-','-','-','-'};
char confirm[MAX_PASSWORD_LENGTH] = {'-','-','-','-','-','-'};

long second = 0;
long currentTime = 0;

int outputTime = 0;

boolean isActiveTimer = false;

void resetState() {
    memset(time, 0, MAX_TIME_LENGTH);
    memset(password, '-', MAX_PASSWORD_LENGTH);
    memset(confirm, '-', MAX_PASSWORD_LENGTH);
    memset(clear_password, '-', MAX_PASSWORD_LENGTH);
}

int passwordConfirm( char key ) {
    static int initConfirm = 0;
    if ( key == '#' ) {
        resetState();
        initConfirm = 0;
        return STATE_MAIN;
    } else if ( initConfirm < MAX_PASSWORD_LENGTH ) {
        confirm[initConfirm] = key;
        Serial.print("Set confirm: [");
        for ( int i = 0; i < MAX_PASSWORD_LENGTH; ++i ) {
            Serial.print(((String)confirm[i]) + ",");
        }
        Serial.println("]");
        ++initConfirm;
    } else {
        for (int i = 0; i < MAX_PASSWORD_LENGTH; ++i) {
            if ( password[i] != confirm[i] ){
                initConfirm = 0;
                resetState();
                return STATE_TIMEBOMB_NO_MATCH_PASSWORD;
            }
        }

        initConfirm = 0;
        return STATE_TIMEBOMB_START_BOMB;
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
        password[initPassword] = key;
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


int inputPassword( char key ) {
    static int initClearPassword = 0;
    if ( key == '#' ) {
        resetState();
        initClearPassword = 0;
        return STATE_TIMEBOMB_RUN_BOMB;
    } else if ( initClearPassword < MAX_PASSWORD_LENGTH ) {
        clear_password[initClearPassword] = key;
        Serial.print("Set Password: [");
        for ( int i = 0; i < MAX_PASSWORD_LENGTH; ++i ) {
            Serial.print(((String)clear_password[i]) + ",");
        }
        Serial.println("]");
        ++initClearPassword;
    } else {
        for (int i = 0; i < MAX_PASSWORD_LENGTH; ++i) {
            if ( password[i] != clear_password[i] ){
                initClearPassword = 0;
                memset( clear_password, '-', MAX_PASSWORD_LENGTH );
                return STATE_TIMEBOMB_CLEAR_BOMB_FAILED;
            }
        }

        initClearPassword = 0;
        return STATE_TIMEBOMB_CLEAR_BOMB;
    }
    return state;
}

void setTimemer() {
    outputTime = ((int)((char)time[3])) + ((int)((char)time[2])*10) + ((int)((char)time[1])*100) + ((int)((char)time[0])*1000);
    display.showNumberDecEx(outputTime, 0xff, true);
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
            setTimemer();
            return STATE_TIMEBOMB_READY_PASSWORD;
        }
    } else if ( key == '*' ) {
        resetState();
        initTime = 0;
        return STATE_MAIN;
    } else if ( key == '#' ) {
        memset(time, 48, MAX_TIME_LENGTH);
        initTime = 0;
    }
    return state;
}

void displayPrint() {
    lcd.clear();
    Serial.println( "state : " + (String)state );
    switch (state) {
        case STATE_MAIN:
            lcd.setCursor(0, 0);
            lcd.print("TDF BOMB SELECT TO MODE");
            lcd.setCursor(0, 1);
            lcd.print("1. TIME BOMB");
            break;
        
        case STATE_TIMEBOMB_READY_TIME:
            lcd.setCursor(0, 0);
            lcd.print("SET TIME");
            lcd.setCursor(0, 1);
            lcd.print((String)time[0] + (String)time[1] + ":"
            + (String)time[2] + (String)time[3]);
            break;
        case STATE_TIMEBOMB_READY_PASSWORD: {
            String pass = "[";
            lcd.setCursor(0, 0);
            lcd.print("SET PASSWORD");
            lcd.setCursor(0, 1);
            for ( int i = 0; i < MAX_PASSWORD_LENGTH; ++i ) {
                pass += password[i];
            }
            pass += "]";
            lcd.print(pass);
            break;
        }
        case STATE_TIMEBOMB_READY_CONFRIM: {
            String conf = "[";
            lcd.setCursor(0, 0);
            lcd.print("PASSWORD CONFRIM");
            lcd.setCursor(0, 1);
            for ( int i = 0; i < MAX_PASSWORD_LENGTH; ++i ) {
                conf += confirm[i];
            }
            conf += "]";
            lcd.print(conf);
            break;
        }
        case STATE_TIMEBOMB_NO_MATCH_PASSWORD: {
            lcd.setCursor(0, 0);
            lcd.print("NO MACH PASSWORD!");
            delay(2000);
            state = STATE_MAIN;
            resetState();
            displayPrint();
            break;
        }
        case STATE_TIMEBOMB_START_BOMB: {
            lcd.setCursor(0, 0);
            lcd.print("READY TO BOMB...");
            for ( int i = 5; i > 0; --i ) {
                lcd.setCursor(0, 1);
                lcd.print( (String)i + "...");
                tone(SOUND_MODULE, 3000);
                delay(100);
                noTone(SOUND_MODULE);
                delay(900);
            }
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("PROCESSING BOMB!!");
            tone(SOUND_MODULE, 3000);
            delay(1000);
            noTone(SOUND_MODULE);
            state = STATE_TIMEBOMB_RUN_BOMB;
            currentTime = millis();
            isActiveTimer = true;
            displayPrint();
            break;
        }
        case STATE_TIMEBOMB_RUN_BOMB:
            lcd.setCursor(0, 0);
            lcd.print("BOMB TO RUNNING.");
            lcd.setCursor(0, 1);
            lcd.print("PRESS \"C\" KEY");
            break;
        case STATE_TIMEBOMB_CLEAR_PASSWORD: {
            String c_pass = "[";
            lcd.setCursor(0, 0);
            lcd.print("PASSWORD");
            lcd.setCursor(0, 1);
            for ( int i = 0; i < MAX_PASSWORD_LENGTH; ++i ) {
                c_pass += clear_password[i];
            }
            c_pass += "]";
            lcd.print(c_pass);
            break;
        }
        case STATE_TIMEBOMB_CLEAR_BOMB_FAILED: {
            lcd.setCursor(0, 0);
            lcd.print("NO MACH PASSWORD!");
            lcd.setCursor(0, 1);
            lcd.print("TRY 3 LEFT.");
            tone(SOUND_MODULE, 500);
            delay(200);
            noTone(SOUND_MODULE);
            delay(200);
            tone(SOUND_MODULE, 500);
            delay(200);
            noTone(SOUND_MODULE);
            delay(200);
            delay(2000);
            state = STATE_TIMEBOMB_RUN_BOMB;
            displayPrint();
            break;
        }
        case STATE_TIMEBOMB_CLEAR_BOMB: {
            lcd.setCursor(0, 0);
            lcd.print("OK. CREAR BOMB... ");
            for ( int i = 0; i < 100; ++i ) {
                lcd.setCursor(0, 1);
                lcd.print( (String)(i+1) + "%");
                tone(SOUND_MODULE, 1000);
                delay(10);
                noTone(SOUND_MODULE);
                delay(10);
            }
            delay(1000);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("CLEAR DONE.");
            lcd.setCursor(0, 1);
            lcd.print("YOU WINER!");
            for ( int i = 0; i < 100; ++i ) {
                tone(SOUND_MODULE, (i*50));
                delay(10);
                noTone(SOUND_MODULE);
                delay(10);
            }
            state = STATE_MAIN;
            resetState();
            displayPrint();
            break;
        }
        default:
            break;
    }
}

int keyboardScanner( char key ) {
    if ( key ) {
        Serial.println("Push Key : " + (String)key + ", state: " + (String)state);
        if ( state == STATE_MAIN ) {
            switch (key) {
                case '1':
                    state = STATE_TIMEBOMB_READY_TIME;
                    break;
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
            memset( confirm, 48, MAX_PASSWORD_LENGTH );
            state = STATE_TIMEBOMB_READY_CONFRIM;
        } else if ( state == STATE_TIMEBOMB_RUN_BOMB ) {
            switch (key) {
                case 'C':
                    state = STATE_TIMEBOMB_CLEAR_PASSWORD;
                    break;
                default:
                    break;
            }
        } else if ( state == STATE_TIMEBOMB_CLEAR_PASSWORD ) {
            state = inputPassword( key );
        } else if ( state == STATE_TIMEBOMB_CLEAR_BOMB ) {
            if ( key == 'C' ) {
                Serial.println( (String)key );
            }
        }
        displayPrint();
    }
    return state;
}

void timeDisplay() {
    static int speed = 1000;
    static int degree = 2;
    static int step = 0;
    static int stepBuffer = 0;
    second = (millis() - currentTime) / (speed/degree);
    stepBuffer = second % 2;
    Serial.println( "stepBuffer : " + (String)stepBuffer + ", millis() : " + millis() + ", second : " + (String)second);
    if ( stepBuffer == 1 && step == 0 ) {
	    display.showNumberDecEx((outputTime - (second/degree)), 0xff, true);
        tone(SOUND_MODULE, 5000);
        delay(100);
        noTone(SOUND_MODULE);
        step = 1;
    } else if ( stepBuffer == 0 && step == 1 ){
	    display.showNumberDecEx((outputTime - (second/degree)), 0, true);
        step = 0;
    }
    // display.setSegments(data);
}

void setup() {
    lcd.init();
    lcd.backlight();
    pinMode(SOUND_MODULE, OUTPUT);
    display.setBrightness(0x0f);
    
    data[0] = display.encodeDigit(0);
    data[1] = display.encodeDigit(0);
    data[2] = display.encodeDigit(0);
    data[3] = display.encodeDigit(0);
    display.setSegments(data);

    Serial.begin(9600);
    Serial.println("Ready to BOMB");
    displayPrint();
}

void loop() {
    keyboardScanner( keypad.getKey() );
    if ( isActiveTimer ) {
        timeDisplay();
    }
}