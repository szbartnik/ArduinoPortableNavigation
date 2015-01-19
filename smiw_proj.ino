#include <Arduino.h>

#include <SoftwareSerial\SoftwareSerial.h>
#include <Wire.h>

#include "InitScreen.h"
#include "Circle.h"

#define PGMT( pgm_ptr ) ( reinterpret_cast< const __FlashStringHelper * >( pgm_ptr ) )
#define LOC_MENU  -1
#define LOC_SDREC  0
#define LOC_NAV    1
#define LOC_GPSPOS 2
#define LOC_MAG    3

// Libraries
#include <SD.h>
#include "SimpleTimer.h"
#include "Lcd.h"
#include "HMC5883L.h"
#include "TinyGPS.h"

HMC5883L compass;
TinyGPS gps;
Sd2Card card;
SimpleTimer timer;

static const byte chipSelect = 10;

SoftwareSerial gpsSerial(4, 3);

static const byte DOWN_KEY = A3;
static const byte UP_KEY = A2;
static const byte EXECUTE_KEY = A1;
static const byte EXIT_KEY = A0;

byte DOWN_buttonState = LOW;
byte UP_buttonState = LOW;
byte EXECUTE_buttonState = LOW;
byte EXIT_buttonState = LOW;

byte DOWN_previousButtonState = LOW;
byte UP_previousButtonState = LOW;
byte EXECUTE_previousButtonState = LOW;
byte EXIT_previousButtonState = LOW;

char currentView;
char markedMenuOption;

byte magnetometerRefreshTimer;

int magnetometerCurrentValue;

void setup(void)
{
	gpsSerial.begin(9600);
	Serial.begin(9600);

	pinMode(DOWN_KEY, INPUT);
	pinMode(UP_KEY, INPUT);
	pinMode(EXECUTE_KEY, INPUT);
	pinMode(EXIT_KEY, INPUT);

	pinMode(chipSelect, OUTPUT);

	compass.SetMeasurementMode(Measurement_Continuous);
	compass.SetScale();

	LcdInitialise();
	LcdClear();
	LcdImage(initImg, 0, 0, 84, 6);

	delay(1000);

	currentView = -1;
	markedMenuOption = 0;
	refreshView();

	magnetometerRefreshTimer = timer.setInterval(1500, magnetometerRefreshTimerElapsed);
	timer.disable(magnetometerRefreshTimer);
}

static void refreshView()
{
	LcdGoToXY(0, 0);
	
	switch (currentView)
	{
		case LOC_MENU:
			LcdString(F("-----Menu-----"));
			LcdString(F("              "));
			LcdString(F("Nagrywanie SD "), markedMenuOption == LOC_SDREC);
			LcdString(F("Nav. do punktu"), markedMenuOption == LOC_NAV);
			LcdString(F("Pozycja GPS   "), markedMenuOption == LOC_GPSPOS);
			LcdString(F("Kompas        "), markedMenuOption == LOC_MAG);
			break;
		case LOC_MAG:
			LcdClear();
			LcdString(F("----Kompas----"));
			
			timer.enable(magnetometerRefreshTimer);
			break;
		default:
			LcdClear();
	}
}

