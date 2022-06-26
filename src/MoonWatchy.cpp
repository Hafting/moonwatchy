/*
  MoonWatchy, by Helge Hafting
	 */

#include "MoonWatchy.h"
#include "defines.h"

#include "UechiGothic20num.h"    //numbers for 12-hour face

#include "Gogh_ExtraBold7A_T.h"  //A-T, for "HAFTING"
#include "nasamoons.h"

//Uppercase font for day/month names & messages
//In Norwegian, so 16-bit font. "MA TI ON TO FR LØ SØ"
#include "Engine5pt7.h"
#include "Engine5pt8.h"

//ascii+latin1+latin-extended-A
//Text in general, calendar, 24-hour face
//FreeSerif fills in "Geometric shapes" and "Miscellaneous Symbols"
#include "leaguegothic12pt7.h" //ascii text
#include "leaguegothic12pt8.h" //latin1+latin extended A
#include "FreeSerif8pt8.h"     //Moon phase symbols, extra small
#include "FreeSerif12pt8.h"    //Other useful symbols. Zodiac, planets, weather,...

//Number font for dates
#include "FreeSansBold15num.h"

//Picture of watch owner
#include "Helge.h"

#define LIGHTMODE 

#ifdef LIGHTMODE
# define FG GxEPD_BLACK
# define BG GxEPD_WHITE
#elif
# define FG GxEPD_WHITE
# define BG GxEPD_BLACK
#endif

//Source: http://individual.utoronto.ca/kalendis/lunar/#FALC
#define SYNODIC_MONTH (29.53058770576)

const GFXfont *Engine5pt[2] = {&Engine5pt7b, &Engine5pt8b}; 

const GFXfont *leaguegothic12pt[4] = {&leaguegothic_regular_webfont12pt7b, &leaguegothic_regular_webfont12pt8b, &FreeSerif8pt8b, &FreeSerif12pt8b};

char const * const monthname[13] = {"", "JAN", "FEB", "MAR", "APR", "MAI", "JUN", "JUL", "AUG", "SEP", "OKT", "NOV", "DES"};

char const * const monthfullname[13] = {"", "januar", "februar", "mars", "april", "mai", "juni", "juli", "august", "september", "oktober", "november", "desember"};

char const * const dayname2[7] = {"ma", "ti", "on", "to", "fr", "lø", "sø"};
char const * const dayname[8] = {"", "SØN", "MAN", "TIR", "ONS", "TOR", "FRE", "LØR"};
const char *num[32] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31"}; 

//x=monthname-1. zodiacsym[x] if day<=zodiacdate[x], else zodiacdate[(x+1) % 12]
char const *zodiacsym[12] = {"♑︎", "♒︎", "♓︎", "♈︎", "♉︎", "♊︎", "♋︎", "♌︎", "♍︎", "♎︎", "♏︎", "♐︎"};
//                              jan feb mar apr may jun jul aug sep oct nov dec
uint8_t const zodiacdate[12] = {19, 18, 20, 19, 20, 21, 22, 22, 22, 23, 21, 21};

const uint8_t BATTERY_SEGMENT_WIDTH = 7;
const uint8_t BATTERY_SEGMENT_HEIGHT = 11;
const uint8_t BATTERY_SEGMENT_SPACING = 9;
const uint8_t WEATHER_ICON_WIDTH = 48;
const uint8_t WEATHER_ICON_HEIGHT = 32;

RTC_DATA_ATTR int facenumber = 0;
//index 0 is invalid, january is 1.
const int monthdays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
const int monthshift[] = {0,0,3,3,6,1,4,6,2,5,0,3,5};
/*
  Use UP/DOWN to change watch face, but only in WATCHFACE_STATE
	Other states and oher buttons are handled by watchy.
*/
void MoonWatchy::handleButtonPress() {
	//Serial.begin(115200);
	//Serial.print("handle...\n");
	if (guiState == WATCHFACE_STATE) {
		//Up and Down switch watch faces
		uint64_t wakeupBit = esp_sleep_get_ext1_wakeup_status();
    if (wakeupBit & UP_BTN_MASK) facenumber = (facenumber + 1) & 7;
		else if (wakeupBit & DOWN_BTN_MASK) {
			facenumber = (facenumber - 1) & 7;
		}
		if (wakeupBit & (DOWN_BTN_MASK | UP_BTN_MASK)) {
			//Face changed, show immediately
			RTC.read(currentTime);
			showWatchFace(true);
		}
		else Watchy::handleButtonPress();
	} else Watchy::handleButtonPress(); //Watchy handles menus etc.
	//Serial.flush();
	//Serial.end();
}

