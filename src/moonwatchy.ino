
/*
  Moonwatchy. Watch with moon phases and calendar, based on Watchy_GSR
	 */


#include "Watchy_GSR.h"
#include "gfx-utf8.h"
#include "UechiGothic20num.h"    //numbers for 12-hour face
#include "Gogh_ExtraBold7A_T.h"  //A-T, for "HAFTING" on the watch face
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

//Colors
#define FG GxEPD_BLACK
#define BG GxEPD_WHITE

//Picture of watch owner
#include "Helge.h"

//Constant for moon phase calculation:
//Source: http://individual.utoronto.ca/kalendis/lunar/#FALC
#define SYNODIC_MONTH (29.53058770576)

//Macro simplifying the use of fontsets
#define USEFONTSET(f) setFontSet((f), sizeof(f)/sizeof(const GFXfont *))

//Expose Watchy_GSR stuff that is needed:
extern RTC_DATA_ATTR     uint8_t   Alarms_Hour[4];
extern RTC_DATA_ATTR     uint8_t   Alarms_Minutes[4];
extern RTC_DATA_ATTR     uint16_t  Alarms_Active[4];   

//Font sets for UTF-8 printing:
const GFXfont *Engine5pt[2] = {&Engine5pt7b, &Engine5pt8b}; 

const GFXfont *leaguegothic12pt[4] = {&leaguegothic_regular_webfont12pt7b, &leaguegothic_regular_webfont12pt8b, &FreeSerif8pt8b, &FreeSerif12pt8b};

//Various texts, using utf8
char const * const monthname[12] = {"JAN", "FEB", "MAR", "APR", "MAI", "JUN", "JUL", "AUG", "SEP", "OKT", "NOV", "DES"};

char const * const monthfullname[12] = {"januar", "februar", "mars", "april", "mai", "juni", "juli", "august", "september", "oktober", "november", "desember"};

char const * const dayname2[7] = {"ma", "ti", "on", "to", "fr", "lø", "sø"};
char const * const dayname[7] = {"SØN", "MAN", "TIR", "ONS", "TOR", "FRE", "LØR"};
const char *num[32] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31"};

//Zodiac symbols, and the dates they change:
//x=monthname-1. zodiacsym[x] if day<=zodiacdate[x], else zodiacdate[(x+1) % 12]
char const *zodiacsym[12] = {"♑︎", "♒︎", "♓︎", "♈︎", "♉︎", "♊︎", "♋︎", "♌︎", "♍︎", "♎︎", "♏︎", "♐︎"};
//                              jan feb mar apr may jun jul aug sep oct nov dec
uint8_t const zodiacdate[12] = {19, 18, 20, 19, 20, 21, 22, 22, 22, 23, 21, 21};

//index 0 is invalid, january is 1.
const int monthdays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
const int monthshift[] = {0,3,3,6,1,4,6,2,5,0,3,5};


// Place all of your data and variables here.

//RTC_DATA_ATTR uint8_t MyStyle;  // Remember RTC_DATA_ATTR for your variables so they don't get wiped on deep sleep.

RTC_DATA_ATTR uint8_t subStyle; //Moonwatch has several pages (clock, calendar, etc)

class OverrideGSR : public WatchyGSR {
	public:
		//utf8_GFX * u8display = new utf8_GFX(&display); 
		utf8_GFX *u8display;
 		OverrideGSR() : WatchyGSR() {};
		String InsertNTPServer() { return "ntp.justervesenet.no"; }
		void InsertDefaults() { AllowDefaultWatchStyles(false); };
		void InsertAddWatchStyles();
		void InsertInitWatchStyle(uint8_t StyleID);
		void InsertDrawWatchStyle(uint8_t StyleID);
		bool InsertHandlePressed(uint8_t SwitchNumber, bool &Haptic, bool &Refresh);
		char const *zodiacsign(int month, int day);
		float moonphase(int year, int month, int day, int hour, int minute);
		bool alarm_active_59min(int8_t alno);
		void drawAlarmMin(int8_t min);
		void drawDate(int16_t x, float y);
		void draw12hours();
		void draw24hours();
		void drawMinuteMarks();
		void draw12hourHands(uint8_t hour12);
		void draw24hourHands();
		void drawNamePlate(float x, int16_t y);
		void drawMoon();
		void drawCalendar(uint8_t Month, uint16_t Year);
		void drawOwner();
};

