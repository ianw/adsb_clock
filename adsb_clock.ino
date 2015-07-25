/*--------------------------------------------------------------------------------------
  Includes
--------------------------------------------------------------------------------------*/
#include <SPI.h>        //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)
#include <DMD.h>        //
#include <TimerOne.h>   //
#include <DS3232RTC.h>
#include <Time.h>
#include <Wire.h>

#include "SystemFont5x7.h"
#include "Arial_black_16.h"
#include "Arial14.h"

//Fire up the DMD library as dmd
#define DISPLAYS_ACROSS 1
#define DISPLAYS_DOWN 1
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);

// RST -> 2
// DATA -> 3
// CLOCK -> 4
//DS1302RTC RTC(2,3,4);

int lastMinute = 0;
int lastSecond = 0;
int lastBrightness = 0;

/* Interrupt handler for Timer1 (TimerOne) driven DMD refresh
 * scanning, this gets called at the period set in
 * Timer1.initialize();
 */
void ScanDMD()
{
  dmd.scanDisplayBySPI();
}

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
 * read state of resistor ladder buttons & alternative digital button,
 * which is pulled-up (i.e. 0 denotes pressed).
 */
int read_buttons(void) {
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
}

int check_buttons(void) {

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

   //initialize TimerOne's interrupt/CPU usage used to scan and refresh the display

   Timer1.initialize( 300 );           //period in microseconds to call ScanDMD. Anything longer than 5000 (5ms) and you can see flicker.
   Timer1.pwm(PIN_DMD_nOE, 100);
   Timer1.attachInterrupt( ScanDMD );   //attach the Timer1 interrupt to ScanDMD which goes to dmd.scanDisplayBySPI()

   Serial.begin(9600);

   //clear/init the DMD pixels held in RAM
   dmd.clearScreen( true );   //true is normal (all pixels off), false is negative (all pixels on)

   setSyncProvider(RTC.get);

   lastMinute = minute();
//   setTime(15,34,00,24,07,2015);
//   if (RTC.set(now()) == 0) {
//           Serial.println("Set time!");
//   }

   // "alternate" button
   pinMode(12, INPUT_PULLUP);

   // buzzer
   pinMode(5, OUTPUT);
}

/*--------------------------------------------------------------------------------------
  loop
  Arduino architecture main loop
--------------------------------------------------------------------------------------*/
void loop(void)
{
   char inData[100];
   char code;
   int ret;

   switch (check_buttons()) {
   case 0:
           break;
   case btn_ONE:
           tone(5, 2048, 100);
           adjustTime(60);
           ret = RTC.set(now());
           break;
   case btn_FIVE:
           tone(5, 1800, 100);
           adjustTime(-60);
           ret = RTC.set(now());
           break;
   }

   // set the brightness to one of 3 levels based on a photoresistor
   // attached to analog pin0.  The PWM duty cycle seems to affect the
   // display logarithmically; values < 100 have largest variation.
   int brightness = map(analogRead(0), 0, 800, 0, 3);
   const int set_brightness[4] = {16, 100, 650, 1024};
   if (lastBrightness != brightness ) {
           lastBrightness = brightness;
           Timer1.setPwmDuty(PIN_DMD_nOE, set_brightness[brightness]);
   }


   // default to showing time
   code = 't';

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

   // scroll the message if "m"
   if (code == 'm') {
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

           if (m != lastMinute) {
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

           dmd.drawChar(  0,  0, t[0], GRAPHICS_NORMAL );
           dmd.drawChar(  7,  0, t[1], GRAPHICS_NORMAL );
           //dmd.drawChar( 14,  0, '.', GRAPHICS_NORMAL );
           // slightly larger dot looks better
           dmd.drawFilledBox(14, 9, 15, 10, GRAPHICS_NORMAL);
           dmd.drawChar( 17,  0, t[2], GRAPHICS_NORMAL );
           dmd.drawChar( 25,  0, t[3], GRAPHICS_NORMAL );

           dmd.selectFont(System5x7);

           // p is position from LHS for the boxes
           for (i=5, p=4; i>=0; i--, p+=4) {
                   if (s & 1<<i) {
                           dmd.drawFilledBox(p, 12, p+2, 14, GRAPHICS_NORMAL);
                   } else {
                           dmd.drawFilledBox(p, 12, p+2, 14, GRAPHICS_NOR);
                           dmd.drawFilledBox(p, 14, p+2, 14, GRAPHICS_NORMAL);
                   }
           }
   }
}