void MoonWatchy::drawWatchFace(){
		uint16_t hour12 = (currentTime.Hour+11) % 12 + 1;
    display.fillScreen(BG);
    display.setTextColor(FG);
		u8display = new utf8_GFX(&display); //For utf-8 printing

		switch (facenumber) {
			case 6: //std 7 segment
		    drawTime7();
		    drawDate7();
  		  drawSteps7();
				drawWeather();
				drawBattery();
				display.drawBitmap(120, 77, WIFI_CONFIGURED ? wifi : wifioff, 26, 18, FG);
				if (BLE_CONFIGURED) {
    	    display.drawBitmap(100, 75, bluetooth, 13, 21, FG);
				}
				break;
			case 0:
				//12-hour watch with hands, date, moon phase
				/*
					 Display coordinates go from 0 to 199,
					 so the watch center is at (99.5, 99.5)
					 The face is drawn using polar coordinates centered on (99.5, 99.5)
				   Correct rounding gives formulas like x = round(99.5 + dist*sin(a))
					 which is the same as x = 99.5 + dist*sin(a) + 0.5
					 which is equivalent to 100 + dist*sin(a)
           Using "100" saves some floating point math.
        */

				draw12hours();
				drawMinuteMarks();
				drawMoon();
				draw12hourHands(hour12);

				//Name and date should move out of the way of the short hand

				//Short hand needs the space about 10:30-1:30
				//10:37-1:23, avoiding unnecessary crash with the long hand
				if (hour12 >= 11 || 
						(hour12 == 10 && currentTime.Minute >= 37) ||
					  (hour12 == 1 && currentTime.Minute < 23)) {
					//Move name away from the small hand
					drawNamePlate(99.5, 151);	
				} else drawNamePlate(99.5, 48); //normal place

				//Short hand needs the space about 8 to 10
				//Long hand needs the space x:40 to x:50, unless hour is 2 or 3
				//Limit at 10, but 9 goes up to 9:59, so...
				if ((hour12 >= 8 && hour12 <= 9) ||
						((hour12 < 2 || hour12 > 3) &&
						 currentTime.Minute >= 40 &&
						 currentTime.Minute <= 50
						)
						) drawDate(153, 99.5);
				else drawDate(46, 99.5);

				break;
		  case 7:
				//24-hour watch with hands, moon phase & step counter
				drawMinuteMarks();
				drawMoon();
				draw24hours();
				draw24hourHands();
				//Name and date that moves away from the short hand:
				if (currentTime.Hour >= 22 || currentTime.Hour < 2) drawNamePlate(99.5, 151);
				else drawNamePlate(99.5, 48); //normal place

				if (currentTime.Hour < 20 && currentTime.Hour >= 16) drawDate(151, 99.5);
				else drawDate(48, 99.5);
			
				break;
			case 1:
				//Calendar for current month
				drawCalendar(currentTime.Month, tmYearToCalendar(currentTime.Year));
				break;
			case 2:
				//Calendar for next month
			{
				uint16_t year = tmYearToCalendar(currentTime.Year);
				uint8_t month = currentTime.Month + 1;
				if (month == 13) {
					month = 1;
					++year;
				}
				drawCalendar(month, year);
				break;
			}
			case 3:
				//Picture of owner
				drawOwner();
				break;
			case 4:
				//Weather reports for several (compiled-in) places
			default:
				//If nothing better, show the facenumber
				display.setFont(&DSEG7_Classic_Bold_53);
				display.setCursor(5, 53+5);
				display.println(facenumber);
				break;
			case 5:
				//Test patterns
				//drawPixel, drawLine, drawFastVline, drawFastHline,
				//drawRect, fillRect, drawCircle, fillCircle
				//drawRoundRect, fillRoundRect (also circles with even radius)
				//drawTriangle, fillTriangle
				//drawChar, print, drawBitmap, font stuff

				//Triangles
			  display.fillTriangle(0,0, 9,0, 9,199, FG);
				display.drawTriangle(10,0, 19,0, 19,199, FG);

				//Circles
			  for (int i = 5; i<10; ++i) {
					display.drawCircle(30, 10, i, FG);
					display.drawCircle(30, 30+i, i, FG);
					display.drawRoundRect(125,125, 60, 60, 3*i, FG);
				}	

				//Test patterns 50-75 75-100 100-125...
				for (int x=0; x<25; ++x) for (int y=0;y<25; ++y) {
					//solid
					display.drawPixel(x+50,y,FG);
					//75%
					if ( (x&1) || (y&1) ) display.drawPixel(x+75,y,FG);
					//66%
					if ( (x+y) % 3) display.drawPixel(x+100,y,FG);
					//50%
					if ( (x^y) & 1 ) display.drawPixel(x+125,y,FG);
					//33%
					if ( !((x+y) % 3)) display.drawPixel(x+150,y,FG);
					//25%
					if ( x & y & 1) display.drawPixel(x+175,y,FG);

				}
				//single
				display.drawFastHLine(50, 50, 150, FG);

				//double
				display.drawFastHLine(50, 55, 150, FG);
				display.drawFastHLine(50, 56, 150, FG);

				//triple
				display.drawFastHLine(50, 60, 150, FG);
				display.drawFastHLine(50, 61, 150, FG);
				display.drawFastHLine(50, 62, 150, FG);

        break;
		}
		delete u8display;
}


