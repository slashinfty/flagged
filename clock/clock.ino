// Grove_4Digital_Display library
#include <TM1637.h>
// EEPROM
#include <EEPROM.h>

// pin numbers
int minus5 = 2; // subtracts 5 from current setting
int minus1 = 3; // subtracts 1 from current setting
int enter = 4; // moves to next setting or pauses active timer
int plus1 = 5; // adds 1 to current setting
int plus5 = 6; // adds 5 to current setting
int player2 = 7; // stops player two's timer and starts player one's timer
int player1 = 8; // stops player one's timer and starts player two's timer
int p1led = 14; // LED for player one
int p2led = 15; // LED for player two

// timers
int CLK2 = 9;
int DIO2 = 10;
int CLK1 = 11;
int DIO1 = 12;
TM1637 TM1(CLK1, DIO1);
TM1637 TM2(CLK2, DIO2);

// global variables
int turn = 0; // 0 for setup, 1 for player one's turn, 2 for player two's turn
int set = 0; // 0-5, inclusive
unsigned long p1time = 0UL; // milliseconds for player one's timer
unsigned long p2time = 0UL; // milliseconds for player two's timer
unsigned long p1bonus = 0UL; // milliseconds for player one's bonus time
unsigned long p2bonus = 0UL; // milliseconds for player two's bonus time
bool delayTime = false; // whether it is delay or bonus time
unsigned long timeStart = 0UL; // milliseconds for player's timer at the start of a turn
unsigned long turnStart = 0UL; // millis() for the start of a turn
int lastTime = 0; // seconds of player's timer when loop last ran

void store() {
    EEPROM.put(0, p1time);
    EEPROM.put(4, p2time);
    EEPROM.put(8, p1bonus);
    EEPROM.put(12, p2bonus);
    EEPROM.put(16, delayTime);
}

// function to display time
void display(TM1637 tm, unsigned long ms) {
    // clear display before displaying new values
    // i.e. if 10 minutes goes to 9 minutes, won't display 19:00
    tm.clearDisplay();
    int sec = int(ms / 1000UL);
    int min = sec / 60;
    sec = sec % 60;
    if (min > 0) {
        tm.point(1);
        if (min > 9) {
            tm.display(0, min / 10 % 10);
        }
        tm.display(1, min % 10);
        tm.display(2, sec / 10 % 10);
    }
    else if (sec > 9) {
        tm.display(2, sec / 10 % 10);
    }
    tm.display(3, sec % 10);
}

// function to display 'b' or 'd' to signify bonus or delay
void alphaDisplay() {
    TM1.clearDisplay();
    TM2.clearDisplay();
    if (delayTime == true) {
        TM1.display(3, 13);
        TM2.display(3, 13);
    } else {
        TM1.display(0, 11);
        TM2.display(0, 11);
    }
}

// function to change time controls
// equations are a little ugly to ensure arithmetic on unsigned longs is done as expected (always non-negative)
// option order: player one minutes, player one seconds, player two minutes, player two seconds, player one bonus, player two bonus
void adjust(int opt, int val) {
    bool pos = val > 0;
    unsigned long mult = pos ? (unsigned long)val : (unsigned long)(-val);
    switch (opt) {
        case 0:
            p1time = pos ? p1time + (mult * 60000UL) : mult * 60000UL > p1time ? 0 : p1time - (mult * 60000UL);
            p1time = p1time > 5999000UL ? 5999000UL : p1time;
            display(TM1, p1time);
            break;
        case 1:
            p1time = pos ? p1time + (mult * 1000UL) : mult * 1000UL > p1time ? 0 : p1time - (mult * 1000UL);
            p1time = p1time > 5999000UL ? 5999000UL : p1time;
            display(TM1, p1time);
            break;
        case 2:
            p2time = pos ? p2time + (mult * 60000UL) : mult * 60000UL > p2time ? 0 : p2time - (mult * 60000UL);
            p2time = p2time > 5999000UL ? 5999000UL : p2time;
            display(TM2, p2time);
            break;
        case 3:
            p2time = pos ? p2time + (mult * 1000UL) : mult * 1000UL > p2time ? 0 : p2time - (mult * 1000UL);
            p2time = p2time > 5999000UL ? 5999000UL : p2time;
            display(TM2, p2time);
            break;
        case 4:
            p1bonus = pos ? p1bonus + (mult * 1000UL) : mult * 1000UL > p1bonus ? 0 : p1bonus - (mult * 1000UL);
            p1bonus = p1bonus > 300000UL ? 300000UL : p1bonus;
            display(TM1, p1bonus);
            break;
        case 5:
            p2bonus = pos ? p2bonus + (mult * 1000UL) : mult * 1000UL > p2bonus ? 0 : p2bonus - (mult * 1000UL);
            p2bonus = p2bonus > 300000UL ? 300000UL : p2bonus;
            display(TM2, p2bonus);
            break;
        case 6:
            delayTime = pos;
            alphaDisplay();
    }
    store();
}

