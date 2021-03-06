/*
XTronical graphical Pulse and Oxygen monitor Displays BPM and blood oxygen saturation % as well as a trace 
representing the rise and fall of blood flow through your capillaries. Note that although the trace looks very ECG like
that it is representing blood flow and not direct electrical activity of the heart although it is obviously directly
related to the heart function. 
See <https://www.xtronical.com/ColourHeartMonitor> for more details or the you tube video : 
 
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public 
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
version.
 
This program is distributed in the hope that it will be useful,but WITHOUT ANY WARRANTY; without even the implied 
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  
If not, see <http://www.gnu.org/licenses/>.
*/
 
 
#include "MAX30100.h"                                                     // Libraries required for the
#include "MAX30100_PulseOximeter.h"                                       // MAX30100 range sensors
#include <Adafruit_GFX.h>                                                 // Graphics library for drawin on screen
#include <Adafruit_ST7735.h>                                              // Hardware-specific library
 
// Recommended settings for the MAX30100, DO NOT CHANGE!!!!,  refer to the datasheet for further info
#define SAMPLING_RATE                       MAX30100_SAMPRATE_100HZ       // Max sample rate
#define IR_LED_CURRENT                      MAX30100_LED_CURR_50MA        // The LEDs currents must be set to a level that 
#define RED_LED_CURRENT                     MAX30100_LED_CURR_27_1MA      // avoids clipping and maximises the dynamic range
#define PULSE_WIDTH                         MAX30100_SPC_PW_1600US_16BITS // The pulse width of the LEDs driving determines
#define HIGHRES_MODE                        true                          // the resolution of the ADC
 
 
// Create objects for the raw data from the sensor (used to make the trace) and the pulse and oxygen levels
MAX30100 sensor;            // Raw Data
PulseOximeter pox;          // Pulse and Oxygen
 
// The following settings adjust various factors of the display
#define SCALING 12                                                  // Scale height of trace, reduce value to make trace height
                                                                    // bigger, increase to make smaller
#define TRACE_SPEED 0.5                                             // Speed of trace across screen, higher=faster   
#define TRACE_MIDDLE_Y_POSITION 41                                  // y pos on screen of approx middle of trace
#define TRACE_HEIGHT 64                                             // Max height of trace in pixels    
#define HALF_TRACE_HEIGHT TRACE_HEIGHT/2                            // half Max height of trace in pixels (the trace amplitude)    
#define TRACE_MIN_Y TRACE_MIDDLE_Y_POSITION-HALF_TRACE_HEIGHT+1     // Min Y pos of trace, calculated from above values
#define TRACE_MAX_Y TRACE_MIDDLE_Y_POSITION+HALF_TRACE_HEIGHT-1     // Max Y pos of trace, calculated from above values
 
// Pins to use with the 7735 display
#define TFT_CS     10   // Chop select
#define TFT_RST    9    // Reset
#define TFT_RS     8    // Register select
 
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_RS, TFT_RST);
 
 void onBeatDetected()
{
  // beep the beeper
}
 