char const *MoonWatchy::zodiacsign(int month, int day) {
	--month;
	return (day <= zodiacdate[month]) ? zodiacsym[month] : zodiacsym[(month + 1) % 12];
}


/*
	 Draws the Norwegian flag, b&w version. Useful for marking the national day
	 and similar. The size is minimal for a flag with correct proportions
	 and all details.
	 
	 It is too big for calendar days, but works for the watch display.
	 */
void MoonWatchy::drawFlag(int16_t x, int16_t y) {
	display.fillRect(x,    y,     6,  6, FG);
	display.fillRect(x,    y+10,  6,  6, FG);
	display.fillRect(x+10, y,    12,  6, FG);
	display.fillRect(x+10, y+10, 12,  6, FG);
	display.fillRect(x,    y+7,  22,  2, FG);
	display.fillRect(x+7,  y,     2, 16, FG);
}


void MoonWatchy::drawCalendar(uint8_t Month, uint16_t Year) {
	//Month calendar. Eventually, with caldav events...
	/*
Layout:					 
juni 2022     
ma ti on to fr lø sø
       1  2  3  4  5
 6  7  8  9 10 11 12
13 14 15 16 17 18 19
20 21 22 23 24 25 26
27 28 29 30         

7 lines (8 if more info) ~25 pixels/line
20 chars/line, ~10 pixels/char columns start on 0,30,60,90,...
print is left adjusted, use getTextBounds and so on

A ring around today.
If we get calendar sync, box or some sort of highlight for days with appointments. Under the calendar: one-line details of the next appointment.

If room, mark full moon, new moon and half moons using appropriate symbols. It is a moon clock, after all!

Also mark compiled-in special days. 

Formula for what day a month starts on needed. Moon phase calculation alredy exists for the moon clocks.

shift table for start of month (non-leap year).
if jan starts on a monday, feb starts 3 days later (thursday)
jan feb mar apr mai jun jul aug sep okt nov des
0 	3 	3 	6 	1 	4 	6 	2 	5 	0 	3 	5

For a leap year:
0   3   4   0   2   5   0   3   6   1   4   6

For a non-leap year, the next year start one day later
For a leap year, the next year start two days later

2000 0
2001 2
2002 3
2003 4
2004 5
2005 0
2006 1
2007 2
2008 3
2009 5

So, for each year later than ref, 1 day later
for each leap year inbetween, an extra day later. All mod 7

year 1900 starts on monday
2000 starts on saturday

ydiff = year-1900
ydiff mod 7 
(((year - 1901) / 4 ) + (year-1900) ) % 7

2022: (121/4 + 122) % 7 = 152 % 7 = 5 = saturday
juni: add montshift[6]=4
156 % 7 = 2 (wednesday)

	 */
	drawFlag(3,3);
	u8display->USEFONTSET(leaguegothic12pt);
	u8display->setTextColor(FG);
	uint16_t startday = (Year - 1901) / 4 +
		Year - 1900 +
		monthshift[Month];
	uint16_t lastday = monthdays[Month];
	if (!(Year & 3)) { //Leap year specials
		if (Month > 2) ++startday;
		else if (Month == 2) ++lastday;
	}
	startday %= 7; //monday=0

	int16_t x1, y1;
	uint16_t w, h;
	//Exact moon phase when this month starts:
	float phase = moonphase(Year, Month,
			                    1, 0, 0);
	float nextphase = phase + 1 / SYNODIC_MONTH;
	if (nextphase >= 1) nextphase -= 1;
	//top line y=25:  monthname year
	//         y=50:  day headings
	display.setCursor(60, 25);
	u8display->setTextWrap(false);
	u8display->print(monthfullname[Month]);
	u8display->print(' ');
	u8display->print(Year);
	//ca 9.5 pix/char, or 19 pixles for two chars, 28 for a group
	//Use getTextBounds for perfect right adjustment	
	for (int i = 0; i < 7; ++i) {
		u8display->getTextBounds(dayname2[i], 0, 0, &x1, &y1, &w, &h);
		display.setCursor(28*i+19-x1-w, 50);
		u8display->print(dayname2[i]);
	}
	uint16_t line = 75;
	for (int i = 1; i <= lastday; ++i) {
		u8display->getTextBounds(num[i], 0, 0, &x1, &y1, &w, &h);
		display.setCursor(startday * 28 + 19 - x1 - w, line);
		u8display->print(num[i]);
		if (i == currentTime.Day && Month == currentTime.Month) {
			//Mark today in the calendar
			display.drawRoundRect(startday*28-1, line-25+3, 31, 27, 12, FG);
			display.drawRoundRect(startday*28, line-25+4, 29, 25, 11, FG);
		}
		/* Moon phase symbols
			 They are printed with black on white, so
			 ● new moon, all black
			 ◐ first quarter, left side is black
			 ○ full moon (white)
			 ◑ third quarter, right side is black.
			 (Confusing, when viewed on a screen with black background!)
		*/
		if (phase > nextphase) u8display->print("●"); 
		else if (phase <= 1.0/4 && nextphase > 1.0/4) u8display->print("◐");
		else if (phase <= 1.0/2 && nextphase > 1.0/2) u8display->print("○");
		else if (phase <= 3.0/4 && nextphase > 3.0/4) u8display->print("◑");
			
		//New line?
		if (++startday == 7) {
			startday = 0;
			line += 25;
		}
		phase = nextphase;
		nextphase += 1/SYNODIC_MONTH;
		if (nextphase >= 1) nextphase -= 1;
	}
}


