


/*  AATiS AS342 Locator & Uhr / Praxisheft 32, S. 36
    Modifiziert Version V3
     neu hinzu,
        Eine weitere Sicht im Display, Akutelle Zeit in ME(S)Z mit dem Datum (tnx an Mario DG1FI für den Tip)
     Dazu ist sind ein paar Änderungen bei den Bibliothek notwendig.
      - NMEA Daten vom GPS werden jetzt über die Bibliothek TinyGPSPlus ausgewertet (Version 1.1.0)
        Download Master unter https://github.com/mikalhart/TinyGPSPlus
        Die in der IDE installierbare Version von TinyGPSPlus ist zu alt und liefert nicht alle Funktionen
      - Für die Zeitfunktionen und Sommerzeitberechnung die Bibliothek  Time in Version 1.6.1, ist in der IDE Verfügbar.
      - UTC als Info zur Zeit bei der Locatoranzeige angezeigt
      - Anzeige Anzahl der empfangenen Sats und die Qualität umgestellt, nun zweizeilige am Ende einer Zeile.
        erste Zeile Sxx für die Anzahl, Qx für die Qualität

    Die Einstellungen #define AS342MOD bleiben wir bisher, für AS342 muss #define AS342MOD 0 eingestellt werden.

    Modifiziert Version
    - Taster an Pin D7 (Digital Pin) gegen Masse kurz gedrückt,  schaltet um zwischen Locator/Uhr, Koordinaten in Grad/Minuten/Sekunden und Koordinaten in Grad Minten in Dezimal
    - Taster an Pin D7 (Digital Pin) gegen Masse länger (>1 Sekunde) gedrückt, Helligkeit vom LCD kann eingestellt werden, Wenn gewünsche Helligkeit erreicht, loslassen, wird aber nicht gespeichert.
      Dazu muss Pin D6 (Digital Pin) mit Pin 15 (Beleuchtung +/Anode) vom LCD direkt verbunden werden, die alte Verbindung unterbrechen, (R3 330R auslöten) (PWM in 64 Schritten von 0 bis 255)
      An Pin6 liegt dann ein PWM Signal an, die Pulsweite steuert die Helligkeit der Hintergrundbeleuchtung.

    - Andere GPS Module, die $GNGGA anstelle von $GPGGA ausgeben werden unterstützt, $GPGGA weiterhin
    - Gibt das Modul Qualität 2 aus, so wird nun was angezeugt, war nur bei Qualität 1
    - In der Locatoranzeige wird neben der Anzahl der empfangenen Sats, die Qualität mit ausgegeben S:xx Q:n
    - Nach dem Einschalten wird in der Zeitanzeige auch die Anzahl der empfangenen Sats ausgegeben
    - Solange bei der Zeitanzeige keine Positionsdaten kommen, wird dieses mit "keine Pos." angezeigt.
    - Kommt noch keine Uhrzeit so wird --:--:-- als Uhrzeit angezeigt
    - Die Anzeige S:xx Q:n kommt auch in der Anzeige  Koordinaten in Grad/Minuten/Sekunden und Koordinaten in Grad Minten in Dezimal
    - Wenn vom GPS länger nicht kommt, Anzeige von "Keine Daten" damit es keinen  Blindflug mehr gibt.

    Über AS342MOD=1 kann eine Version erzeugt werden, bei der das GPS an rxPin = 8; txPin = 9; angeschlossen wird. Hierbei wird über diese Pins die serielle Schnittstelle fürs GPS per
    Software ( AltSoftSerial https://www.pjrc.com/teensy/td_libs_AltSoftSerial.html, verfügbar in IDE) relasiert, die Daten von Modul($G.GGA und Displayanzeige) werden dann zur seriellen Schittstelle des Arduino geschickt.

    Das LCD wird hierbei über I2C angeschlossen (PCF8574 I2C Adapter an 0x27, Anschlüsse vom PCF8574 zum Display sind vom Adapter abhängig und müssen ggf. im Quellcode (LiquidCrystal_I2C LCD...)angepasst werden
    https://github.com/fmalpartida/New-LiquidCrystal NICHT verfügbar in IDE)

*/
#define AS342MOD 1

#if AS342MOD == 0
#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

