
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

//Number font, large clear numerals
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

//NVS key for saving the current stepcounter
#define MW_DCNT "MW-DCNT"
//NVS keys for saving weekday stepcounts
char const * const mwdaystep[7] = {"MW-D0", "MW-D1", "MW-D2", "MW-D3", "MW-D4", "MW-D5", "MW-D6"};



//Expose Watchy_GSR stuff that is needed:
extern RTC_DATA_ATTR     uint8_t   Alarms_Hour[4];
extern RTC_DATA_ATTR     uint8_t   Alarms_Minutes[4];
extern RTC_DATA_ATTR     uint16_t  Alarms_Active[4];   

extern RTC_DATA_ATTR struct Stepping final {
    uint8_t Hour;
    uint8_t Minutes;
    bool Reset;
    uint32_t Yesterday;
} Steps;

extern RTC_DATA_ATTR struct BatteryUse final {
    float Last;             // Used to track battery changes, only updates past 0.01 in change.
    int8_t Direction;       // -1 for draining, 1 for charging.
    int8_t DarkDirection;   // Direction copy for Options.SleepMode.
    int8_t UpCount;         // Counts how many times the battery is in a direction to determine true charging.
    int8_t DownCount;
    int8_t State;           // 0=not visible, 1= showing chargeme, 2= showing reallychargeme, 3=showing charging.
    int8_t DarkState;       // Dark state of above.
    float MinLevel;         // Lowest level before the indicator comes on.
    float LowLevel;         // The battery is about to get too low for the RTC to function.
} Battery;

//Font sets for UTF-8 printing:
const GFXfont *Engine5pt[2] = {&Engine5pt7b, &Engine5pt8b}; 

//Text: leaguegothic, because it is a narrow font covering ascii & latin extensions
//Symbols taken from FreeSerif, because it has just about anything 
const GFXfont *leaguegothic12pt[4] = {&leaguegothic_regular_webfont12pt7b, &leaguegothic_regular_webfont12pt8b, &FreeSerif8pt8b, &FreeSerif12pt8b};

//Large numerals
//Does not need utf8 functionality, but printleft() uses utf8 printing, so...
const GFXfont *freesansbold15pt[1] = {&FreeSansBold15pt7b};

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


int const monthdays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
int const monthshift[] = {0,3,3,6,1,4,6,2,5,0,3,5};

//Sine table for avoiding calls to sincosf()
//sines for 0, 1, 2, ..., 89, 90 degrees. Generated programmatically.
float const sine[] = {
	0.0000000,	// 0
	0.0174524,	// 1
	0.0348995,	// 2
	0.0523360,	// 3
	0.0697565,	// 4
	0.0871557,	// 5
	0.1045285,	// 6
	0.1218693,	// 7
	0.1391731,	// 8
	0.1564345,	// 9
	0.1736482,	//10
	0.1908090,	//11
	0.2079117,	//12
	0.2249511,	//13
	0.2419219,	//14
	0.2588190,	//15
	0.2756374,	//16
	0.2923717,	//17
	0.3090170,	//18
	0.3255682,	//19
	0.3420201,	//20
	0.3583679,	//21
	0.3746066,	//22
	0.3907311,	//23
	0.4067366,	//24
	0.4226183,	//25
	0.4383712,	//26
	0.4539905,	//27
	0.4694716,	//28
	0.4848096,	//29
	0.5000000,	//30
	0.5150381,	//31
	0.5299193,	//32
	0.5446391,	//33
	0.5591929,	//34
	0.5735765,	//35
	0.5877852,	//36
	0.6018150,	//37
	0.6156615,	//38
	0.6293204,	//39
	0.6427876,	//40
	0.6560590,	//41
	0.6691306,	//42
	0.6819984,	//43
	0.6946584,	//44
	0.7071068,	//45
	0.7193398,	//46
	0.7313537,	//47
	0.7431449,	//48
	0.7547095,	//49
	0.7660444,	//50
	0.7771460,	//51
	0.7880107,	//52
	0.7986355,	//53
	0.8090170,	//54
	0.8191521,	//55
	0.8290375,	//56
	0.8386706,	//57
	0.8480481,	//58
	0.8571673,	//59
	0.8660254,	//60
	0.8746197,	//61
	0.8829476,	//62
	0.8910065,	//63
	0.8987941,	//64
	0.9063078,	//65
	0.9135455,	//66
	0.9205049,	//67
	0.9271839,	//68
	0.9335804,	//69
	0.9396926,	//70
	0.9455186,	//71
	0.9510565,	//72
	0.9563048,	//73
	0.9612617,	//74
	0.9659258,	//75
	0.9702957,	//76
	0.9743701,	//77
	0.9781476,	//78
	0.9816272,	//79
	0.9848077,	//80
	0.9876884,	//81
	0.9902681,	//82
	0.9925461,	//83
	0.9945219,	//84
	0.9961947,	//85
	0.9975641,	//86
	0.9986295,	//87
	0.9993908,	//88
	0.9998477,	//89
	1.0000000	//90
};
/*
	sines and cosines for other quadrants:
	cos(a) = sin(a+90)
	sin(360-a) = -sin(a)
	sin(180+a) = -sin(a)
	sin(180-a) = sin(a)

  
  Trigonometry:
	             sin(a)          cos(a)
	0<=a<=90     sine[a];        sine[90-a]
	90<=a<=180   sine[180-a];   -sine[a-90]
	180<=a<=270 -sine[a-180];   -sine[270-a]
	270<=a<=360 -sine[360-a];    sine[a-270]
	 */