//No doubt whose watch this is 
void MoonWatchy::drawOwner() {
	display.drawBitmap(16, 0, epd_bmp_ownerHelge, 168, 200, FG);
}

void MoonWatchy::draw12hourHands(uint16_t hour12) {
	//Long hand
	//60 positions, 6 degrees between minute marks
	//Polar: x=r sin a, y=-r cos a. Zero degrees is "up"

	float a, a1, a2, sin_a, cos_a, sin_a1, cos_a1;
	int16_t x1, y1;
	
	a = radians(currentTime.Minute * 6);
	sin_a = sin(a);
	cos_a = cos(a);

	display.drawLine(100+79*sin_a, 100-79*cos_a, 100+98*sin_a, 100-98*cos_a, FG);

	//connecting blob
	display.fillCircle(100+34*sin_a, 100-34*cos_a, 4, FG);


	a1 = a+0.024; //right ring
	x1 = 100+42*sin(a1); y1 =  100-42*cos(a1);
	display.drawCircle(x1, y1, 4, FG);
	display.drawCircle(x1, y1, 5, FG);

	a1 = a1+0.119; //right dot .091 .110
	display.fillCircle(100+42*sin(a1), 100-42*cos(a1), 2, FG);

	a1 = a-0.040; //left ring  
	sin_a1 = sin(a1); cos_a1 = cos(a1);
	x1 = 100+49.75*sin_a1; y1 = 100-49.75*cos_a1;
	display.drawCircle(x1, y1, 4, FG);
	display.drawCircle(x1, y1, 5, FG);

	a1 = a1-0.101; //left dot .081 .105
	display.fillCircle(100+49.75*sin(a1), 100-49.75*cos(a1), 2, FG);

	//Long triangle, give this hand some mass
	a1 = a-0.050;
	x1 = 100+55*sin(a1); y1 = 100-55*cos(a1); 
	a1 = a+0.050;
	display.fillTriangle(x1, y1, 
			100+55*sin(a1), 100-55*cos(a1),
			100+81*sin_a, 100-81*cos(a), FG);

	//Blob near the tip, for visibility
	display.fillCircle(100+94.5*sin_a, 100-94.5*cos_a, 2, FG);


	//Short hand .Hour  360/12=30 degrees per hour
	a = radians((hour12+currentTime.Minute/60.0)*30);
	sin_a = sin(a);
	cos_a = cos(a);

	//connecting blob
	display.fillCircle(100+35*sin_a, 100-35*cos_a, 5, FG);

	a1 = a-0.13; //center left ring
	x1 = 100 + 43.3*sin(a1); y1 = 100 - 43.3*cos(a1);
	display.drawCircle(x1, y1, 5, FG);
	display.drawCircle(x1, y1, 6, FG);

	a1 = a1-0.13; //left dot
	x1 = 100 + 43.4*sin(a1); y1 = 100 - 43.4*cos(a1);
	display.fillCircle(x1, y1, 2, FG);


	a1 = a+0.13; //center right ring
	x1 = 100 + 43.3*sin(a1); y1 = 100 - 43.3*cos(a1);
	display.drawCircle(x1, y1, 5, FG);
	display.drawCircle(x1, y1, 6, FG);

	a1 = a1+0.13; //right dot
	x1 = 100 + 43.4*sin(a1); y1 = 100 - 43.4*cos(a1);
	display.fillCircle(x1, y1, 2, FG);

	//outer circle
	x1 = 100 + 53.4*sin_a; y1 = 100 - 53.4*cos_a;
	display.drawCircle(x1, y1, 5, FG);
	display.drawCircle(x1, y1, 6, FG);

	//outer dot
	display.fillCircle(100+60*sin_a, 100-60*cos_a, 3, FG);

	//tip
	a1 = a - 0.034;
	a2 = a + 0.034;
	display.fillTriangle(100+58*sin(a1), 100-58*cos(a1),
			100+58*sin(a2), 100-58*cos(a2),
			100+74*sin_a, 100-74*cos_a, FG);
}