/*
 * Keep your functions inside the class, but at the bottom to avoid confusion.
 * Be sure to visit https://github.com/GuruSR/Watchy_GSR/blob/main/Override%20Information.md for full information on how to override
 * including functions that are available to your override to enhance functionality.
*/
		

/*
    void OverrideGSR::InsertPost(){
    };
*/


   

/*
    bool OverrideGSR::OverrideBitmap(){
      return false;
    };
*/

/*
    void InsertOnMinute(){
    };
*/

/*
    void InsertWiFi(){
    };
*/

/*
    void InsertWiFiEnding(){
    };
*/

// The next 3 functions allow you to add your own WatchFaces, there are examples that do work below.

    void OverrideGSR::InsertAddWatchStyles(){
      AddWatchStyle("Moonwatch 12"); 
			AddWatchStyle("Moonwatch 24");
    };


    //Needed to get a font in the menu, otherwise not used.
    void OverrideGSR::InsertInitWatchStyle(uint8_t StyleID){
      if (1 /*StyleID == MyStyle */){
          Design.Menu.Top = 72;
          Design.Menu.Header = 25;
          Design.Menu.Data = 66;
          Design.Menu.Gutter = 3;
          Design.Menu.Font = &aAntiCorona12pt7b;
          Design.Menu.FontSmall = &aAntiCorona11pt7b;
          Design.Menu.FontSmaller = &aAntiCorona10pt7b;
          Design.Face.Bitmap = nullptr;
          Design.Face.SleepBitmap = nullptr;
          Design.Face.Gutter = 4;
          Design.Face.Time = 56;
          Design.Face.TimeHeight = 45;
          Design.Face.TimeColor = GxEPD_BLACK;
          Design.Face.TimeFont = &aAntiCorona36pt7b;
          Design.Face.TimeLeft = 0;
          Design.Face.TimeStyle = WatchyGSR::dCENTER;
          Design.Face.Day = 101;
          Design.Face.DayGutter = 4;
          Design.Face.DayColor = GxEPD_BLACK;
          Design.Face.DayFont = &aAntiCorona16pt7b;
          Design.Face.DayFontSmall = &aAntiCorona15pt7b;
          Design.Face.DayFontSmaller = &aAntiCorona14pt7b;
          Design.Face.DayLeft = 0;
          Design.Face.DayStyle = WatchyGSR::dCENTER;
          Design.Face.Date = 143;
          Design.Face.DateGutter = 4;
          Design.Face.DateColor = GxEPD_BLACK;
          Design.Face.DateFont = &aAntiCorona15pt7b;
          Design.Face.DateFontSmall = &aAntiCorona14pt7b;
          Design.Face.DateFontSmaller = &aAntiCorona13pt7b;
          Design.Face.DateLeft = 0;
          Design.Face.DateStyle = WatchyGSR::dCENTER;
          Design.Face.Year = 186;
          Design.Face.YearLeft = 99;
          Design.Face.YearColor = GxEPD_BLACK;
          Design.Face.YearFont = &aAntiCorona16pt7b;
          Design.Face.YearLeft = 0;
          Design.Face.YearStyle = WatchyGSR::dCENTER;
          Design.Status.WIFIx = 5;
          Design.Status.WIFIy = 193;
          Design.Status.BATTx = 155;
          Design.Status.BATTy = 178;
      }
    };