//table-based sincos. The angle a is in degrees, not radians.
void sincost(int a, float *const sin_a, float *const cos_a) {
	//Sanitize:
	if (a > 360) a %= 360;
	else if (a < 0) a = 360 + (a % 360);
	//sincos
	if (a <= 180) {        // <= 180
		if (a <= 90) {       // <= 90
			*sin_a = sine[a];
			*cos_a = sine[90-a];
		} else {             // 90..180
			*sin_a = sine[180-a];
			*cos_a = -sine[a-90];
		}
	} else {
		if (a <= 270) {      //180..270
			*sin_a = -sine[a-180];
			*cos_a = -sine[270-a];
		} else {             //270..360
			*sin_a = -sine[360-a];
			*cos_a = sine[a-270];
		}
	}
}

// Place all of your data and variables here.

//RTC_DATA_ATTR uint8_t MyStyle;  // Remember RTC_DATA_ATTR for your variables so they don't get wiped on deep sleep.

RTC_DATA_ATTR uint8_t subStyle; //Moonwatch has several pages (clock, calendar, etc)


class OverrideGSR : public WatchyGSR {
	public:
		//utf8_GFX * u8display = new utf8_GFX(&display); 
		utf8_GFX *u8display;
 		OverrideGSR() : WatchyGSR() {};
		String InsertNTPServer() { return "ntp.justervesenet.no"; }
		void InsertDefaults() { AllowDefaultWatchStyles(false); Steps.Hour = 5; };
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
		void drawSteps(uint32_t steps);
		void drawClockSteps(uint32_t steps);
		void drawStepsPage();
		void drawStepsTable();
		void handleReboot();
		void saveStepcounter();
		void msgBox(char const * const s);
		uint32_t getCounter();
		void stepCheck();
		void printleft(int16_t x, int16_t y, char const * const s);
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

RTC_DATA_ATTR struct {
	uint32_t daysteps[7]; //[0] is steps for last sunday, [1] last monday...
	uint32_t stepOffset; //Offset for the stepcounter, from NVS
	uint8_t lastUpdate; //weekday of last shift. To avoid double updates.
} WeekSteps;

//If we are at reset time for the stepcounter,
//also update the week log. Also store the newly completed day in NVS
void OverrideGSR::stepCheck() {
	if (WatchTime.Local.Hour == Steps.Hour && WatchTime.Local.Minute == Steps.Minutes) {
		//The stepcounter was reset, and accumulated steps are in Steps.Yesterday
		//Find daynumber for yesterday
		uint8_t yesterday = (WatchTime.Local.Wday + 7 - 1) % 7;
		//Further check to see if the work is done already:
		if (WeekSteps.lastUpdate != yesterday) {
			Steps.Yesterday += WeekSteps.stepOffset; //Correct GSR if need be.
			WeekSteps.daysteps[yesterday] = Steps.Yesterday;
			WeekSteps.lastUpdate = yesterday;

			//Erase any explicitly saved stepcount, and zero the offset. Correct "Yesterday"
			if (WeekSteps.stepOffset) {
				WeekSteps.stepOffset = 0;
				NVS.erase(MW_DCNT);
			}

			//Also store this count to NVS, in order to survive frequent fw updates:
			NVS.setInt(mwdaystep[yesterday], Steps.Yesterday);
		}
				
  }
}


/*
	Reads the step counter, and adds in the offset from NVS
	 */
uint32_t OverrideGSR::getCounter() {
	return SBMA.getCounter() + WeekSteps.stepOffset;
}

/*
Detect reboots, by testing this flag.  Then, zero it after 
handling the fresh start (i.e. after loading stuff from flash)
	 */
RTC_DATA_ATTR uint8_t rebooted = 1;

void OverrideGSR::InsertDrawWatchStyle(uint8_t StyleID) {
	//StyleID
	//0: 12 hour hands
	//1: 24 hour hands
	if (!SafeToDraw()) return;

	u8display = new utf8_GFX(&display); //For utf-8 printing
  /*
		Substyles:
		0 main watch face (12 or 24 hour, with hands) moon phase
		  day+date+month+zodiac sign
		1 Calendar for this month
		2 Calendar for next month
		3 picture of owner
		4 ->7
		5 ->7
		6 ->3
		7 display step counter, battery status, whatever other tech info that fits

	*/
	if (rebooted) handleReboot();
	stepCheck(); 
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
				//Also, avoid the long hand when the short hand isn't in the alternate location

				//The short hand needs the space about 10:30-1:30
				//10:37-1:23, avoiding unnecessary crash with the long hand
				/*The long hand needs the space about x:55-x:05, unless the short hand points to 5,6,7
          Note that at 4:55, the sort hand almost points to 5.
					At 7:55, it almost points to 8.
				*/
				if (hour12 >= 11 || 
						(hour12 == 10 && WatchTime.Local.Minute >= 37) ||
						(hour12 == 1 && WatchTime.Local.Minute < 23) ||
						(WatchTime.Local.Minute <= 5 && (hour12 < 5 || hour12 > 7)) ||
						(WatchTime.Local.Minute >= 55 && (hour12 < 4 || hour12 > 6)) ) {
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
			drawClockSteps(getCounter());
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
		case 5: //unused
			subStyle = 3;
			//fall through deliberately
		case 3: //Picture of owner
			drawOwner();
			break;
		case 6: //Table with 8 days of steps
			drawStepsTable();
			break;
		case 4: //unused
			subStyle = 7;
			//fall through deliberately
		case 7: //Various sensor info: steps, battery. Also shows time
			drawStepsPage();
	}

		delete u8display; //could be more persistent?			
};


/*
1. Store stepcounts for a week in NVS, in order to draw a graph.
   NVS is a "filesystem" and survives reboot/poweroff/reprogramming
	 Keep the same stuff in RTC_DATA_ATTR, reload from NVS when 
	 data has been zeroed by a reflash/reboot
	 When the time comes for resetting the stepcounter, the
	 day's final count is stored into NVS.

	 Detecting a reboot: also have a RTC_DATA_ATTR inverted checksum.

2. current stepcount is also lost, when reprogramming.
   Not good, for the programming hobbyist!
	 a. save stepcount+date(day+month) to flash, when pressing up+down
	    And day+month must be adjusted for the fact that the step 
			reset time is not at midnight.
	 b. when reboot is detected, fetch stepcount from flash.
	    if it is still the same day, keep the loaded count as an
			offset that is added to all presentations of stepcount.
      SBMA.getCounter() replaced with getCounter() that 
			adds in this offset.

   
	 */

void OverrideGSR::handleReboot() {
	//Handle the reboot, by re-fetching stuff from NVS (flash)

	//Saved daily stepcount, if any:
	uint64_t datedcnt = NVS.getInt(MW_DCNT); //0 on failure
	uint8_t storedMonth = datedcnt >> 40;
	uint8_t storedDay = (datedcnt >> 32) & 255;
	if (datedcnt) if (storedDay == WatchTime.Local.Day && storedMonth == WatchTime.Local.Month) {
		//Valid stepcount found
		WeekSteps.stepOffset = datedcnt & 0xFFFF;
	} else {
		//Erase the outdated stepcount from flash, so it won't reappear next year
		NVS.erase(MW_DCNT);
		WeekSteps.stepOffset = 0;
	} else WeekSteps.stepOffset = 0;


	//Saved weekday stepcounts, if any:
	for (int i=0; i<7; ++i) {
		//No special case for "not found", as "not found" yields a zero int.
		WeekSteps.daysteps[i] = NVS.getInt(mwdaystep[i]);
	}

	//Also update Steps.Yesterday
	//The stepcounter is not reset at midnight, but at a more convenient time.
	//So "yesterday" is usually Wday-1, but could be Wday-2 when the day is new 
	//but the stepcounter is not yet reset.
	int16_t minusdays = 1;
	if (WatchTime.Local.Hour < Steps.Hour || (WatchTime.Local.Hour == Steps.Hour && WatchTime.Local.Minute < Steps.Minutes)) minusdays = 2;
	int16_t yesterday = (WatchTime.Local.Wday + 7 - minusdays) % 7;
	Steps.Yesterday = WeekSteps.daysteps[yesterday];

	rebooted = 0; 
}


/*
  Convert numbers < 100 to string.
	s[2] is supposed to be \0
	 */
void num2str(uint16_t n, char s[3]) {
	if (n >= 100) {
		s[0]=s[1]='X';
		return;
	}
	if (n >= 10) {
		s[1] = n % 10 + '0';
		s[0] = n / 10 + '0';
	} else {
		s[0] = n + '0';
		s[1] = 0;
	}
}


/*
  Prints a string left justified at the position.
  Uses whatever font that is active for u8display
*/
void OverrideGSR::printleft(int16_t x, int16_t y, char const * const s) {
	int16_t x1, y1;
	uint16_t w, h;
	u8display->getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
	display.setCursor(x-w, y);
	u8display->print(s);
}

/*
   Page with steps for 8 days, in table form.
*/
void OverrideGSR::drawStepsTable() {
	//!!!not done
	
}

//Page where the steps are important,
//Todays, yesterdays, and a graph for last week.
void OverrideGSR::drawStepsPage() {
	//Time+date
	drawDate(175,25);
	u8display->USEFONTSET(leaguegothic12pt);
	display.setCursor(35, 25);
	u8display->print("Tid:");
	display.setFont(&Uechi_Gothic20pt7b);
	display.setCursor(70, 26);
	display.print(WatchTime.Local.Hour);
	display.print(":"); 
	if (WatchTime.Local.Minute < 10) display.print("0");
	display.print(WatchTime.Local.Minute);
	display.writeFastHLine(0, 30, 150-0, FG);
	display.writeFastVLine(150, 30, 91-30, FG);

	//steps
	printleft(32, 54, "Steg:");
	printleft(32, 84, "I går:");
	display.writeFastHLine(0, 91, 200, FG);

	u8display->USEFONTSET(freesansbold15pt);
	display.setFont(&FreeSansBold15pt7b); //numeric only
	display.setCursor(45, 56);
	drawSteps(getCounter());
	display.setCursor(45,86);
	drawSteps(Steps.Yesterday);

	u8display->USEFONTSET(leaguegothic12pt); //Back to the usual font

	//Draw a graph with steps and days
	//Day names under the x axis, and a notch per day
	//7 days history, plus today. So 8 days.
	//8 days + y axis legend, 9 positions. 200 pixels. pos*200/9+100/9
	//200/9:22  100/9:11
	//scaled y axis, roughly "from 0 to max"
	//Room for a legend with 2 numbers, such as 4k, 8k.
	//Draw graph heavier than the axes, it could overlap with the X axis

	//Find the maximum, for scaling
	uint32_t maxStep = max(getCounter(), (uint32_t)3000); // < 3000 won't work
	for (int i = 0; i < 7; ++i) maxStep = max(WeekSteps.daysteps[i],  maxStep); 

	//Rounding. Max divisible by 3000
	maxStep = ((maxStep + 2999) / 3000) * 3000;

	//underline at  y=177
	//overline at y=91


	//The step counter does not reset at midnight, but a more convenient time
	//So the dayname lags between midnight and stepcounter reset time.
	uint8_t day = WatchTime.Local.Wday;
	if (WatchTime.Local.Hour < Steps.Hour || (WatchTime.Local.Hour == Steps.Hour && WatchTime.Local.Minute < Steps.Minutes)) day = (day + 7 - 1) % 7;

	//Axes
	const int xaxis = 155;
	const int xend = 200-3;
	const int yaxis = 22;
	const int yend = xaxis-60;
	const int arrl = 3; //Arrow length
	display.writeFastHLine(6, xaxis, xend-6, FG);
	display.writeFastVLine(yaxis, xaxis+1, yend-xaxis-1, FG);
	//Arrows on axes
	display.fillTriangle(xend, xaxis, xend-arrl, xaxis-arrl, xend-arrl, xaxis+arrl, FG);
	display.fillTriangle(yaxis, yend, yaxis+arrl, yend+arrl, yaxis-arrl, yend+arrl, FG);
	//Help lines
	for (int x = 33; x < 33+7*22; x += 2) {
		display.drawPixel(x, xaxis-20, FG);
		display.drawPixel(x, xaxis-40, FG);
		display.drawPixel(x, xaxis-60, FG);
	}
	for (int x = 33; x <= 33+7*22; x += 22) for (int y = 3; y < 60; y += 3) {
		display.drawPixel(x, xaxis-y, FG);
	}

	//day names under x axis, notches, and the graph.
	int16_t prev_y = -1;
	for (int i = 0; i <= 7; ++i) {
		int16_t x1, y1;
		uint16_t width, height;
		char const * const dayname = dayname2[(day + i + 7 - 1) % 7];
		u8display->getTextBounds(dayname, 0, 0, &x1, &y1, &width, &height);
		x1 = 33 + 22*i;
		display.setCursor(x1 - width/2, 175);
		u8display->print(dayname);
		display.fillTriangle(x1, xaxis-2, x1-1, xaxis-1, x1+1, xaxis-1, FG);
		int16_t y = xaxis - 60*((i<7) ? WeekSteps.daysteps[(day+i) % 7] : getCounter())/maxStep;
		if (y != xaxis) display.fillRoundRect(x1-2, y-2, 6, 6, 3, FG);
		if (prev_y > -1) {
			//Fatter line by quad drawing. 
			//For all slopes, thickness 2.
			display.drawLine(x1-21, prev_y, x1+1, y, FG);
			display.drawLine(x1-22, prev_y, x1, y, FG);
			display.drawLine(x1-22, prev_y+1, x1, y+1, FG);
			display.drawLine(x1-21, prev_y+1, x1+1, y+1, FG);
		}
		prev_y = y;	
	}

	//Numbers at y axis
	char s[3] = {0,0,0};
	display.setCursor(0, xaxis-10);
	num2str(maxStep/3000, s);
	printleft(yaxis-5, xaxis-10, s);
	num2str(2*maxStep/3000, s);
	printleft(yaxis-5, xaxis-10-20, s);
	display.setCursor(yaxis+6, xaxis-45);
	u8display->print("k");
	//Bumps/notches
	display.fillTriangle(yaxis-2, xaxis-20, yaxis-1, xaxis-21, yaxis-1, xaxis-19, FG);
	display.fillTriangle(yaxis-2, xaxis-40, yaxis-1, xaxis-41, yaxis-1, xaxis-39, FG);
	//
	if (maxStep <= 30000) for (int y = 1000; y<maxStep; y += 1000) {
		display.writeFastHLine(yaxis+1, xaxis - (float)y/maxStep * 60, 2, FG);
	}

	//battery
	display.writeFastHLine(0, 178, 199, FG);
	display.setCursor(5, 199);
	u8display->print("Batteri: ");
	float battvolt = getBatteryVoltage();
	float const battmax = 4.26;
	u8display->print(battvolt); //max is 4.26? Can calc. a percentage
															//goes down to 3.14V, -63%.
															//Still, use Battery.MinLevel, as going lower is bad for the battery.
	u8display->print("V   ");
	u8display->print((int) ((battvolt-Battery.MinLevel)/(battmax-Battery.MinLevel)*100));
	u8display->print("% ");
	u8display->print(Battery.Direction == 1 ? "☝" : "☟"); //Charging indicator
}

//Draws a number, assumed to be steps. Assumes fonts & position are set up
//Adds smiles if over 10.000 or 100.000
void OverrideGSR::drawSteps(uint32_t steps) {
	display.print(steps);
	if (steps >= 10000 ) {
		u8display->print("  ☺");
		if (steps >= 100000) u8display->print("☻"); //Something of an easter egg...
	}	
}


//Small step count outside the clock face circle
//To the right:  a rotating hand, rotates once per 1000 steps
//To the left: "mechanical" counter showing how many k-steps.
void OverrideGSR::drawClockSteps(uint32_t steps) {
	float sin_a, cos_a;
	int16_t rot = steps % 1000;

	//Outer ring and inner dot
	display.drawCircle(184, 184, 15, FG);
	display.fillCircle(184, 184, 4, FG);

	//10 marks around the edge
	for (int16_t a = 0; a < 360; a += 36) {
		sincost(a, &sin_a, &cos_a);
		display.drawLine(184.5 + 15*sin_a, 184.5 - 15*cos_a,
		                 184.5 + 12*sin_a, 184.5 - 12*cos_a, FG);
	}
	//Rotating hand:
	sincost(rot * 36 / 100, &sin_a, &cos_a);
	//No need to call sincos again, using the fact that:
	//sin(a+90) = cos(a), cos(a+90) = -sin(a)
	//sin(a-90) = -cos(a), cos(a-90) = sin(a)
	display.fillTriangle(184.5 + 15*sin_a, 184.5 - 15*cos_a,
	                     184.5 -  4*cos_a, 184.5 -  4*sin_a,
	                     184.5 +  4*cos_a, 184.5 +  4*sin_a,
	                     FG);
	steps /= 1000;
	u8display->USEFONTSET(leaguegothic12pt);
	int16_t x1, y1;
	uint16_t width, height;
	u8display->getTextBounds("8", 1, 196, &x1, &y1, &width, &height);
	//x1,y1: UL corner
	int digit[2]; //So maximum 99000 steps...
	digit[0] = steps % 10; steps /= 10;
	digit[1] = steps % 10; steps /= 10;
	int digit_i = 1;
	//Skip leading zeroes:
	int16_t x = 0;
	while (digit_i && ! digit[digit_i] && !steps) {
		--digit_i;
		x += width + 8;
		x1 += width + 8;
	}
	//Draw "mechanical counter"
	do {
		display.fillRect(x1-3, y1-3, width+6, height+6, FG);
		int16_t off = (digit[digit_i] == 1 || digit[digit_i] == 7) ? 1 : 0;
		display.setCursor(x+1+off, 196);
		u8display->setTextColor(BG);
		u8display->print(!steps ? num[digit[digit_i]] : "X");
		x += width + 8;
		x1 += width + 8;
	} while (digit_i--);
	u8display->setTextColor(FG);
	display.setCursor(x-1, 198);
	u8display->print("k");
}


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
	//Could also use printleft(), now that it is implemented.	
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
  int a = WatchTime.Local.Hour*15+WatchTime.Local.Minute/4; //15 degrees per hour, one degree every 4 minutes