void MoonWatchy::draw24hourHands() {
	//Short hand
	float a = radians((currentTime.Hour+currentTime.Minute/60.0)*15);
	float sin_a = sin(a),	cos_a = cos(a); 
	float a1 = a - 0.37, a2 = a + 0.37; //suitable width in radians
																			//Triangular hand, with white stripe
																			//two triangles, avoid overpainting the moon
	display.fillTriangle(100+33*sin_a, 100-33*cos_a, 
			100+69*sin_a, 100-69*cos_a,
			100+33*sin(a1), 100-33*cos(a1), FG);
	display.fillTriangle(100+33*sin_a, 100-33*cos_a, 
			100+69*sin_a, 100-69*cos_a,
			100+33*sin(a2), 100-33*cos(a2), FG);
	display.drawLine(100+34*sin_a, 100-34*cos_a,
			100+65*sin_a, 100-65*cos_a, BG);

	//Long thin hand with white stripe
	a = radians(currentTime.Minute * 6);
	sin_a = sin(a); cos_a = cos(a);
	a1 = a - 0.19; a2 = a + 0.19;
	display.fillTriangle(100+33*sin(a2), 100-33*cos(a2), 
			100+88*sin_a, 100-88*cos_a,
			100+33*sin(a1), 100-33*cos(a1), FG);
	display.drawLine(100+34*sin_a, 100-34*cos_a,
			100+65*sin_a, 100-65*cos_a, BG);
	display.drawLine(100+85*sin_a, 100-85*cos_a,
			100+98*sin_a, 100-98*cos_a, FG);
}

void MoonWatchy::draw12hours() {
	//Clockface numerals and hour marks
	//getTextBounds allows perfect placement, regardless of font
	uint16_t width, height;
	int16_t x1, y1;
	float a;
	display.setFont(&Uechi_Gothic20pt7b);
	for (int i = 1; i <= 12; ++i) {
		display.getTextBounds(num[i], 0, 0, &x1, &y1, &width, &height);
		a = radians(i*30);
		display.setCursor(100+83*sin(a)-width/2.0, 100-83*cos(a)+height/2.0);
		display.print(num[i]);
	}
}


