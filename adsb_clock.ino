#include <assert.h>
#include <SPI.h>
#include <DMD.h>
#include <TimerOne.h>
#include <DS3232RTC.h>
#include <Time.h>
#include <Wire.h>

#include "SystemFont5x7.h"
//#include "Arial_black_16.h"
#include "Arial14.h"

//Fire up the DMD library as dmd
#define DISPLAYS_ACROSS 1
#define DISPLAYS_DOWN 1
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);

/* Interrupt handler for Timer1 (TimerOne) driven DMD refresh
 * scanning, this gets called at the period set in
 * Timer1.initialize();
 */
void ScanDMD()
{
  dmd.scanDisplayBySPI();
}

int lastMinute = 0;
int lastSecond = 0;
long lastBrightness = 0;

/* Resistor ladder button definitions */
#define btn_ONE   (1<<0)
#define btn_TWO   (1<<1)
#define btn_THREE (1<<2)
#define btn_FOUR  (1<<3)
#define btn_FIVE  (1<<4)
#define btn_SIX   (1<<5)
#define btn_SEVEN (1<<6)
#define btn_EIGHT (1<<7)
#define DEBOUNCE 10

/*
 * Read state of resistor ladder buttons from adc pin 1.  Also handle
 * the "alternative" digital button, which is pulled-up (i.e. 0
 * denotes pressed) and plugged into pin 12.
 */
int read_buttons(void)
{
        // resistor ladder on pin 1
        int adc_in = analogRead(1);
        // alternate toggle button on 12
        int alt = !digitalRead(12);

        // we can only have one pressed
        if (adc_in >= 800) {
                return 0;
        }

        if (adc_in < 50  && !alt) return btn_ONE;
        if (adc_in < 50  && alt)  return btn_FIVE;

        if (adc_in < 200 && !alt) return btn_TWO;
        if (adc_in < 200 && alt)  return btn_SIX;

        if (adc_in < 450 && !alt) return btn_THREE;
        if (adc_in < 450 && alt)  return btn_SEVEN;

        if (adc_in < 800 && !alt) return btn_FOUR;
        if (adc_in < 800 && alt)  return btn_EIGHT;

        assert("button read not handled!");
}

/*
 * Check if a button has been pushed.  If we see the button down,
 * remember that until we see it not pressed.  Simple debouncing done
 * by ignoring input for DEBOUNCE ms.  Returns 0 or btn_* if the
 * button was seen as pressed.
 */
int check_buttons(void)
{
        static int was_pushed;
        static unsigned long last_time;
        int pushed = 0;
        int button_just_pushed = 0;

        if (millis() < last_time) {
                last_time = millis();
        }

        if ((last_time + DEBOUNCE) > millis()) {
                return 0;
        }
        last_time = millis();

        pushed = read_buttons();

        /* if we saw a button was pushed, and now it's not, then it
         * was "just pushed" */
        if (was_pushed && pushed == 0) {
                button_just_pushed = was_pushed;
                was_pushed = 0;
        }
        /* a button is now pushed, remember that */
        if (pushed != 0) {
                was_pushed = pushed;
        }

        return button_just_pushed;
}

void setup(void)
{
        Serial.begin(9600);

        /* Anything longer than 5000 (5ms) for DMD rescan and you can
         * see flicker. */
        Timer1.initialize( 300 );
        Timer1.attachInterrupt( ScanDMD );

        /* PWN duty cycle for screen brightness */
        Timer1.pwm(PIN_DMD_nOE, 100);


        dmd.clearScreen( true );

        setSyncProvider(RTC.get);

        lastMinute = minute();

#if 0
        /* use this to set initial time */
        setTime(15,34,00,24,07,2015);
        if (RTC.set(now()) == 0) {
           Serial.println("Set time!");
        }
#endif // inital time set

        // "alternate" button
        pinMode(12, INPUT_PULLUP);

        // buzzer
        pinMode(5, OUTPUT);
}