static int freeRam()
{
	extern int __heap_start, *__brkval;
	int v;
	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

static void mySmartdelay(unsigned int ms)
{
	unsigned int start = millis();
	do
	{
		while (gpsSerial.available())
		{
			gps.encode(gpsSerial.read());
		}
	} while (millis() - start < ms);
}

static void print_float(float val, float invalid, byte len, byte prec)
{
	if (val == invalid)
	{
		while (len-- > 1)
			Serial.print('*');
		Serial.print(' ');
	}
	else
	{
		Serial.print(val, prec);
		byte vi = abs((byte)val);
		byte flen = prec + (val < 0.0 ? 2 : 1); // . and -
		flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
		for (byte i = flen; i<len; ++i)
			Serial.print(' ');
	}
	mySmartdelay(0);
}

static void print_int(unsigned long val, unsigned long invalid, int len)
{
	char sz[10];
	if (val == invalid)
		strcpy(sz, "*******");
	else
		sprintf(sz, "%ld", val);
	sz[len] = 0;
	for (int i = strlen(sz); i<len; ++i)
		sz[i] = ' ';
	if (len > 0)
		sz[len - 1] = ' ';
	Serial.print(sz);
	mySmartdelay(0);
}

static void print_str(const char *str, byte len)
{
	byte slen = strlen(str);
	for (byte i = 0; i<len; ++i)
		Serial.print(i<slen ? str[i] : ' ');
	mySmartdelay(0);
}

static void moveMenu(bool isDirectionUp)
{
	markedMenuOption = isDirectionUp 
		? (markedMenuOption == 0 
			? 3 
			: markedMenuOption - 1) 
		: (markedMenuOption == 3 
			? 0 
			: markedMenuOption + 1);
}

static void GpsTest()
{
	mySmartdelay(1000);

	float flat, flon;
	static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;

	print_int(gps.satellites(), TinyGPS::GPS_INVALID_SATELLITES, 5);
	gps.f_get_position(&flat, &flon);
	print_float(flat, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6);
	print_float(flon, TinyGPS::GPS_INVALID_F_ANGLE, 11, 6);
	print_float(gps.f_altitude(), TinyGPS::GPS_INVALID_F_ALTITUDE, 7, 2);
	print_float(gps.f_course(), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2);
	print_float(gps.f_speed_kmph(), TinyGPS::GPS_INVALID_F_SPEED, 6, 2);
	print_int(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0xFFFFFFFF : (unsigned long)TinyGPS::distance_between(flat, flon, LONDON_LAT, LONDON_LON) / 1000, 0xFFFFFFFF, 9);
	Serial.println();
}

static int ReadMagnetometer()
{
	MagnetometerScaled scaledValue = compass.ReadScaledAxis();
	float heading = atan2(scaledValue.YAxis, scaledValue.XAxis);

	heading += (4.0 + (37.0 / 60.0)) / (180 / M_PI); // correction of declination for Glewitz 
	heading += heading < 0 ? 2 * PI : 0;             // angle correction 1
	heading -= heading > 2 * PI ? 2 * PI : 0;        // angle correction 2
	heading = heading * 180 / M_PI;                  // from radians to degrees

	return (int)heading;
}

static void SdCardCheck()
{
	LcdString(F("Init SD... "));
	Serial.print(F("Init SD... "));

	if (!card.init(SPI_HALF_SPEED, chipSelect)) 
	{
		LcdString(F("SD card failure! "));
		Serial.println(F("SD card failure! "));
	}
	else 
	{
		LcdString(F("SD card ok! "));
		Serial.println(F("SD card ok! "));
	}
}

void loop()
{
	DOWN_previousButtonState = DOWN_buttonState;
	UP_previousButtonState = UP_buttonState;
	EXECUTE_previousButtonState = EXECUTE_buttonState;
	EXIT_previousButtonState = EXIT_buttonState;

	DOWN_buttonState = digitalRead(DOWN_KEY);
	UP_buttonState = digitalRead(UP_KEY);
	EXECUTE_buttonState = digitalRead(EXECUTE_KEY);
	EXIT_buttonState = digitalRead(EXIT_KEY);

	if (DOWN_buttonState == HIGH && DOWN_previousButtonState == LOW)
	{
		ButtonClicked(DOWN_KEY);
	}
	else if (UP_buttonState == HIGH && UP_previousButtonState == LOW)
	{
		ButtonClicked(UP_KEY);
	}
	else if (EXECUTE_buttonState == HIGH && EXECUTE_previousButtonState == LOW)
	{
		ButtonClicked(EXECUTE_KEY);
	}
	else if (EXIT_buttonState == HIGH && EXIT_previousButtonState == LOW)
	{
		Serial.println(freeRam());
		SdCardCheck();
		GpsTest();

		ButtonClicked(EXIT_KEY);
	}

	timer.run();
}

void magnetometerRefreshTimerElapsed()
{
	LcdImage(circleImg, 20, 2, 45, 4);

	magnetometerCurrentValue = ReadMagnetometer();

	LcdGoToXY(33, 1);
	LcdString(F("   "));

	if (magnetometerCurrentValue < 10) LcdGoToXY(39, 1);
	else if(magnetometerCurrentValue < 100) LcdGoToXY(36, 1);
	else LcdGoToXY(33, 1);

	char buffer[4];
	String str = String(magnetometerCurrentValue);
	str.toCharArray(buffer, 4);

	LcdString(buffer);

	printNorthDirection();
}

void printNorthDirection()
{
	byte x = 39;
	byte y = 32;

	x -= 15 * sin((float)magnetometerCurrentValue * 1000 / 57296);
	y -= 15 * cos((float)magnetometerCurrentValue * 1000 / 57296);

	LcdGoToXY(x, y / 8);
	LcdString("N");
}

void ButtonClicked(byte buttonId)
{
	switch (buttonId)
	{
		case DOWN_KEY:
			if (currentView == LOC_MENU) moveMenu(false);
			break;
		case UP_KEY:
			if (currentView == LOC_MENU) moveMenu(true);
			break;
		case EXECUTE_KEY:
			if (currentView == LOC_MENU)
				currentView = markedMenuOption;
			break;
		case EXIT_KEY:
			currentView = LOC_MENU;
			timer.disable(magnetometerRefreshTimer);
			break;
	}

	refreshView();
	delay(50);
}