void MoonWatchy::draw24hours() {
	drawFlag(0,0); //test
	//Numbers & hour marks
	int16_t x1, y1;
	uint16_t width, height;
	float a, sin_a, cos_a;
	display.setFont(&leaguegothic_regular_webfont12pt7b);
	for (int i = 0; i <= 23; ++i) {
		display.getTextBounds(num[i], 0, 0, &x1, &y1, &width, &height);
		a = radians(i*15);
		sin_a = sin(a);
		cos_a = cos(a);
		display.setCursor(100+87*sin_a-width/2.0, 100-87*cos_a+height/2.0);
		display.print(num[i]);
		display.fillCircle(100+71*sin_a, 100-71*cos_a, 2, FG);
	}
}

//Returns the moon phase as a float in the range [0,1>
//0:new moon, 1/4:first quarter, 1/2:full moon 3/4: third quarter
//Based on a reference New moon 30 may 2022, 11:30 GMT
//Synodic month: 29.53059 days. This gives all future new moons.
float MoonWatchy::moonphase(int year, int month, int day, int hour, int minute) {
	int mdays = 1; //Remaining day in may, after new moon on may 30.
	int i;
	if (year == 2022) {
		for (i = 6; i < month; ++i) mdays += monthdays[i];
		mdays += day; //Days in this month
	} else {
		if (year == 2023) {
			mdays = 246; //Rest of 2022
		} else {
			mdays = 611; //Rest of 2022+2023
		}
		if (year > 2024) {
			int years = year - 2024;
			int leaps = 1 + years / 4; //Correct until year 2100 ..
			mdays += years*365 + leaps;
		}
		//Remaining whole months in the current year
		for (i = 1; i < month; ++i) mdays += monthdays[i];
		//Was there a leap day in this year?
		if (!(year & 3) && month > 2) ++mdays; 
		mdays += day;	
	} //mdays holds the days since the reference new moon
		//Have the days, including today.
		//Now to handle the hours and minutes and timezone:
	float moondays = mdays - 1 + //Today has not passed, subtract
		((12.5 -    //Remaning hours of reference day
			GMT_OFFSET_SEC / 3600 + //Timezone
			hour + minute / 60.0) //Time of day
		 /24);
	float moons = moondays / SYNODIC_MONTH;
	return moons - (int)moons;
}

void MoonWatchy::drawMoon() {
	//Center the appropriate moon image. Radius 30
	/*
		 Have 2 moon images for every day, except new moon days.
	 */

	float phase = moonphase(tmYearToCalendar(currentTime.Year),
			                    currentTime.Month, currentTime.Day,
													currentTime.Hour, currentTime.Minute);
	//Have the phase, pick one of 59 images
	//Image 0 is a full moon, so shift by 30
	int moonnumber=phase * 59 + 30;
	moonnumber %= 59;

	int moon_ix = moonnumber;
	if (moon_ix > 25) {
		if (moon_ix >= 35) moon_ix -= 9; else moon_ix = -1;
	}
	if (moon_ix >= 0) {
		display.drawBitmap(70, 70, epd_bitmap_allArray[moon_ix], 58, 58, FG);
	} else display.fillRoundRect(69,69,60,60,30,FG); //New moon

	//Solid ring around moon, centered on 99.5,99.5
	for (int i = 0; i < 3; ++i) {
		display.drawRoundRect(69-i, 69-i, 60+2*i, 60+2*i, 30+i, FG);
	} //Room for a 58x58 circular moon
}


void MoonWatchy::drawMinuteMarks() {
	//Minute marks
	int ii = 0;
	for (int i = 0; i < 60; ++i) {
		float a = radians(i*6);
		float sin_a = sin(a);
		float cos_a = cos(a);
		if (ii) {
			//Small minute mark
			display.drawLine(100+99*sin_a, 100-99*cos_a, 
			                 100+102*sin_a,100-102*cos_a, FG);
		} else {
			//Bigger 5min mark
			display.fillCircle(100+99.5*sin_a, 100-99.5*cos_a, 2, FG);
		}
		if (++ii == 5) ii = 0;
	}
}

/*Draw my name with a frame, centered on (x,y)
 x is a float, because we typically center on 99.5,
 achieving pixel-perfect rounding if the name has odd length.
 */