  float sin_a, cos_a, sin_a1, cos_a1, sin_a2, cos_a2;
                                      //Triangular hand, with white stripe
                                      //two triangles, avoid overpainting the moon
	sincost(a, &sin_a, &cos_a);
	sincost(a-22, &sin_a1, &cos_a1);
	sincost(a+22, &sin_a2, &cos_a2);
  display.fillTriangle(100+33*sin_a, 100-33*cos_a,
      100+69*sin_a, 100-69*cos_a,
      100+33*sin_a1, 100-33*cos_a1, FG);
  display.fillTriangle(100+33*sin_a, 100-33*cos_a,
      100+69*sin_a, 100-69*cos_a,
      100+33*sin_a2, 100-33*cos_a2, FG);
  display.drawLine(100+34*sin_a, 100-34*cos_a,
      100+65*sin_a, 100-65*cos_a, BG);

  //Long thin hand with white stripe
  a = WatchTime.Local.Minute * 6;
	sincost(a, &sin_a, &cos_a);
	sincost(a-11, &sin_a1, &cos_a1);
	sincost(a+11, &sin_a2, &cos_a2);
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

	float sin_a, cos_a, sin_a1, cos_a1, sin_a2, cos_a2;
	int16_t x1, y1, a;

	a = WatchTime.Local.Minute * 6;
	sincost(a, &sin_a, &cos_a);

