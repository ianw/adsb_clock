
/*--------------------------------------------------------------------------------------
  Includes
--------------------------------------------------------------------------------------*/
#include <SPI.h>        //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)
#include <DMD.h>        //
#include <TimerOne.h>   //
#include <DS1302RTC.h>
#include <Time.h>

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
DS1302RTC RTC(2,3,4);

int lastMinute = 0;

/*--------------------------------------------------------------------------------------
  Interrupt handler for Timer1 (TimerOne) driven DMD refresh scanning, this gets
  called at the period set in Timer1.initialize();
--------------------------------------------------------------------------------------*/
void ScanDMD()
{
  dmd.scanDisplayBySPI();
}

/*--------------------------------------------------------------------------------------
  setup
  Called by the Arduino architecture before the main loop begins
--------------------------------------------------------------------------------------*/
void setup(void)
{

   //initialize TimerOne's interrupt/CPU usage used to scan and refresh the display

   Timer1.initialize( 300 );           //period in microseconds to call ScanDMD. Anything longer than 5000 (5ms) and you can see flicker.
   Timer1.attachInterrupt( ScanDMD );   //attach the Timer1 interrupt to ScanDMD which goes to dmd.scanDisplayBySPI()

   Serial.begin(9600);

   //clear/init the DMD pixels held in RAM
   dmd.clearScreen( true );   //true is normal (all pixels off), false is negative (all pixels on)

   setSyncProvider(RTC.get);

   lastMinute = minute();
//   setTime(20,03,00,10,06,2015);
//   if (RTC.set(now()) == 0) {
//           Serial.println("Set time!");
//   }
}

/*--------------------------------------------------------------------------------------
  loop
  Arduino architecture main loop
--------------------------------------------------------------------------------------*/
void loop(void)
{
   char inData[100];
   char code;

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
           long start = millis();
           long timer = start;
           boolean ret=false;
           while (!ret) {
                   if ((timer+100) < millis()) {
                           ret = dmd.stepMarquee(-1,0);
                           timer = millis();
                   }
           }
   } else {
           char t[8];
           int h = hour();
           int s = second();
           int m = minute();
           dmd.selectFont(Arial_14);

           if (m != lastMinute) {
                   dmd.clearScreen(true);
                   lastMinute = m;
           }

           Serial.println(s);
           sprintf(t, "%02d%02d%02d%02d", h,m,s);

           dmd.drawChar(  0,  0, t[0], GRAPHICS_NORMAL );
           dmd.drawChar(  7,  0, t[1], GRAPHICS_NORMAL );
           dmd.drawChar( 14,  0, '.', GRAPHICS_NORMAL );
           dmd.drawChar( 17,  0, t[2], GRAPHICS_NORMAL );
           dmd.drawChar( 25,  0, t[3], GRAPHICS_NORMAL );

           dmd.selectFont(System5x7);

           // 2 4 8 16 32 64
           // 64 32 16 8 4 2
           #define START 5
           if (s & 0x1<<6) {
                   dmd.drawFilledBox(START, 12, START+2, 14, GRAPHICS_NORMAL);
           } else {
                   dmd.drawFilledBox(START, 12, START+2, 14, GRAPHICS_NOR);
           }
           if (s & 0x1<<5) {
                   dmd.drawFilledBox(START+4, 12, START+6, 14, GRAPHICS_NORMAL);
           } else {
                   dmd.drawFilledBox(START+4, 12, START+6, 14, GRAPHICS_NOR);
           }

           if (s & 0x1<<4) {
                   dmd.drawFilledBox(START+8, 12, START+10, 14, GRAPHICS_NORMAL);
           } else {
                   dmd.drawFilledBox(START+8, 12, START+10, 14, GRAPHICS_NOR);
           }
           if (s & 0x1<<3) {
                   dmd.drawFilledBox(START+12, 12, START+14, 14, GRAPHICS_NORMAL);
           } else {
                   dmd.drawFilledBox(START+12, 12, START+14, 14, GRAPHICS_NOR);
           }
           if (s & 0x1<<2) {
                   dmd.drawFilledBox(START+16, 12, START+18, 14, GRAPHICS_NORMAL);
           } else {
                   dmd.drawFilledBox(START+16, 12, START+18, 14, GRAPHICS_NOR);
           }
           if (s & 0x1<<1) {
                   dmd.drawFilledBox(START+20, 12, START+22, 14, GRAPHICS_NORMAL);
           } else {
                   dmd.drawFilledBox(START+20, 12, START+22, 14, GRAPHICS_NOR);
           }

//           delay(100);
   }
}