#else
#include "LiquidCrystal_I2C.h"
LiquidCrystal_I2C lcd(0x27, 4, 5, 6, 0, 1, 2, 3, 7, NEGATIVE);
/*   LiquidCrystal_I2C(uint8_t lcd_Addr, uint8_t En, uint8_t Rw, uint8_t Rs,
                     uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7,
                     uint8_t backlighPin, t_backlighPol pol);
*/
#endif
#include <TimeLib.h>
#define DEBUGln(...) Serial.println(__VA_ARGS__)

const byte pwmdat[]  = {1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 10, 11, 12, 14, 15,
                        16, 18, 19, 21, 23, 25, 27, 30, 32, 35, 38, 42, 45, 50, 54, 59, 64, 70, 76, 83, 91, 99, 108, 117, 128, 139,
                        152, 166, 181, 197, 215, 234, 255
                       };
byte helligkeitidx = 48;
boolean changehelligkeit = true;

enum ArtLATLNG {LAT = 0, LNG = 1};
enum AusgabeType {DIS = 0, SER = 1};

int displaystate = 0;
int GpsState = 0;
int OldGpsState = -1;
//boolean mustclear = true;
boolean tastergedrueckt = true;
boolean locktaster = false;
long tastermillis;
long hellmillis;
boolean moddisplaystate = false;
long OldFixStatus;

long       noGPSDatamillis = 0;
uint32_t       lastgpscharsProcessed = 0;
boolean noGPSData;
#include <AltSoftSerial.h>
const byte rxPin = 8;
const byte txPin = 9;



#include <TinyGPSPlus.h>

/*
   This sample code demonstrates the normal use of a TinyGPSPlus (TinyGPSPlus) object.
   It requires the use of SoftwareSerial, and assumes that you have a
   4800-baud serial GPS device hooked up on pins 4(rx) and 3(tx).
*/

const char compile_date[] PROGMEM = __DATE__ " " __TIME__;
const char FormatStrTIME[] PROGMEM =  "%02d:%02d:%02d %s";
const char FormatStrDATE[] PROGMEM =  "%c%c %02d.%02d.%4d";

static const int RXPin = 8, TXPin = 9;
static const uint32_t GPSBaud = 9600;
static const byte tasterPin = 7;
static const byte DisplayBeleuchtungPin = 6;
String teststr;
float h = 23;
float fac = 1;


char FormatStrbuffer[20];
char sz[32];

// The TinyGPSPlus object
TinyGPSPlus gps;

// The serial connection to the GPS device
AltSoftSerial ss(rxPin, txPin, false);

void setup()
{
#if AS342MOD == 1
  Serial.begin(115200);
  ss.begin(GPSBaud);
  Serial.println(F("input"));
  ss.setTimeout(500);
#else
  Serial.begin(GPSBaud);
#endif
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print(F("AATiS AS342 "));
  lcd.setCursor(0, 1);
  lcd.print(F("DF1HPK / DG6OU"));
  pinMode(tasterPin, INPUT_PULLUP);
  pinMode(DisplayBeleuchtungPin, OUTPUT);
  analogWrite(DisplayBeleuchtungPin, pwmdat[helligkeitidx]);
  changehelligkeit = true;
  noGPSData = true;
  OldFixStatus = 0;
  OldGpsState = -1;
}

