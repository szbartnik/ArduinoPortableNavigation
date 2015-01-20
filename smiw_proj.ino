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
#define LOC_NAV2   4

// Libraries
#include <SD.h>
#include "SimpleTimer.h"
#include "Lcd.h"
#include "HMC5883L.h"
#include "TinyGPS.h"

HMC5883L compass;
TinyGPS gps;
SimpleTimer timer;

static const char* gpsFileName = "gps.txt";
static const byte SD_CHIP_SELECT = 10;

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
char markedSavedLocationEntry;
float markedLocationLatitude;
float markedLocationLongtitude;

byte magnetometerRefreshTimer;
byte gpsDataRefreshTimer;
byte navigationRefreshTimer;

void setup(void)
{
    gpsSerial.begin(9600);
    Serial.begin(9600);

    pinMode(DOWN_KEY, INPUT);
    pinMode(UP_KEY, INPUT);
    pinMode(EXECUTE_KEY, INPUT);
    pinMode(EXIT_KEY, INPUT);

    pinMode(SD_CHIP_SELECT, OUTPUT);

    compass.SetMeasurementMode(Measurement_Continuous);
    compass.SetScale();

    LcdInitialise();
    LcdClear();
    LcdImage(initImg, 0, 0, 84, 6);

    delay(1000);
    LcdClear();
    LcdGoToXY(25, 2);

    if (!SD.begin(SD_CHIP_SELECT)) {
        LcdString(F("SD error!"));
    }
    else{
        LcdString(F("SD ok!"));
    }

    delay(1000);

    currentView = -1;
    markedMenuOption = 0;
    markedSavedLocationEntry = 0;
    refreshView();

    // Magnetometer timer initialization
    magnetometerRefreshTimer = timer.setInterval(1500, magnetometerRefreshTimerElapsed);
    timer.disable(magnetometerRefreshTimer);

    // Gps timer initialization
    gpsDataRefreshTimer = timer.setInterval(2000, gpsDataRefreshTimerElapsed);
    timer.disable(gpsDataRefreshTimer);

    // Navigation timer initialization
    navigationRefreshTimer = timer.setInterval(2000, navigationRefreshTimerElapsed);
    timer.disable(navigationRefreshTimer);
}

static void refreshView()
{
	Serial.println(freeRam());
    LcdGoToXY(0, 0);
    
    switch (currentView)
    {
        case LOC_MENU:
            LcdString(F("-----Menu-----"));
            LcdString(F("              "));
            LcdString(F("Nagrywanie SD "), markedMenuOption == LOC_SDREC);
            LcdString(F("Nav.do punktu "), markedMenuOption == LOC_NAV);
            LcdString(F("Pozycja GPS   "), markedMenuOption == LOC_GPSPOS);
            LcdString(F("Kompas        "), markedMenuOption == LOC_MAG);
            break;

        case LOC_MAG:
            LcdClear();
            LcdString(F("----Kompas----"));
            timer.enable(magnetometerRefreshTimer);
            break;

        case LOC_GPSPOS:
            LcdClear();
            LcdString(F("---Dane GPS---"));
            LcdString(F("Satelity:"), true);
            LcdGoToXY(0, 2);
            LcdString(F("Lat:"), true);
            LcdGoToXY(0, 3);
            LcdString(F("Lon:"), true);
            LcdGoToXY(0, 4);
            LcdString(F("Alt:"), true);
            LcdGoToXY(0, 5);
            LcdString(F("Speed:"), true);
            timer.enable(gpsDataRefreshTimer);
            break;

        case LOC_SDREC:
            LcdClear();
            LcdString(F("-Zapis punktu-"));
            if (sdCardSaveCurrentLocation())
            {
                LcdGoToXY(15, 2);
                LcdString(F("Zapisano!"));
            }
            else
            {
                LcdGoToXY(0, 2);
                LcdString(F("GPS not fixed!"));
            }
            
            delay(2000);
            currentView = LOC_MENU;
            refreshView();
            break;

        case LOC_NAV:
            showSavedLocations();
            break;

        case LOC_NAV2:
            LcdClear();
            LcdString(F("----Nawiguj---"));
            timer.enable(navigationRefreshTimer);
            break;

        default:
            LcdClear();
    }
}