	display.drawLine(100+79*sin_a, 100-79*cos_a, 100+98*sin_a, 100-98*cos_a, FG);

	//connecting blob
	display.fillCircle(100+34*sin_a, 100-34*cos_a, 4, FG);


	//right ring
	sincost(a+2, &sin_a1, &cos_a1);
	x1 = 100+42*sin_a1; y1 =  100-42*cos_a1;
	display.drawCircle(x1, y1, 4, FG);
	display.drawCircle(x1, y1, 5, FG);

	//right dot
	sincost(a+9, &sin_a1, &cos_a1);
	display.fillCircle(100+42*sin_a1, 100-42*cos_a1, 2, FG);

	//left ring  
	sincost(a-2, &sin_a1, &cos_a1);
	x1 = 100+49.75*sin_a1; y1 = 100-49.75*cos_a1;
	display.drawCircle(x1, y1, 4, FG);
	display.drawCircle(x1, y1, 5, FG);

	//left dot 
	sincost(a-8, &sin_a1, &cos_a1);
	display.fillCircle(100+49.75*sin_a1, 100-49.75*cos_a1, 2, FG);

	//Long triangle, give this hand some mass
	sincost(a-3, &sin_a1, &cos_a1);
	x1 = 100+55*sin_a1; y1 = 100-55*cos_a1; 
	sincost(a+3, &sin_a1, &cos_a1);
	display.fillTriangle(x1, y1, 
			100+55*sin_a1, 100-55*cos_a1,
			100+81*sin_a, 100-81*cos_a, FG);