void loop()
{
  char wday[14] = "SoMoDiMiDoFrSa";
  String spaces = "     ";
  boolean summertime;

  smartDelay(1000);
  /*
  Serial.print(gps.sentencesWithFix()); Serial.print("|");
  Serial.print(gps.time.isValid()); Serial.print("|"); Serial.print((char)gps.location.isValid()); Serial.print("|");
  Serial.print(gps.satellites.value()); Serial.print("|");
  Serial.print((char)gps.location.FixQuality()); Serial.print("|"); Serial.print(GpsState); Serial.print("|");
  Serial.print(displaystate); Serial.print("|");
  Serial.println(gps.time.age());
  */
  if (OldGpsState != GpsState) {
    lcd.clear();
    OldGpsState = GpsState;
  }

  if (( (gps.sentencesWithFix() - OldFixStatus)  > 0 ) && ! noGPSData) {
    GpsState = 5;
    OldFixStatus = gps.sentencesWithFix();
    switch (displaystate) {
      case 0:

        lcd.setCursor(0, 0);//lcd.print(displaystate + 48);
        sz[0] = 0;
        strcpy_P(FormatStrbuffer, FormatStrTIME);
        snprintf(sz, sizeof(sz), FormatStrbuffer , gps.time.hour(), gps.time.minute(), gps.time.second(), "UTC");
        lcd.print(sz);
        lcd.setCursor(0, 1);
        lcd.print(calcLocator((gps.location.lat()), (gps.location.lng())));
        write_AnzSatQual(gps.satellites.value(), gps.location.FixQuality());
        teststr = String(gps.altitude.meters(), 0) + "m" ;
        teststr = spaces.substring(1, 6 - teststr.length()) + teststr;
        lcd.setCursor(8, 1);
        lcd.print(String(teststr ));
        break;
      case 1:
        lcd.setCursor(0, 0); //lcd.print(displaystate + 48);
        lcd.print(DegreesToDegMinSec(gps.location.lat(), LAT, DIS));
        lcd.setCursor(0, 1);
        lcd.print(DegreesToDegMinSec(gps.location.lng(), LNG, DIS));
        break;
      case 2:
        lcd.setCursor(0, 0); //lcd.print(displaystate + 48);
        lcd.print(abs(gps.location.lat()), 6);
        lcd.print(((gps.location.lat() > 0) ? "N" : "S"));
        lcd.setCursor(0, 1);
        lcd.print(abs(gps.location.lng()), 6);
        lcd.print(((gps.location.lng() > 0) ? "E" : "W"));
        break;
      case 3:
        lcd.setCursor(0, 0);
        setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day() , gps.date.month(), gps.date.year());
        if (summertime_EU(year(), month(), day(), hour(), 0)) {
          adjustTime(7200);
          summertime = true;
        } else {
          adjustTime(3600);
          summertime = false;
        }
        sz[0] = 0;
        strcpy_P(FormatStrbuffer, FormatStrTIME);
        snprintf(sz, sizeof(sz), FormatStrbuffer ,  hour(), minute(), second(), ((summertime) ? "MESZ" : "MEZ"));
        lcd.print(sz);
        lcd.setCursor(0, 1);
        sz[0] = 0;
        strcpy_P(FormatStrbuffer, FormatStrDATE);
        snprintf(sz, sizeof(sz), FormatStrbuffer , wday[(weekday() - 1) * 2], wday[(weekday() - 1) * 2 + 1] , day(), month(), year());
        lcd.print(sz);
        break;
    }
    write_AnzSatQual(gps.satellites.value(), gps.location.FixQuality());
#if AS342MOD == 1
    Serial.print(F("LAT=")); Serial.print(gps.location.lat(), 6); // Latitude in degrees (double)
    Serial.print(F(" LNG=")); Serial.print(gps.location.lng(), 6); // Longitude in degrees (double)
    Serial.print(F(" ALT="));  Serial.print(gps.altitude.meters());
    Serial.print(F(" LOC=")); Serial.println(calcLocator((gps.location.lat()), (gps.location.lng())));
#endif
  }
  else {
    lcd.setCursor(0, 0);
    if (noGPSData ) {
      lcd.clear();
      lcd.print(F("Keine GPS Daten"));
      GpsState = 2;
    }
    else
      //   { if (gps.sentencesWithFix() > 0) {
    { if ((gps.date.isValid()) && (gps.date.day() > 0)) {
        sz[0] = 0;
        strcpy_P(FormatStrbuffer, FormatStrTIME);
        snprintf(sz, sizeof(sz), FormatStrbuffer , gps.time.hour(), gps.time.minute(), gps.time.second(), "UTC");
        GpsState = 3;
      } else
      {
        sprintf(sz, "--:--:--");
        GpsState = 4;
      }

      lcd.print(sz);

      write_AnzSatQual(gps.satellites.value(), gps.location.FixQuality());
      lcd.setCursor(0, 1);
      /*12345678901234567 */
      lcd.print("keine Pos.");
      OldFixStatus = gps.sentencesWithFix() ;
    }
  }
#if AS342MOD == 0
  Serial.print(gps.sentencesWithFix()); Serial.print("|");
  Serial.print(GpsState); Serial.print("|");
  Serial.print(gps.satellites.value()); Serial.print("|");
  Serial.println((char)gps.location.FixQuality());
#endif
  if ((millis() - noGPSDatamillis) > 1000 && (gps.charsProcessed() - lastgpscharsProcessed) < 10) {
    Serial.println(F("No GPS data received: check wiring"));
    noGPSDatamillis = millis();
    lastgpscharsProcessed = gps.charsProcessed();
    noGPSData = true;
    GpsState = 1;
  }
  else
  {
    noGPSData = false;
    lastgpscharsProcessed = gps.charsProcessed();
  }
}

