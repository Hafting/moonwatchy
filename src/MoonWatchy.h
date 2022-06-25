#ifndef MOONWATCHY_H
#define MOONWATCHY_H

/*
  By Helge Hafting
	 */

#include <Watchy.h>
#include "Seven_Segment10pt7b.h"
#include "DSEG7_Classic_Regular_15.h"
#include "DSEG7_Classic_Bold_25.h"
#include "DSEG7_Classic_Regular_39.h"
#include "icons.h"

#include "gfx-utf8.h"



//Avoid(mis)typing all that sizeof stuff:
#define USEFONTSET(f) setFontSet((f), sizeof(f)/sizeof(const GFXfont *))
class MoonWatchy : public Watchy{
    using Watchy::Watchy;
	  protected:
			utf8_GFX *u8display;			
    public:
        void drawWatchFace();
				void draw12hours();
				void draw24hours();
				void drawOwner();
				void draw12hourHands(uint16_t hour12);
				void draw24hourHands();
				void drawNamePlate(float x, int16_t y);
				void drawDate(int16_t x, float y);
				void drawMinuteMarks();
				void drawMoon();
				void drawCalendar(uint8_t Month, uint16_t Year);
				char const *zodiacsign(int month, int day);
				float moonphase(int year, int month, int day, int hour, int minute);
				//7-segment stuff:
        void drawTime7();
        void drawDate7();
        void drawSteps7();
        void drawWeather();
        void drawBattery();
        virtual void handleButtonPress();//Must be virtual in Watchy.h too
};

#endif
