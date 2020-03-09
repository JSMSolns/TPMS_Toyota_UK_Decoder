
//#define USE_ADAFRUIT 1
#define USE_TEXTONLY 1

#if USE_ADAFRUIT
   #include <Adafruit_GFX.h>
   #include <Adafruit_SSD1306.h>


   #define SCREEN_WIDTH 128 // OLED display width, in pixels
   #define SCREEN_HEIGHT 64 // OLED display height, in pixels
   Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,  &Wire, -1);


  void ShowTitle()
  {
    display.clearDisplay();
    display.setFont(Adafruit5x7);
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setTextColor(WHITE, BLACK);       // Draw white text
  
    display.setCursor(0, 0);
    display.println("Toyota TPMS Monitor");
    display.println("   (JSM Solutions)");
  
  
  }
  
  void UpdateDisplay()
  {
    int i;
    int x = 0;
    int y = 0;
    char s[6];
  
    ShowTitle();
    
    display.setFont(Adafruit5x7);
    display.setTextSize(2);
  
    for (i = 0; i < 4; i++)
    {
      switch (i)
      {
        case 0:
          x = 0;
          y = 16;
          break;
        case 1:
          x = 64;
          y = 16;
          break;
        case 2:
          x = 0;
          y = 48;
          break;
        case 3:
          x = 64;
          y = 48;
          break;
      }
  
  
      display.setCursor(x, y);
  
      if (TPMS[i].TPMS_ID != 0)
      {
        dtostrf(TPMS[i].TPMS_Pressure, 3, 1, s);
        //sprintf(temperature,"%s F", str_temp);
        //sprintf(s,"%.1f",TPMS[i].TPMS_Pressure);
        display.print(s);
      }
    }
    display.display();
  
  }
   
#else
   #include "SSD1306Ascii.h"
   #include "SSD1306AsciiWire.h"

   SSD1306AsciiWire display;


   void ShowTitle()
  {
    display.clear();
  
    display.set1X();             // Normal 1:1 pixel scale
    //display.setTextColor(WHITE, BLACK);       // Draw white text
  
    display.setCursor(0, 0);
    display.println("Toyota TPMS Monitor");
    display.println("      (JSM Solutions)");
  
  
  }

  char DisplayTimeoutBar(unsigned long TimeSinceLastUpdate)
  {
      int HowCloseToTimeout;
      HowCloseToTimeout = (int)(TimeSinceLastUpdate/(TPMS_TIMEOUT/5));

      switch(HowCloseToTimeout)
      {
        case 0: 
           //return(FONTBAR_7);
           return('5');
           break;
        case 1: 
           //return(FONTBAR_5);
           return('4');
           break;
        case 2: 
           //return(FONTBAR_3);
           return('3');
           break;
        case 3: 
           //return(FONTBAR_2);
           return('2');
           break;
        case 4: 
           //return(FONTBAR_1);
           return('1');
           break;
        default: 
           //return(FONTBAR_0);
           return('0');
           break;
                      
      }
  }
  
  void UpdateDisplay()
  {
    int i;
    int x = 0;
    int y = 0;
    char s[6];
  
    ShowTitle();
  

  
    for (i = 0; i < 4; i++)
    {
      switch (i)
      {
        case 0:
          x = 0;
          y = 2;
          break;
        case 1:
          x = 59;
          y = 2;
          break;
        case 2:
          x = 0;
          y = 5;
          break;
        case 3:
          x = 59;
          y = 5;
          break;
      }
  
  
      display.setCursor(x, y);
  
      if (TPMS[i].TPMS_ID != 0)
      {
        dtostrf(TPMS[i].TPMS_Pressure, 3, 1, s);
        //sprintf(temperature,"%s F", str_temp);
        //sprintf(s,"%.1f",TPMS[i].TPMS_Pressure);
        display.setFont(Adafruit5x7);
        display.set2X();
        display.print(s);


        display.setCursor(x, y+2);
        display.setFont(Adafruit5x7);
        display.set1X();
        dtostrf(TPMS[i].TPMS_Temperature, 2, 0, s);
        display.print(" ");
        display.print(s);
        display.setFont(System5x7);
        display.print(char(128));  //degrees symbol
        display.setFont(Adafruit5x7);
        display.print("C");
        display.print("  ");

        //display vertical bar showing how long since last update 7 bars = recent 1 bar = nearing timeout (at timeout it will be removed from display altogether)
        display.setFont(System5x7);          
        display.print(DisplayTimeoutBar(millis() - TPMS[i].lastupdated));
      }


    }
    
  
  }
#endif