void OverrideGSR::InsertDrawWatchStyle(uint8_t StyleID) {
	//StyleID
	//0: 12 hour hands
	//1: 24 hour hands
	if (!SafeToDraw()) return;

	u8display = new utf8_GFX(&display); //For utf-8 printing

	switch (subStyle) {
		case 0: //12 or 24 hour analog watch
			if (StyleID) {
				drawMinuteMarks();
				drawMoon();
				draw24hours();
				draw24hourHands();
        //Name and date that moves away from the short hand:
        if (WatchTime.Local.Hour >= 22 || WatchTime.Local.Hour < 2) drawNamePlate(99.5, 151);
        else drawNamePlate(99.5, 48); //normal place

        if (WatchTime.Local.Hour < 20 && WatchTime.Local.Hour >= 16) drawDate(151, 99.5);
        else drawDate(48, 99.5);

			} else {
				uint8_t hour12 = (WatchTime.Local.Hour+11) % 12 + 1;
				draw12hours();
				drawMinuteMarks();
				drawMoon();
				draw12hourHands(hour12);

				//Name and date should move out of the way of the short hand

				//Short hand needs the space about 10:30-1:30
				//10:37-1:23, avoiding unnecessary crash with the long hand
				if (hour12 >= 11 || 
						(hour12 == 10 && WatchTime.Local.Minute >= 37) ||
						(hour12 == 1 && WatchTime.Local.Minute < 23)) {
					//Move name away from the small hand
					drawNamePlate(99.5, 151);       
				} else drawNamePlate(99.5, 48); //normal place

				//Short hand needs the space about 8 to 10
				//Long hand needs the space x:40 to x:50, unless hour is 2 or 3
				//Limit at 10, but 9 goes up to 9:59, so...
				if ((hour12 >= 8 && hour12 <= 9) ||
						((hour12 < 2 || hour12 > 3) &&
						 WatchTime.Local.Minute >= 40 &&
						 WatchTime.Local.Minute <= 50
						)
					 ) drawDate(153, 99.5);
				else drawDate(46, 99.5);

			}
			break;
		case 1: //Calendar for this month
			drawCalendar(WatchTime.Local.Month, WatchTime.Local.Year + 1900);
			break;
		case 2:	//Calendar for next month
			{
				uint16_t year = WatchTime.Local.Year + 1900;
				uint8_t month = WatchTime.Local.Month + 1;
				if (month == 12) {
					month = 0;
					++year;
				}
				drawCalendar(month, year);
			}
			break;
		case 3: //Picture of owner
			drawOwner();
			break;
	}

		delete u8display; //could be more persistent?			
};


//Returns correct zodiac sign for the given day, ptr to utf8 string
char const *OverrideGSR::zodiacsign(int month, int day) {
	return (day <= zodiacdate[month]) ? zodiacsym[month] : zodiacsym[(month + 1) % 12];
}


void OverrideGSR::drawOwner() {
        display.drawBitmap(16, 0, epd_bmp_ownerHelge, 168, 200, FG);
}

