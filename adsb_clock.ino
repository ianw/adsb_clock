
/*--------------------------------------------------------------------------------------
  Includes
--------------------------------------------------------------------------------------*/
#include <SPI.h>        //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)
#include <DMD.h>        //
#include <TimerOne.h>   //
#include "SystemFont5x7.h"
#include "Arial_black_16.h"
#include "Arial14.h"

//Fire up the DMD library as dmd
#define DISPLAYS_ACROSS 1
#define DISPLAYS_DOWN 1
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);

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

   Timer1.initialize( 1000 );           //period in microseconds to call ScanDMD. Anything longer than 5000 (5ms) and you can see flicker.
   Timer1.attachInterrupt( ScanDMD );   //attach the Timer1 interrupt to ScanDMD which goes to dmd.scanDisplayBySPI()

   Serial.begin(9600);

   //clear/init the DMD pixels held in RAM
   dmd.clearScreen( true );   //true is normal (all pixels off), false is negative (all pixels on)

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
           dmd.selectFont(Arial_14);
           dmd.drawChar(  0,  0, '2', GRAPHICS_NORMAL );
           dmd.drawChar(  7,  0, '3', GRAPHICS_NORMAL );
           dmd.drawChar( 14,  0, ':', GRAPHICS_NORMAL );
           dmd.drawChar( 17,  0, '4', GRAPHICS_NORMAL );
           dmd.drawChar( 25,  0, '5', GRAPHICS_NORMAL );
           delay(500);
   }


}