static void checktaster() {
  if ( digitalRead(tasterPin) == 0 && !tastergedrueckt) {
    tastermillis = millis();
    tastergedrueckt = true;
    hellmillis = millis();
  }

  if ( digitalRead(tasterPin) == 0  && tastergedrueckt &&  locktaster && (millis() - tastermillis) > 1000) {
    if (millis() - hellmillis > pwmdat[helligkeitidx]) {
      changehelligkeit = true;
      moddisplaystate = false;
      hellmillis = millis();
      helligkeitidx++;
      if (helligkeitidx > 64) {
        helligkeitidx = 0;
      }
    }
  }

  if ( digitalRead(tasterPin) == 0 && tastergedrueckt &&  !locktaster && (millis() - tastermillis) > 75) {
    moddisplaystate = true;
    locktaster = true;
  }

  if ( digitalRead(tasterPin) == 1 && tastergedrueckt) {
    locktaster = false;
    tastergedrueckt = false;
    changehelligkeit = false;
    if (moddisplaystate && (GpsState == 5)) {
      displaystate = displaystate + 1  ;
      moddisplaystate = false;
      OldGpsState--;
      if (displaystate >= 4 ) {
        displaystate = 0;
      }
    }
  }
  if ( changehelligkeit ) {
    analogWrite(DisplayBeleuchtungPin, pwmdat[int(helligkeitidx)]);
  }
}


static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
#if AS342MOD == 1
    while (ss.available()) {
      gps.encode(ss.read());
    }
#else
    while (Serial.available()) {
      gps.encode(Serial.read());
    }
#endif

    checktaster();
  } while (millis() - start < ms);
}

String DegreesToDegMinSec(float x, int NSEW, int Ausgabetype) {
  int ddd = x;
  char NSEWData[5] = "NSEW";
  char Hemi = NSEWData[(NSEW * 2) + ((ddd > 0) ? 0 : 1)];
  char Ausgabegradzeichen[2] = {0xdf, 0xb0}; //Grad für lcd oder konsole
  float minutesRemainder = abs(x - ddd) * 60;
  int arcMinutes = (int)minutesRemainder;
  float arcSeconds = (float)((minutesRemainder - arcMinutes) * 60);
  String gms = "";
  gms = ((abs(round(ddd)) > 9) ? "" : "0") +  String(abs(ddd)) + char(Ausgabegradzeichen[Ausgabetype] )  +  ((round(arcMinutes) > 9) ? "" : "0") + String(round(abs(arcMinutes))) + "'" +  ((round(arcSeconds) > 9) ? "" : "0" ) +  String(abs(arcSeconds + 0.0000001), 1) + "\"" + Hemi;
  return gms;
}

boolean summertime_EU(int year, byte month, byte day, byte hour, byte tzHours)
// European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// return value: returns true during Daylight Saving Time, false otherwise
{
  if (month < 3 || month > 10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
  if (month > 3 && month < 10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
  if (month == 3 && (hour + 24 * day) >= (1 + tzHours + 24 * (31 - (5 * year / 4 + 4) % 7)) || month == 10 && (hour + 24 * day) < (1 + tzHours + 24 * (31 - (5 * year / 4 + 1) % 7)))
    return true;
  else
    return false;
}

String calcLocator(float lat, float lon) {
  const double DEG_25 = 2.5 / 60 * 1000; // 2.5 degrees
  const double DEG_50 = 5.0 / 60 * 1000; // 5.0 degrees
  double dLon = min( 180 + lon, 359.999999999);
  double dLat = min( 90 + lat, 179.999999999);
  long workingLon = (long)(dLon * 1000);
  long workingLat = (long)(dLat * 1000);
  String locator = "";
  locator += char(0x41 + (int)(workingLon / 20000));
  locator += char(0x41 + (int)(workingLat / 10000));
  locator += char(0x30 + (int)((workingLon % 20000) / 2000));
  locator += char(0x30 + (int)((workingLat % 10000) / 1000));
  locator += char(0x61 + (int)((workingLon % 20000 % 2000) / DEG_50 ));
  locator += char(0x61 + (int)((workingLat % 10000 % 1000) / DEG_25 ));
  return locator;
}

void write_AnzSatQual(int numOfSat, char quality ) {
  lcd.setCursor(13 + ((numOfSat > 9) ? 0 : 1), 0);
  lcd.print("S");
  lcd.print(numOfSat);
  lcd.setCursor(14, 1);
  lcd.print("Q");
  lcd.setCursor(15, 1);
  lcd.print(quality);
}