	//Blob near the tip, for visibility
	display.fillCircle(100+94.5*sin_a, 100-94.5*cos_a, 2, FG);

	
	//Short hand .Hour  360/12=30 degrees per hour
	a = hour12*30 + WatchTime.Local.Minute/2; //30 degrees per hour, half a degree per minute
	sincost(a, &sin_a, &cos_a);

	//connecting blob
	display.fillCircle(100+35*sin_a, 100-35*cos_a, 5, FG);

	//center left ring
	sincost(a-8, &sin_a1, &cos_a1);
	x1 = 100 + 43.3*sin_a1; y1 = 100 - 43.3*cos_a1;
	display.drawCircle(x1, y1, 5, FG);
	display.drawCircle(x1, y1, 6, FG);

	//left dot
	sincost(a-16, &sin_a1, &cos_a1);
	x1 = 100 + 43.4*sin_a1; y1 = 100 - 43.4*cos_a1;
	display.fillCircle(x1, y1, 2, FG);

	//center right ring
	sincost(a+8, &sin_a1, &cos_a1);
	x1 = 100 + 43.3*sin_a1; y1 = 100 - 43.3*cos_a1;
	display.drawCircle(x1, y1, 5, FG);
	display.drawCircle(x1, y1, 6, FG);

	//right dot
	sincost(a+16, &sin_a1, &cos_a1);
	x1 = 100 + 43.4*sin_a1; y1 = 100 - 43.4*cos_a1;
	display.fillCircle(x1, y1, 2, FG);