void loop(void)
{
   char inData[100];
   char code = 't';
   int ret;
   bool must_update = false;

   switch (check_buttons()) {
   default:
           break;
   case btn_ONE:
           tone(5, 2048, 100);
           setTime( now() + 3600 );
           ret = RTC.set(now());
           must_update = true;
           break;
   case btn_FIVE:
           tone(5, 1800, 100);
           setTime( now() - 3600 );
           ret = RTC.set(now());
           must_update = true;
           break;
   case btn_TWO:
           tone(5, 2048, 100);
           setTime( now() + 60 );
           ret = RTC.set(now());
           must_update = true;
           break;
   case btn_SIX:
           tone(5, 1800, 100);
           setTime( now() - 60 );
           ret = RTC.set(now());
           must_update = true;
           break;
   case btn_FOUR:
           /* show date */
           code = 'd';
           break;
   }

   // set the brightness to one of 3 levels based on a photoresistor
   // attached to analog pin0.
   // a really bright light returns about 970 for this photoresistor
   // The PWM duty cycle seems to affect the display logarithmically;
   // values < 100 have largest variation.  this slightly logarithmic
   // response seems to work well and reduce flicker.
   long b = analogRead(0);
   long b_squared = (b * b) / 1024;
   b_squared = max(16, b_squared); // minimum dim
   if (lastBrightness != b_squared) {
           lastBrightness = b_squared;
           Timer1.setPwmDuty(PIN_DMD_nOE, b_squared);
   }

   if (Serial.available()) {
           code = Serial.read();
           if (code == 'm') {
                   char len[2];
                   int i, to_read;

                   while (Serial.available() == 0) {}
                   len[0] = Serial.read();
                   while (Serial.available() == 0) {}
                   len[1] = Serial.read();
                   to_read = atoi(len);

                   for (i = 0; i < to_read; i++) {
                           while (Serial.available() == 0) {}
                           inData[i] = Serial.read();
                   }
                   inData[i] = '\0';
           }
   }

   if (code == 'd') {
           // show date and temp
           char s[6];  // XX XXX
                       // 123456
           dmd.clearScreen(true);
           dmd.selectFont(System5x7);

           sprintf(s, "%02d %s", day(), monthShortStr(month()));
           dmd.drawString(1, 1, s, 2, GRAPHICS_NORMAL);
           dmd.drawString(14, 1, s+3, 3, GRAPHICS_NORMAL);

           sprintf(s, "%02dC", RTC.temperature() / 4);
           dmd.drawString(8, 9, s, 3, GRAPHICS_NORMAL);
           delay(5000);
           dmd.clearScreen(true);
   } else if (code == 'm') {
           // scroll a message
           dmd.clearScreen(true);
           dmd.selectFont(System5x7);
           dmd.drawMarquee(inData, strlen(inData), (32*DISPLAYS_ACROSS)-1, 5);
           unsigned long start = millis();
           unsigned long timer = start;
           boolean ret=false;
           while (!ret) {
                   if ((timer+100) < millis()) {
                           ret = dmd.stepMarquee(-1,0);
                           timer = millis();
                   }
           }
   } else {
           // the current time
           char t[8];
           int h = hour();
           int s = second();
           int m = minute();

           int i,p;

           dmd.selectFont(Arial_14);

           /* only wipe the whole screen when we update the time once
            * a minute.  The second counters will wipe themselves.
            * Reduces flicker */
           if (m != lastMinute || must_update) {
                   dmd.clearScreen(true);
                   lastMinute = m;
           }

           // don't bother if there's nothing to update
           if (s != lastSecond) {
                   lastSecond = s;
           } else {
                   return;
           }

           sprintf(t, "%02d%02d%02dg", h,m,s);

           dmd.drawChar(  1,  0, t[0], GRAPHICS_NORMAL );
           dmd.drawChar(  8,  0, t[1], GRAPHICS_NORMAL );
           //dmd.drawChar( 14,  0, '.', GRAPHICS_NORMAL );
           // slightly larger dot looks better
           dmd.drawFilledBox(15, 9, 16, 10, GRAPHICS_NORMAL);
           dmd.drawChar( 18,  0, t[2], GRAPHICS_NORMAL );
           dmd.drawChar( 25,  0, t[3], GRAPHICS_NORMAL );

           dmd.selectFont(System5x7);

           // p is position from LHS for the boxes
           for (i=5, p=4; i>=0; i--, p+=4) {
                   if (s & 1<<i) {
                           dmd.drawFilledBox(p, 13, p+2, 15, GRAPHICS_NORMAL);
                   } else {
                           dmd.drawFilledBox(p, 13, p+2, 15, GRAPHICS_NOR);
                           dmd.drawFilledBox(p, 15, p+2, 15, GRAPHICS_NORMAL);
                   }
           }
   }
}
