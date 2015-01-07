#include <SoftwareSerial\SoftwareSerial.h>
#include <Wire.h>
//#include <SD.h>

#include "InitScreen.h"

// Libraries
#include "Lcd.h"
#include "HMC5883L.h"
#include "TinyGPS.h"

HMC5883L compass;
TinyGPS gps;

//Sd2Card card;
//const byte chipSelect = 10;

SoftwareSerial gpsSerial(4, 3);

void setup(void)
{
	gpsSerial.begin(9600);
	Serial.begin(9600);

	pinMode(A0, INPUT);
	pinMode(A1, INPUT);
	pinMode(A2, INPUT);
	pinMode(A3, INPUT);

	//pinMode(chipSelect, OUTPUT);
	//digitalWrite(chipSelect, HIGH);

	LcdInitialise();
	LcdClear();
	LcdImage(initImg, 0, 0, 84, 6);

	compass.SetMeasurementMode(Measurement_Continuous);
	compass.SetScale(0.88);
}

byte buttonState_0 = LOW;
byte buttonState_1 = LOW;
byte buttonState_2 = LOW;
byte buttonState_3 = LOW;

byte previousButtonState_0 = LOW;
byte previousButtonState_1 = LOW;
byte previousButtonState_2 = LOW;
byte previousButtonState_3 = LOW;

String xStr, yStr, zStr;

void loop()
{
	previousButtonState_0 = buttonState_0;
	previousButtonState_1 = buttonState_1;
	previousButtonState_2 = buttonState_2;
	previousButtonState_3 = buttonState_3;

	buttonState_0 = digitalRead(A0);
	buttonState_1 = digitalRead(A1);
	buttonState_2 = digitalRead(A2);
	buttonState_3 = digitalRead(A3);

	if (previousButtonState_0 == LOW)
	{
		if (buttonState_0 == HIGH)
		{
			LcdClear();
			LcdString("Button 0! ");
			Serial.write("Button 0! ");
			GpsTest();
			delay(200);
		}
	}

	if (previousButtonState_1 == LOW)
	{
		if (buttonState_1 == HIGH)
		{
			LcdClear();
			LcdString("Button 1! ");
			Serial.write("Button 1! ");
			SdCardCheck();
			delay(200);
		}
	}

	if (previousButtonState_2 == LOW)
	{
		if (buttonState_2 == HIGH)
		{
			LcdClear();
			LcdString("Button 2! ");
			Serial.write("Button 2! ");
			ReadMagnetometer();
			delay(200);
		}
	}

	if (previousButtonState_3 == LOW)
	{
		if (buttonState_3 == HIGH)
		{
			LcdClear();
			LcdString("Button 3! ");
			Serial.write("Button 3! ");
			delay(200);
		}
	}
}

void GpsTest()
{
	smartdelay(1000);

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

static void smartdelay(unsigned long ms)
{
	unsigned long start = millis();
	do
	{
		while (gpsSerial.available())
		{
			gps.encode(gpsSerial.read());
		}
	} while (millis() - start < ms);
}

void ReadMagnetometer()
{
	char xBuff[16];
	char yBuff[16];
	char zBuff[16];

	MagnetometerRaw scaledValue = compass.ReadRawAxis();

	xStr = String(scaledValue.XAxis);
	xStr.toCharArray(xBuff, 16);

	yStr = String(scaledValue.YAxis);
	yStr.toCharArray(yBuff, 16);

	zStr = String(scaledValue.ZAxis);
	zStr.toCharArray(zBuff, 16);

	LcdGoToXY(0, 1);
	LcdString("X: ");
	LcdString(xBuff);
	Serial.write("X: ");
	Serial.write(xBuff);

	LcdGoToXY(0, 2);
	LcdString("Y: ");
	LcdString(yBuff);
	Serial.write("Y: ");
	Serial.write(yBuff);

	LcdGoToXY(0, 3);
	LcdString("Z: ");
	LcdString(zBuff);
	Serial.write("Z: ");
	Serial.write(zBuff);
}

void SdCardCheck()
{
	LcdString("Init SD... ");
	Serial.write("Init SD... ");
	Serial.println(freeRam());
	//digitalWrite(chipSelect, LOW);

	//if (!card.init(SPI_HALF_SPEED, chipSelect)) 
	//{
	//	//LcdString("SD card failure! ");
	//	Serial.write("SD card failure! ");
	//}
	//else 
	//{
	//	//LcdString("SD card ok! ");
	//	Serial.write("SD card ok! ");
	//}
	//digitalWrite(chipSelect, HIGH);
}

int freeRam() {
	extern int __heap_start, *__brkval;
	int v;
	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

static void print_float(float val, float invalid, int len, int prec)
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
		int vi = abs((int)val);
		int flen = prec + (val < 0.0 ? 2 : 1); // . and -
		flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
		for (int i = flen; i<len; ++i)
			Serial.print(' ');
	}
	smartdelay(0);
}

static void print_int(unsigned long val, unsigned long invalid, int len)
{
	char sz[32];
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
	smartdelay(0);
}

static void print_str(const char *str, int len)
{
	int slen = strlen(str);
	for (int i = 0; i<len; ++i)
		Serial.print(i<slen ? str[i] : ' ');
	smartdelay(0);
}