// function to light LED for player whose turn it is
void ledToggle(int player) {
    if (player == 1) {
        digitalWrite(p1led, HIGH);
        digitalWrite(p2led, LOW);
    }
    else {
        digitalWrite(p2led, HIGH);
        digitalWrite(p1led, LOW);
    }
}

void setup() {
    // recover previous settings
    EEPROM.get(0, p1time);
    EEPROM.get(4, p2time);
    EEPROM.get(8, p1bonus);
    EEPROM.get(12, p2bonus);
    EEPROM.get(16, delayTime);

    // initialize displays
    TM1.init();
    TM1.set(2);
    display(TM1, p1time);
    TM2.init();
    TM2.set(2);
    display(TM2, p2time);

    // set pins
    pinMode(minus5, INPUT);
    pinMode(minus1, INPUT);
    pinMode(enter, INPUT);
    pinMode(plus1, INPUT);
    pinMode(plus5, INPUT);
    pinMode(player1, INPUT);
    pinMode(player2, INPUT);
    pinMode(p1led, OUTPUT);
    pinMode(p2led, OUTPUT);
}

void loop() {
    // read buttons to set time controls
    if (turn == 0) {
        if (digitalRead(plus5) == HIGH) {
            adjust(set, 5);
            delay(250);
        }
        else if (digitalRead(plus1) == HIGH) {
            adjust(set, 1);
            delay(250);
        }
        else if (digitalRead(minus1) == HIGH) {
            adjust(set, -1);
            delay(250);
        }
        else if (digitalRead(minus5) == HIGH) {
            adjust(set, -5);
            delay(250);
        }
        else if (digitalRead(enter) == HIGH) {
            // keep set between 0 and 6, inclusively
            set = (set + 1) % 7;
            if (set == 4) {
                display(TM1, p1bonus);
                display(TM2, p2bonus);
            }
            else if (set == 0) {
                display(TM1, p1time);
                display(TM2, p2time);
            } else if (set == 6) {
                alphaDisplay();
            }
            delay(250);
        }
        // if player one or player two is pressed, start the timer for the other player
        else if (digitalRead(player1) == HIGH) {
            if (set >= 3) {
                display(TM1, p1time);
                display(TM2, p2time);
            }
            turn = 2;
            ledToggle(2);
            timeStart = p2time;
            turnStart = millis();
            lastTime = int(timeStart / 1000UL);
        }
        else if (digitalRead(player2) == HIGH) {
            if (set >= 3) {
                display(TM1, p1time);
                display(TM2, p2time);
            }
            turn = 1;
            ledToggle(1);
            timeStart = p1time;
            turnStart = millis();
            lastTime = int(timeStart / 1000UL);
        }
    }
    else {
        // loop for displaying time while active
        while (p1time > 0 && p2time > 0) {
            unsigned long now = millis();
            unsigned long timeSpent = now - turnStart;
            // handling delay
            if (delayTime == true) {
                unsigned long currentDelay = turn == 1 ? p1bonus : p2bonus;
                timeSpent = now - turnStart <= currentDelay ? 0 : now - turnStart - currentDelay;
            }
            // pause timers
            if (digitalRead(enter) == HIGH) {
                delay(1000);
                while (true) {
                    if (digitalRead(enter) == HIGH) {
                        delay(300);
                        break;
                    }
                }
                timeStart = turn == 1 ? p1time : p2time;
                turnStart = millis();
            }
            // run timer for player one
            else if (turn == 1) {
                p1time = (timeSpent > timeStart) ? 0 : timeStart - timeSpent;
                // when player one presses their button
                if (digitalRead(player1) == HIGH) {
                    if (delayTime == false) {
                        p1time += p1bonus;
                    }
                    turn = 2;
                    ledToggle(2);
                    timeStart = p2time;
                    turnStart = millis();
                    lastTime = int(timeStart / 1000UL);
                }
                // only display time if the seconds are different
                if (int(p1time / 1000UL) != lastTime || turn == 2) {
                    display(TM1, p1time);
                    lastTime = int(p1time / 1000UL);
                }
            }
            // run timer for player two
            else if (turn == 2) {
                p2time = (timeSpent > timeStart) ? 0 : timeStart - timeSpent;
                // when player two presses their button
                if (digitalRead(player2) == HIGH) {
                    if (delayTime == false) {
                        p2time += p2bonus;
                    }
                    turn = 1;
                    ledToggle(1);
                    timeStart = p1time;
                    turnStart = millis();
                    lastTime = int(timeStart / 1000UL);
                }
                // only display time if the seconds are different
                if (int(p2time / 1000UL) != lastTime || turn == 1) {
                    display(TM2, p2time);
                    lastTime = int(p2time / 1000UL);
                }
            }
        }
        // exit when game has finished
        digitalWrite(p1led, LOW);
        digitalWrite(p2led, LOW);
        exit(0);
    }
}