void OverrideGSR::drawNamePlate(float x, int16_t y) {
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


//Returns the moon phase as a float in the range [0,1>
//0:new moon, 1/4:first quarter, 1/2:full moon 3/4: third quarter
//Based on a reference New moon 30 may 2022, 11:30 GMT
//Synodic month: 29.53059 days. This gives all future new moons.
float OverrideGSR::moonphase(int year, int month, int day, int hour, int minute) {
	int mdays = 1; //Remaining day in may, after new moon on may 30.
	int i;
	if (year == 2022) {
		for (i = 5; i < month; ++i) mdays += monthdays[i];
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
		for (i = 0; i < month; ++i) mdays += monthdays[i];
		//Was there a leap day in this year?
		if (!(year & 3) && month > 1) ++mdays; 
		mdays += day;   
	} //mdays holds the days since the reference new moon
		//Have the days, including today.
		//Now to handle the hours and minutes and timezone:
	float moondays = mdays - 1 + //Today has not passed, subtract
		               ((12.5 +    //Remaning hours of reference day
			   hour + minute / 60.0) //Time of day
		                /24);
	float moons = moondays / SYNODIC_MONTH;
	return moons - (int)moons;
}


void OverrideGSR::drawMoon() {
	//Center the appropriate moon image. Radius 30
	/*
		 Have 2 moon images for every day, except new moon days.
	 */

	//UTC, not localtime. Reference moon is UTC so...
	float phase = moonphase(WatchTime.UTC.Year + 1970,
			WatchTime.UTC.Month, WatchTime.UTC.Day,
			WatchTime.UTC.Hour, WatchTime.UTC.Minute);
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


void OverrideGSR::drawCalendar(uint8_t Month, uint16_t Year) {
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
0       3       3       6       1       4       6       2       5       0       3              5

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
	//drawFlag(3,3);
	u8display->USEFONTSET(leaguegothic12pt);
	u8display->setTextColor(FG);
	uint16_t startday = (Year - 1901) / 4 +
		Year - 1900 +
		monthshift[Month];
	uint16_t lastday = monthdays[Month];
	if (!(Year & 3)) { //Leap year specials
		if (Month > 1) ++startday;
		else if (Month == 1) ++lastday;
	}
	startday %= 7; //monday=0

	int16_t x1, y1;
	uint16_t w, h;
	//Exact moon phase when this month starts:
	float phase = moonphase(Year, Month,	1, 0, 0);
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
		if (i == WatchTime.Local.Day && Month == WatchTime.Local.Month) {
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


void OverrideGSR::draw24hourHands() {
  //Short hand
  float a = radians((WatchTime.Local.Hour+WatchTime.Local.Minute/60.0)*15);
  float sin_a, cos_a, sin_a1, cos_a1, sin_a2, cos_a2;
  float a1 = a - 0.37, a2 = a + 0.37; //suitable width in radians
                                      //Triangular hand, with white stripe
                                      //two triangles, avoid overpainting the moon
	sincosf(a, &sin_a, &cos_a);
	sincosf(a1, &sin_a1, &cos_a1);
	sincosf(a2, &sin_a2, &cos_a2);
  display.fillTriangle(100+33*sin_a, 100-33*cos_a,
      100+69*sin_a, 100-69*cos_a,
      100+33*sin_a1, 100-33*cos_a1, FG);
  display.fillTriangle(100+33*sin_a, 100-33*cos_a,
      100+69*sin_a, 100-69*cos_a,
      100+33*sin_a2, 100-33*cos_a2, FG);
  display.drawLine(100+34*sin_a, 100-34*cos_a,
      100+65*sin_a, 100-65*cos_a, BG);

  //Long thin hand with white stripe
  a = radians(WatchTime.Local.Minute * 6);
  a1 = a - 0.19; a2 = a + 0.19;
	sincosf(a, &sin_a, &cos_a);
	sincosf(a1, &sin_a1, &cos_a1);
	sincosf(a2, &sin_a2, &cos_a2);
  display.fillTriangle(100+33*sin_a2, 100-33*cos_a2,
      100+88*sin_a, 100-88*cos_a,
      100+33*sin_a1, 100-33*cos_a1, FG);
  display.drawLine(100+34*sin_a, 100-34*cos_a,
      100+65*sin_a, 100-65*cos_a, BG);
  display.drawLine(100+85*sin_a, 100-85*cos_a,
      100+98*sin_a, 100-98*cos_a, FG);
}


void OverrideGSR::draw12hourHands(uint8_t hour12) {
	//Long hand
	//60 positions, 6 degrees between minute marks
	//Polar: x=r sin a, y=-r cos a. Zero degrees is "up"

	float a, a1, a2, sin_a, cos_a, sin_a1, cos_a1, sin_a2, cos_a2;
	int16_t x1, y1;

	a = radians(WatchTime.Local.Minute * 6);
	sincosf(a, &sin_a, &cos_a);

	display.drawLine(100+79*sin_a, 100-79*cos_a, 100+98*sin_a, 100-98*cos_a, FG);

	//connecting blob
	display.fillCircle(100+34*sin_a, 100-34*cos_a, 4, FG);


	a1 = a+0.024; //right ring
	sincosf(a1, &sin_a1, &cos_a1);
	x1 = 100+42*sin_a1; y1 =  100-42*cos_a1;
	display.drawCircle(x1, y1, 4, FG);
	display.drawCircle(x1, y1, 5, FG);

	a1 = a1+0.119; //right dot .091 .110
	sincosf(a1, &sin_a1, &cos_a1);
	display.fillCircle(100+42*sin_a1, 100-42*cos_a1, 2, FG);

	a1 = a-0.040; //left ring  
	sincosf(a1, &sin_a1, &cos_a1);
	x1 = 100+49.75*sin_a1; y1 = 100-49.75*cos_a1;
	display.drawCircle(x1, y1, 4, FG);
	display.drawCircle(x1, y1, 5, FG);

	a1 = a1-0.101; //left dot .081 .105
	sincosf(a1, &sin_a1, &cos_a1);
	display.fillCircle(100+49.75*sin_a1, 100-49.75*cos_a1, 2, FG);

	//Long triangle, give this hand some mass
	a1 = a-0.050;
	sincosf(a1, &sin_a1, &cos_a1);
	x1 = 100+55*sin_a1; y1 = 100-55*cos_a1; 
	a1 = a+0.050;
	sincosf(a1, &sin_a1, &cos_a1);
	display.fillTriangle(x1, y1, 
			100+55*sin_a1, 100-55*cos_a1,
			100+81*sin_a, 100-81*cos_a, FG);

	//Blob near the tip, for visibility
	display.fillCircle(100+94.5*sin_a, 100-94.5*cos_a, 2, FG);


	//Short hand .Hour  360/12=30 degrees per hour
	a = radians((hour12+WatchTime.Local.Minute/60.0)*30);
	sincosf(a, &sin_a, &cos_a);

	//connecting blob
	display.fillCircle(100+35*sin_a, 100-35*cos_a, 5, FG);

	a1 = a-0.13; //center left ring
	sincosf(a1, &sin_a1, &cos_a1);
	x1 = 100 + 43.3*sin_a1; y1 = 100 - 43.3*cos_a1;
	display.drawCircle(x1, y1, 5, FG);
	display.drawCircle(x1, y1, 6, FG);

	a1 = a1-0.13; //left dot
	sincosf(a1, &sin_a1, &cos_a1);
	x1 = 100 + 43.4*sin_a1; y1 = 100 - 43.4*cos_a1;
	display.fillCircle(x1, y1, 2, FG);

	a1 = a+0.13; //center right ring
	sincosf(a1, &sin_a1, &cos_a1);
	x1 = 100 + 43.3*sin_a1; y1 = 100 - 43.3*cos_a1;
	display.drawCircle(x1, y1, 5, FG);
	display.drawCircle(x1, y1, 6, FG);

	a1 = a1+0.13; //right dot
	sincosf(a1, &sin_a1, &cos_a1);
	x1 = 100 + 43.4*sin_a1; y1 = 100 - 43.4*cos_a1;
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
	sincosf(a1, &sin_a1, &cos_a1);
	sincosf(a2, &sin_a2, &cos_a2);
	display.fillTriangle(100+58*sin_a1, 100-58*cos_a1,
			100+58*sin_a2, 100-58*cos_a2,
			100+74*sin_a, 100-74*cos_a, FG);
}


void OverrideGSR::draw12hours() {
	//Clockface numerals and hour marks
	//getTextBounds allows perfect placement, regardless of font
	uint16_t width, height;
	int16_t x1, y1;
	float a, sin_a, cos_a;
	display.setFont(&Uechi_Gothic20pt7b);
	for (int i = 1; i <= 12; ++i) {
		display.getTextBounds(num[i], 0, 0, &x1, &y1, &width, &height);
		a = radians(i*30);
		sincosf(a, &sin_a, &cos_a);
		display.setCursor(100+83*sin_a-width/2.0, 100-83*cos_a+height/2.0);
		display.print(num[i]);
	}
}


void OverrideGSR::draw24hours() {
	//Numbers & hour marks
	int16_t x1, y1;
	uint16_t width, height;
	float a, sin_a, cos_a;
	display.setFont(&leaguegothic_regular_webfont12pt7b);
	for (int i = 0; i <= 23; ++i) {
		display.getTextBounds(num[i], 0, 0, &x1, &y1, &width, &height);
		a = radians(i*15);
		sincosf(a, &sin_a, &cos_a);
		display.setCursor(100+87*sin_a-width/2.0, 100-87*cos_a+height/2.0);
		display.print(num[i]);
		display.fillCircle(100+71*sin_a, 100-71*cos_a, 2, FG);
	}
}


/*
	Check if an alarm will fire in 0 to 59 minutes, so this fact may
	be displayed on the watch face by highlighting a minute mark.
  A: Alarms_Hour == this hour, and Alarms_Minutes >= this minute
  B: Alarms_Hour == next hour, and Alarms_Minutes < this minute
	in case A, also check if the alarm is active on this weekday
	in case B, the next hour *might* be tomorrow, different day!
	 */
bool OverrideGSR::alarm_active_59min(int8_t alno) {
	int8_t wday = WatchTime.Local.Wday;
	if (Alarms_Hour[alno] == WatchTime.Local.Hour && 
			Alarms_Minutes[alno] >= WatchTime.Local.Minute) {} else
	if (Alarms_Hour[alno] == (WatchTime.Local.Hour + 1) % 24 &&
				  Alarms_Minutes[alno] < WatchTime.Local.Minute) {
		if (!Alarms_Hour[alno]) wday = (wday+1) % 7;
	} else return false;
	//alarm is within an hour, is it also active?
	//correct day of week, and "repeat" or "active" or "!tripped"
	//day bits: 0=sunday, 1=monday...
	int16_t testbits = (1 << wday) | 256; //correct day, and "Active"
  if ((testbits & Alarms_Active[alno]) == testbits) {
		//Need either repeat(128) or not Tripped(512)
		testbits = (Alarms_Active[alno] & (128 | 512)) ^ 512;
		if (testbits) return true;
	}		
	return false;
}

//Draw minute marks on the clock face.
//Also, show any alarms that will sound 0-59 minutes from now.
void OverrideGSR::drawMinuteMarks() {
	//Draw minute marks
	int ii = 0;
	for (int i = 0; i < 60; ++i) {
		float a = radians(i*6), sin_a, cos_a;
		sincosf(a, &sin_a, &cos_a);
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

	//Now check the alarms
	for (int j=0; j<4; ++j) if (alarm_active_59min(j)) {
		drawAlarmMin(Alarms_Minutes[j]);
	}

}


//Show that an alarm will go off at the chosen minute:
void OverrideGSR::drawAlarmMin(int8_t min) {
	float a = radians(min*6);
	float sin_a, cos_a;
	sincosf(a, &sin_a, &cos_a);
	int16_t x = 100+99.5*sin_a;
	int16_t y = 100-99.5*cos_a;
	display.fillCircle(x, y, 6, FG);
	display.fillCircle(x, y, 3, BG);
	display.drawPixel(x, y, FG);
}



void OverrideGSR::drawDate(int16_t x, float y) {
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
	display.getTextBounds(num[WatchTime.Local.Day], 0, 0, &x1m, &y1m, &widthm, &heightm);

	u8display->USEFONTSET(Engine5pt);
	u8display->getTextBounds(monthname[WatchTime.Local.Month], 0, 0, &x1b, &y1b, &widthb, &heightb);
	u8display->getTextBounds(dayname[WatchTime.Local.Wday], 0, 0, &x1t, &y1t, &widtht, &heightt);

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
	u8display->print(dayname[WatchTime.Local.Wday]);
	display.setCursor(box_xall - x1b + (boxw-widthb-4)/2, box_yb - y1b);
	u8display->print(monthname[WatchTime.Local.Month]);
	display.setCursor(box_xall - x1m + (boxw-widthm-4)/2, box_ym - y1m);
	display.print(num[WatchTime.Local.Day]);

	u8display->USEFONTSET(leaguegothic12pt);
	char const *zsign = zodiacsign(WatchTime.Local.Month, WatchTime.Local.Day);
	u8display->getTextBounds(zsign, 0, 0, &x1t, &y1t, &widtht, &heightt);
	display.setCursor(box_xall - x1t + (boxw-widtht-4)/2, box_yb + heightb + 4 - y1t);
	u8display->print(zsign);
}


bool OverrideGSR::InsertHandlePressed(uint8_t SwitchNumber, bool &Haptic, bool &Refresh) {
	switch (SwitchNumber){
		case 2: //Back
			Haptic = true;  // Cause Hptic feedback if set to true.
			Refresh = true; // Cause the screen to be refreshed (redrwawn).
			return true;  // Respond with "I used a button", so the WatchyGSR knows you actually did something with a button.
			break;
		case 3: //Up, next watch face
			subStyle = (subStyle + 1) & 3;
			showWatchFace();
			return true;
			break;
		case 4: //Down, previous watch face
			subStyle = (subStyle - 1) & 3;
			showWatchFace();
			return true;
	}
	return false;
};


/*
    bool OverrideGSR::OverrideSleepBitmap(){
      return false;
    };
*/


// Do not edit anything below this, leave all of your code above.
OverrideGSR watchy;

void setup(){
  watchy.init();
}

void loop(){}