void MoonWatchy::drawNamePlate(float x, int16_t y) {
	//My name
	const char *navn = "HAFTING";
	int16_t x1, y1, x2, y2;
	uint16_t width, height;
	display.setFont(&Gogh_ExtraBold7pt7b);
	display.getTextBounds(navn, 0, 0, &x1, &y1, &width, &height);
	x2 = x-width/2.0; y2 = y-height/2;
	display.fillRect(x2-1, y2-1, width+2, height+2, BG);
	display.drawRect(x2-2, y2-2, width+4, height+4, FG);
	display.setCursor(x2-x1, y2-y1);
	display.setTextColor(FG);
	display.print(navn);
}

/*
	Draw the date, centered on (x,y)
  LØ     small, Engine font
	31     large, FreeSansBold, possibly white on black
	MAI    small, Engine font

  Perhaps a frame around, if room
currentTime.Wday
currentTime.Day
currentTime.Month
	 */
void MoonWatchy::drawDate(int16_t x, float y) {
	//t: top, m:middle, b:bottom
	int16_t x1t, y1t, box_xall, box_yt;
	uint16_t widtht, heightt;
	int16_t x1m, y1m, box_ym;
	uint16_t widthm, heightm;
	int16_t x1b, y1b, box_yb;
	uint16_t widthb, heightb;

	uint16_t boxw, boxh;
	int16_t box_x, box_y;
	//Find sizes of text, so boxes can be drawn and centered:
	//uses 0,0; the real coordinates aren't known until
	//centering is calculated.

	display.setFont(&FreeSansBold15pt7b);
	display.getTextBounds(num[currentTime.Day], 0, 0, &x1m, &y1m, &widthm, &heightm);

	u8display->USEFONTSET(Engine5pt);
	u8display->getTextBounds(monthname[currentTime.Month], 0, 0, &x1b, &y1b, &widthb, &heightb);
	u8display->getTextBounds(dayname[currentTime.Wday], 0, 0, &x1t, &y1t, &widtht, &heightt);

	//widest box -> boxw. Framing adds 2 on each side
	boxw = (widtht > widthb) ? widtht : widthb;
	boxw = (boxw > widthm) ? boxw : widthm;
	boxw += 4;

	//heigth of column, including framing. Framing overlaps in the middle.
	boxh = 2 + heightt + 2 + 1 + heightm + 1 + 2 + heightb + 2;

	box_x = x - boxw / 2.0;
	box_y = y - boxh / 2;

	box_xall = box_x + 2;
	box_yt = box_y + 2; 
	box_ym = box_yt + heightt + 2 + 1;
	box_yb = box_ym + heightm + 1 + 2;

	display.fillRect(box_x + 1, box_y + 1, boxw - 2, boxh - 2, BG);
	display.drawRect(box_x, box_y, boxw, boxh, FG);

	u8display->setTextColor(FG);
	display.setTextColor(FG);
	display.setCursor(box_xall - x1t + (boxw-widtht-4)/2, box_yt - y1t);
	u8display->print(dayname[currentTime.Wday]);
	display.setCursor(box_xall - x1b + (boxw-widthb-4)/2, box_yb - y1b);
	u8display->print(monthname[currentTime.Month]);
	display.setCursor(box_xall - x1m + (boxw-widthm-4)/2, box_ym - y1m);
	display.print(num[currentTime.Day]);

	u8display->USEFONTSET(leaguegothic12pt);
	char const *zsign = zodiacsign(currentTime.Month, currentTime.Day);
	u8display->getTextBounds(zsign, 0, 0, &x1t, &y1t, &widtht, &heightt);
	display.setCursor(box_xall - x1t + (boxw-widtht-4)/2, box_yb + heightb + 4 - y1t);
	u8display->print(zsign);
}

void MoonWatchy::drawTime7(){
    display.setFont(&DSEG7_Classic_Bold_53);
    display.setCursor(5, 53+5);
    int displayHour;
    if(HOUR_12_24==12){
      displayHour = ((currentTime.Hour+11)%12)+1;
    } else {
      displayHour = currentTime.Hour;
    }
    if(displayHour < 10){
        display.print("0");
    }
    display.print(displayHour);
    display.print(":");
    if(currentTime.Minute < 10){
        display.print("0");
    }
    display.println(currentTime.Minute);
}