void showSavedLocations()
{
    char buffer[12];
    
    File gpsFile = SD.open(gpsFileName, FILE_WRITE);
    gpsFile.close();

    gpsFile = SD.open(gpsFileName, FILE_READ);

    for (byte i = 0; i <= markedSavedLocationEntry; i++)
    {
        markedLocationLatitude = gpsFile.parseFloat();
        markedLocationLongtitude = gpsFile.parseFloat();
    }

    gpsFile.close();

    LcdClear();
    LcdString(F("-Wybierz cel-"));

    if (markedLocationLatitude == 0.0 || markedSavedLocationEntry < 0)
    {
        if (--markedSavedLocationEntry < 0)
        {
            LcdGoToXY(0, 3);
            LcdString("No saved data!");
        }
        else showSavedLocations();
    }
    else
    {
        LcdGoToXY(15, 2);
        LcdString(dtostrf(markedLocationLatitude, 0, 6, buffer));
        LcdGoToXY(15, 3);
        LcdString(dtostrf(markedLocationLongtitude, 0, 6, buffer));
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

static int readMagnetometer()
{
    MagnetometerScaled scaledValue = compass.ReadScaledAxis();
    float heading = atan2(scaledValue.YAxis, scaledValue.XAxis);

    heading += (4.0 + (37.0 / 60.0)) / (180 / M_PI); // correction of declination for Glewitz 
    heading += heading < 0 ? 2 * PI : 0;             // angle correction 1
    heading -= heading > 2 * PI ? 2 * PI : 0;        // angle correction 2
    heading = heading * 180 / M_PI;                  // from radians to degrees

    return (int)heading;
}

static bool sdCardSaveCurrentLocation()
{
    float flat, flon;
    mySmartdelay(1000);
    gps.f_get_position(&flat, &flon);

    if (flat == TinyGPS::GPS_INVALID_F_ANGLE || flon == TinyGPS::GPS_INVALID_F_ANGLE)
        return false;

    char buffer[12];

    File gpsFile = SD.open(gpsFileName, FILE_WRITE);

    gpsFile.print(dtostrf(flat, 0, 6, buffer));
    gpsFile.print(" ");
    gpsFile.print(dtostrf(flon, 0, 6, buffer));
    gpsFile.println();

    gpsFile.close();

    return true;
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
        buttonClicked(DOWN_KEY);
    }
    else if (UP_buttonState == HIGH && UP_previousButtonState == LOW)
    {
        buttonClicked(UP_KEY);
    }
    else if (EXECUTE_buttonState == HIGH && EXECUTE_previousButtonState == LOW)
    {
        buttonClicked(EXECUTE_KEY);
    }
    else if (EXIT_buttonState == HIGH && EXIT_previousButtonState == LOW)
    {
        buttonClicked(EXIT_KEY);
    }

    timer.run();
}

void magnetometerRefreshTimerElapsed()
{
    LcdImage(circleImg, 20, 2, 45, 4);

    int magnetometerCurrentValue = readMagnetometer();

    LcdGoToXY(33, 1);
    LcdString(F("   "));

    if (magnetometerCurrentValue < 10) LcdGoToXY(39, 1);
    else if(magnetometerCurrentValue < 100) LcdGoToXY(36, 1);
    else LcdGoToXY(33, 1);

    char buffer[4];
    String str = String(magnetometerCurrentValue);
    str.toCharArray(buffer, 4);

    LcdString(buffer);

    printDirection(magnetometerCurrentValue, true);
}

void gpsDataRefreshTimerElapsed()
{
    char buffer[9];
    mySmartdelay(1000);

    // Satellites number
    byte satellites = gps.satellites();

    LcdGoToXY(60, 1);
    if (satellites == TinyGPS::GPS_INVALID_SATELLITES) {
        LcdString(F("****"));
    }
    else {
        LcdString(F("    "));
        LcdGoToXY(60, 1);
        String str = String(satellites);
        str.toCharArray(buffer, 9);
        LcdString(buffer);
    }

    // Latitude & longtitude
    float flat, flon;
    gps.f_get_position(&flat, &flon);

    LcdGoToXY(30, 2);
    if (flat == TinyGPS::GPS_INVALID_F_ANGLE || flon == TinyGPS::GPS_INVALID_ANGLE) {
        LcdString(F("*********"));
        LcdGoToXY(30, 3);
        LcdString(F("*********"));
    }
    else{
        LcdString(dtostrf(flat, 9, 6, buffer));
        LcdGoToXY(30, 3);
        LcdString(dtostrf(flon, 9, 6, buffer));
    }

    // Altitude (flat is now altitude)
    flat = gps.f_altitude();

    LcdGoToXY(30, 4);
    if (flat == TinyGPS::GPS_INVALID_F_ALTITUDE) {
        LcdString(F("*********"));
    }
    else{
        LcdString(dtostrf(flat, 9, 2, buffer));
    }

    // Speed (flat is now speed)
    flat = gps.f_speed_kmph();

    LcdGoToXY(42, 5);
    if (flat == TinyGPS::GPS_INVALID_F_SPEED) {
        LcdString(F("*******"));
    }
    else{
        LcdString(dtostrf(flat, 7, 2, buffer));
    }
}

void navigationRefreshTimerElapsed()
{
    //LcdImage(circleImg, 20, 2, 45, 4);

    //magnetometerCurrentValue = readMagnetometer();


}

void printDirection(int angle, bool isNortIndicator)
{
    byte x = 39;
    byte y = 32;

    x -= 15 * sin((float)angle * 1000 / 57296);
    y -= 15 * cos((float)angle * 1000 / 57296);

    LcdGoToXY(x, y / 8);

    if (isNortIndicator) LcdString("N");
    else LcdString("#");
}

void moveLocationsList(bool isDirectionUp)
{
    markedSavedLocationEntry = isDirectionUp
        ? (markedSavedLocationEntry < 1
            ? markedSavedLocationEntry
            : markedSavedLocationEntry - 1)
        : markedSavedLocationEntry + 1;
}

void buttonClicked(byte buttonId)
{
    switch (buttonId)
    {
        case DOWN_KEY:
            if (currentView == LOC_MENU)
                moveMenu(false);
            else if (currentView == LOC_NAV)
                moveLocationsList(false);
            break;
        case UP_KEY:
            if (currentView == LOC_MENU)
                moveMenu(true);
            else if (currentView == LOC_NAV)
                moveLocationsList(true);
            break;
        case EXECUTE_KEY:
            if (currentView == LOC_MENU)
                currentView = markedMenuOption;
            else if (currentView == LOC_NAV && markedSavedLocationEntry >= 0)
                currentView = LOC_NAV2;
            break;
        case EXIT_KEY:
            Serial.println(freeRam());
            if (currentView == LOC_NAV2)
            {
                currentView = LOC_NAV;
                timer.disable(navigationRefreshTimer);
            }
            else
            {
                currentView = LOC_MENU;
                timer.disable(magnetometerRefreshTimer);
                timer.disable(gpsDataRefreshTimer);
            }
            break;
    }

    refreshView();
    delay(50);
}