	//outer circle
	x1 = 100 + 53.4*sin_a; y1 = 100 - 53.4*cos_a;
	display.drawCircle(x1, y1, 5, FG);
	display.drawCircle(x1, y1, 6, FG);

	//outer dot
	display.fillCircle(100+60*sin_a, 100-60*cos_a, 3, FG);

	//tip
	sincost(a-2, &sin_a1, &cos_a1);
	sincost(a+2, &sin_a2, &cos_a2);
	display.fillTriangle(100+58*sin_a1, 100-58*cos_a1,
			100+58*sin_a2, 100-58*cos_a2,
			100+74*sin_a, 100-74*cos_a, FG);   
}


void OverrideGSR::draw12hours() {
	//Clockface numerals and hour marks
	//getTextBounds allows perfect placement, regardless of font
	uint16_t width, height;
	int16_t x1, y1;
	float sin_a, cos_a;
	display.setFont(&Uechi_Gothic20pt7b);
	for (int i = 1; i <= 12; ++i) {
		display.getTextBounds(num[i], 0, 0, &x1, &y1, &width, &height);
		sincost(i*30, &sin_a, &cos_a);
		display.setCursor(100+83*sin_a-(float)width/2, 100-83*cos_a+(float)height/2);
		display.print(num[i]);
	}
}