void setup()
{
    tft.initR(INITR_144GREENTAB);   // initialize a ST7735S chip, for 128x128 display
    tft.fillScreen(ST7735_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(ST7735_WHITE);
    pox.setOnBeatDetectedCallback(onBeatDetected);
 
    // Initialize the sensor. Failures are generally due to an improper I2C wiring, missing power supply
    // or wrong target chip. Occasionally fails on startup (very rare), just press reset on Arduino
    if (!sensor.begin()) {
      tft.print("Could not initialise MAX30100");
      for(;;);                                                // End program in permanent loop
    }
    tft.setCursor(0,0);
    if (!pox.begin()) {
        tft.println("Could not initialise MAX30100");
        for(;;);                                              // End program in permanent loop
    }
    
    // Set up the parameters for the raw data object
    sensor.setMode(MAX30100_MODE_SPO2_HR);
    sensor.setLedsCurrent(IR_LED_CURRENT, RED_LED_CURRENT);
    sensor.setLedsPulseWidth(PULSE_WIDTH);
    sensor.setSamplingRate(SAMPLING_RATE);
    sensor.setHighresModeEnabled(HIGHRES_MODE);
 
    // Display BPM and O2 titles, these remain on screen, we only erase the trace and the 
    // BPM/O2 results, otherwise we can get some flicker    
    tft.setTextSize(2);
    tft.setCursor(0,86);
    tft.print("BPM   O");
    tft.setCursor(92,86);
    tft.print("%");
    tft.setTextSize(1);
    tft.setCursor(84,94);
    tft.print("2");                            // The small subscriper 2 of O2
    tft.setCursor(1,0);   
    tft.print("XTronical Health Care");
    tft.setTextSize(2);
    tft.drawRect(0,TRACE_MIN_Y-1,128,TRACE_HEIGHT+2,ST7735_BLUE);     // The border box for the trace    
}
 
void loop()
{
 
  int16_t Diff=0;                             // The difference between the Infra Red (IR) and Red LED raw results
  uint16_t ir, red;                           // raw results returned in these
  static float lastx=1;                       // Last x position of trace
  static int lasty=TRACE_MIDDLE_Y_POSITION;   // Last y position of trace, default to middle
  static float x=1;                           // current x position of trace
  int32_t y;                                  // current y position of trace
  uint8_t BPM,O2;                             // BPM and O2 values
  static uint32_t tsLastReport = 0;           // Last time BMP/O2 were checked
  static int32_t SensorOffset=10000;          // Offset to lowest point that raw data does not go below, default 10000
                                              // Note that as sensors may be slightly different the code adjusts this
                                              // on the fly if the trace is off screen. The default was determined
                                              // By analysis of the raw data returned                   
 
  pox.update();                               // Request pulse and o2 data from sensor
  sensor.update();                            // request raw data from sensor
  if(sensor.getRawValues(&ir, &red))          // If raw data available for IR and Red 
  {
    if(red<1000)                              // No pulse
      y=TRACE_MIDDLE_Y_POSITION;              // Set Y to default flat line in middle
    else
    {
      // Plot our new point
      Diff=(ir-red);                          // Get raw difference between the 2 LEDS
      Diff=Diff-SensorOffset;                 // Adjust the baseline of raw values by removing the offset (moves into a good range for scaling)
      Diff=Diff/SCALING;                      // Scale the difference so that it appears at a good height on screen
      
      // If the Min or max are off screen then we need to alter the SensorOffset, this should bring it nicely on screen
      if(Diff<-HALF_TRACE_HEIGHT)
        SensorOffset+=(SCALING*(abs(Diff)-32));
      if(Diff>HALF_TRACE_HEIGHT)
        SensorOffset+=(SCALING*(abs(Diff)-32));
        
      y=Diff+(TRACE_MIDDLE_Y_POSITION-HALF_TRACE_HEIGHT);         // These two lines move Y pos of trace to approx middle of display area
      y+=TRACE_HEIGHT/4;                                              
    } 
    
    if(y>TRACE_MAX_Y) y=TRACE_MAX_Y;                              // If going beyond trace box area then crop the trace
    if(y<TRACE_MIN_Y) y=TRACE_MIN_Y;                              // so it stays within
    tft.drawLine(lastx,lasty,x,y,ST7735_YELLOW);                  // Plot the next part of the trace
    lasty=y;                                                      // Save where the last Y pos was
    lastx=x;                                                      // Save where the last X pos was
    x+=TRACE_SPEED;                                               // Move trace along the display
    if(x>126)                                                     // If reached end of display then reset to statt
    {
      tft.fillRect(1,TRACE_MIN_Y,126,TRACE_HEIGHT,ST7735_BLACK);  // Blank trace display area
      x=1;                                                        // Back to start
      lastx=x;                                                    
    }
    
    if (millis() - tsLastReport > 1000)                           // If more than 1 second (1000milliseconds) has past
    {                                                             // since getting heart rate and O2 then get some bew values
      tft.fillRect(0,104,128,16,ST7735_BLACK);                    // Clear the old values
      BPM=round(pox.getHeartRate());                              // Get BPM
      if((BPM<60)|(BPM>110))                                      // If too low or high for a resting heart rate then display in red
        tft.setTextColor(ST7735_RED);
      else
        tft.setTextColor(ST7735_GREEN);                           // else display in green
      tft.setCursor(0,104);                                       // Put BPM at this position
      tft.print(BPM);                                             // print BPM to screen
      O2=pox.getSpO2();                                           // Get the O2
      if(O2<94)                                                   // If too low then display in red
        tft.setTextColor(ST7735_RED);
      else
        tft.setTextColor(ST7735_GREEN);                           // else green
      tft.setCursor(72,104);                                      // Set print position for the O2 value
      tft.print(O2);                                              // print it to screen
      tsLastReport = millis();                                    // Set the last time values got to current time
    }
  }
}