void MoonWatchy::drawDate7(){
    display.setFont(&Seven_Segment10pt7b);

    int16_t  x1, y1;
    uint16_t w, h;

    String dayOfWeek = dayStr(currentTime.Wday);
    display.getTextBounds(dayOfWeek, 5, 85, &x1, &y1, &w, &h);
    if(currentTime.Wday == 4){
        w = w - 5;
    }
    display.setCursor(85 - w, 85);
    display.println(dayOfWeek);

    String month = monthShortStr(currentTime.Month);
    display.getTextBounds(month, 60, 110, &x1, &y1, &w, &h);
    display.setCursor(85 - w, 110);
    display.println(month);

    display.setFont(&DSEG7_Classic_Bold_25);
    display.setCursor(5, 120);
    if(currentTime.Day < 10){
    display.print("0");
    }
    display.println(currentTime.Day);
    display.setCursor(5, 150);
    display.println(tmYearToCalendar(currentTime.Year));// offset from 1970, since year is stored in uint8_t
}
void MoonWatchy::drawSteps7(){
    // reset step counter at midnight
    if (currentTime.Hour == 0 && currentTime.Minute == 0){
      sensor.resetStepCounter();
    }
    uint32_t stepCount = sensor.getCounter();
    display.drawBitmap(10, 165, steps, 19, 23, FG);
    display.setCursor(35, 190);
    display.println(stepCount);
}
void MoonWatchy::drawBattery(){
    display.drawBitmap(154, 73, battery, 37, 21, FG);
    display.fillRect(159, 78, 27, BATTERY_SEGMENT_HEIGHT, BG);//clear battery segments
    int8_t batteryLevel = 0;
    float VBAT = getBatteryVoltage();
    if(VBAT > 4.1){
        batteryLevel = 3;
    }
    else if(VBAT > 3.95 && VBAT <= 4.1){
        batteryLevel = 2;
    }
    else if(VBAT > 3.80 && VBAT <= 3.95){
        batteryLevel = 1;
    }
    else if(VBAT <= 3.80){
        batteryLevel = 0;
    }

    for(int8_t batterySegments = 0; batterySegments < batteryLevel; batterySegments++){
        display.fillRect(159 + (batterySegments * BATTERY_SEGMENT_SPACING), 78, BATTERY_SEGMENT_WIDTH, BATTERY_SEGMENT_HEIGHT, FG);
    }
}

void MoonWatchy::drawWeather(){

    weatherData currentWeather = getWeatherData();

    int8_t temperature = currentWeather.temperature;
    int16_t weatherConditionCode = currentWeather.weatherConditionCode;

    display.setFont(&DSEG7_Classic_Regular_39);
    int16_t  x1, y1;
    uint16_t w, h;
    display.getTextBounds(String(temperature), 0, 0, &x1, &y1, &w, &h);
    if(159 - w - x1 > 87){
        display.setCursor(159 - w - x1, 150);
    }else{
        display.setFont(&DSEG7_Classic_Bold_25);
        display.getTextBounds(String(temperature), 0, 0, &x1, &y1, &w, &h);
        display.setCursor(159 - w - x1, 136);
    }
    display.println(temperature);
    display.drawBitmap(165, 110, currentWeather.isMetric ? celsius : fahrenheit, 26, 20, FG);
    const unsigned char* weatherIcon;

    //https://openweathermap.org/weather-conditions
    if(weatherConditionCode > 801){//Cloudy
    weatherIcon = cloudy;
    }else if(weatherConditionCode == 801){//Few Clouds
    weatherIcon = cloudsun;
    }else if(weatherConditionCode == 800){//Clear
    weatherIcon = sunny;
    }else if(weatherConditionCode >=700){//Atmosphere
    weatherIcon = atmosphere;
    }else if(weatherConditionCode >=600){//Snow
    weatherIcon = snow;
    }else if(weatherConditionCode >=500){//Rain
    weatherIcon = rain;
    }else if(weatherConditionCode >=300){//Drizzle
    weatherIcon = drizzle;
    }else if(weatherConditionCode >=200){//Thunderstorm
    weatherIcon = thunderstorm;
    }else
    return;
    display.drawBitmap(145, 158, weatherIcon, WEATHER_ICON_WIDTH, WEATHER_ICON_HEIGHT, FG);
}