void OverrideGSR::draw24hours() {
	//Numbers & hour marks
	int16_t x1, y1;
	uint16_t width, height;
	float sin_a, cos_a;
	display.setFont(&leaguegothic_regular_webfont12pt7b);
	for (int i = 0; i <= 23; ++i) {
		display.getTextBounds(num[i], 0, 0, &x1, &y1, &width, &height);
		sincost(i*15, &sin_a, &cos_a);
		display.setCursor(100+87*sin_a - (float)width/2, 100-87*cos_a + (float)height/2);
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
		float sin_a, cos_a;
		sincost(i*6, &sin_a, &cos_a);
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
	float sin_a, cos_a;
	sincost(min*6, &sin_a, &cos_a);
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


/* Maxes a box and displays an utf8-string at least 1s */
void OverrideGSR::msgBox(char const * const s) {
	uint16_t width, height;
	int16_t x1, y1, x, y;
	u8display->USEFONTSET(leaguegothic12pt);
	u8display->getTextBounds(s, 0, 0, &x1, &y1, &width, &height);
	x = 100 - width/2;
	y = 100 - height/2;
	display.fillRect(x-2, y-2, width+4, height+4, BG);
	display.drawRect(x-3, y-3, width+6, height+6, FG);
	display.setCursor(x-x1, y-y1);
	u8display->setTextColor(FG);
	u8display->print(s);
}

/*
	 Saves the stepcounter to NVS, so it may be restored after reboot/reflash
	 Need to save the current count AND date, so we don't restore on a later date.
	 B=NVS.getString(GTZ,S);
	 B = NVS.setString(GTZ,NewTZ);

GTZ: "GSR-TZ", identifier string
extern ArduinoNvs NVS;
setInt(String key, int8/16/32/64);

dato+stepcount går inn i en enkelt uint64_t
	 */
void OverrideGSR::saveStepcounter() {
	uint64_t datedcnt = getCounter() | ((uint64_t)WatchTime.Local.Day << 32) | ((uint64_t)WatchTime.Local.Month << 40);
	NVS.setInt(MW_DCNT, datedcnt);
  msgBox("Lagret skritteller");
	//BUG: watchy resets itself at this point. Why? Not much of a problem, but still...
}
/*
  Overrides:  
	             UP - next watch screen
						 DOWN - previous watch screen
             BACK - silence alarm (not done yet!!!)

	Special sequence:
	   From main watch face: DOWN UP BACK in the same minute. 
		 Trigger saving of steps into NVS, in preparation for a reflash.
	 */
bool OverrideGSR::InsertHandlePressed(uint8_t SwitchNumber, bool &Haptic, bool &Refresh) {
	static uint8_t special;
	switch (SwitchNumber){
		case 2: //Back
			Haptic = true;  // Cause Haptic feedback if set to true.
			Refresh = true; // Cause the screen to be refreshed (redrawn).
			if (special == 2) saveStepcounter();
			special = 0;
			return true;  // Respond with "I used a button", so the WatchyGSR knows you actually did something with a button.
			break;
		case 3: //Up, next watch face
			subStyle = (subStyle + 1) & 7;
			Refresh = true;
			special = (special == 1) ? 2 : 0; //special seq. step 2
			//Haptic = true;
			return true;
			break;
		case 4: //Down, previous watch face
			special = !subStyle ? 1 : 0; //special seq. step 1
			subStyle = (subStyle - 1) & 7;
			Refresh = true;
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
