/*  AATiS AS342 Locator & Uhr / Praxisheft 32, S. 36
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

#define AS342MOD 0


#if AS342MOD == 0
#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
#define DEBUGln(...) Serial.println(__VA_ARGS__)
#else
#include "LiquidCrystal_I2C.h"
LiquidCrystal_I2C lcd(0x27, 4, 5, 6, 0, 1, 2, 3, 7, NEGATIVE);
/*   LiquidCrystal_I2C(uint8_t lcd_Addr, uint8_t En, uint8_t Rw, uint8_t Rs,
                     uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7,
                     uint8_t backlighPin, t_backlighPol pol);
*/
#include <AltSoftSerial.h>
const byte rxPin = 8;
const byte txPin = 9;
AltSoftSerial GPSSerial (rxPin, txPin, false);
#define DEBUGln(...) Serial.println(__VA_ARGS__)
#endif

const char compile_date[]  = __DATE__ " " __TIME__;
const byte tasterPin = 7;
const byte DisplayBeleuchtungPin = 6;
const byte pwmdat[]  = {1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 10, 11, 12, 14, 15,
                        16, 18, 19, 21, 23, 25, 27, 30, 32, 35, 38, 42, 45, 50, 54, 59, 64, 70, 76, 83, 91, 99, 108, 117, 128, 139, 152, 166, 181, 197, 215, 234, 255
                       };

enum AusgabeType {LCD = 0, SER = 1};



float calcDegrees(String input);
String calcLocator(float lat, float lon);
int displaystate = 0;
boolean locktaster = false;
boolean firstrun = true;
boolean mustclear = true;
long tastermillis;
long hellmillis;
long noGPSDataCounter;
boolean tastergedrueckt = true;
byte helligkeit = 32;
boolean changehelligkeit = true;
boolean moddisplaystate = false;
String oldquality = "0";



void setup() {
  Serial.begin(9600);
  DEBUGln(F("AATiS AS342 - DF1HPK / DG6OU"));
  DEBUGln(compile_date);

#if AS342MOD == 1
  // Define pin modes for TX and RX
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  // Set the baud rate for the SoftwareSerial object
  GPSSerial.begin(9600);
  if (GPSSerial.isListening()) {
    DEBUGln(F("GPSSerial is listening!"));
  }
#endif

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print(F("AATiS AS342"));
  lcd.setCursor(0, 1);
  lcd.print(F("DF1HPK / DG6OU"));

  pinMode(tasterPin, INPUT_PULLUP);
  pinMode(DisplayBeleuchtungPin, OUTPUT);
  changehelligkeit = true;
  analogWrite(DisplayBeleuchtungPin, helligkeit);
  delay(1000);
  lcd.setCursor(0, 1);
  lcd.print(F("Warte GPS-Daten"));
  firstrun = true;
  noGPSDataCounter = millis();
}

void write_AnzSatQual(String numOfSat, String quality ) {
  lcd.setCursor(13, 0);
  lcd.print("S");
  lcd.setCursor(14, 0);
  lcd.print(numOfSat);
  lcd.setCursor(14, 1);
  lcd.print("Q");
  lcd.setCursor(15, 1);
  lcd.print(quality);
}

void loop() {

  if ( digitalRead(tasterPin) == 0 && !tastergedrueckt) {
    tastermillis = millis();
    tastergedrueckt = true;
    hellmillis = millis();
  }

  if ( digitalRead(tasterPin) == 0  && tastergedrueckt &&  locktaster && (millis() - tastermillis) > 1000) {
    if (millis() - hellmillis > pwmdat[helligkeit]) {
      changehelligkeit = true;
      moddisplaystate = false;
      noGPSDataCounter = millis();
      hellmillis = millis();
      helligkeit++;
      if (helligkeit > 64) {
        helligkeit = 0;
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
    if (moddisplaystate) {
      displaystate = displaystate + 1  ;
      moddisplaystate = false;
      mustclear = true;
      if (displaystate >= 3 ) {
        displaystate = 0;
      }
    }
  }

#if AS342MOD == 1
  if (GPSSerial.available() > 0 && !changehelligkeit) {
    String input = GPSSerial.readStringUntil('\n');
#else
  if (Serial.available() > 0 && !changehelligkeit) {
    String input = Serial.readStringUntil('\n');
#endif

    if (input.startsWith("$GNGGA") || input.startsWith("$GPGGA")) {
      if ( firstrun ) {
        firstrun = false;
        mustclear = true;
      }
      noGPSDataCounter = millis();
      DEBUGln(input);
      input.remove(input.length() - 1, 1);
      input.remove(0, input.indexOf(',') + 1);
      String utc = input.substring(0, input.indexOf(','));
      if (utc.length() == 0) {utc="------";}
      input.remove(0, input.indexOf(',') + 1);
      String lat = input.substring(0, input.indexOf(','));
      input.remove(0, input.indexOf(',') + 1);
      lat.concat(input.substring(0, input.indexOf(',')));
      input.remove(0, input.indexOf(',') + 1);
      String lon = input.substring(0, input.indexOf(','));
      input.remove(0, input.indexOf(',') + 1);
      lon.concat(input.substring(0, input.indexOf(',')));
      input.remove(0, input.indexOf(',') + 1);
      String quality = input.substring(0, input.indexOf(','));
      input.remove(0, input.indexOf(',') + 1);
      String numOfSat = input.substring(0, input.indexOf(','));
      input.remove(0, input.indexOf(',') + 1);
      input.remove(0, input.indexOf(',') + 1);
      String alt = input.substring(0, input.indexOf(','));
      input.remove(0, input.indexOf(',') + 1);
      alt.concat(input.substring(0, input.indexOf(',')));
      if (oldquality != quality) {
        mustclear = true;
        oldquality = quality;
      }
      if (mustclear) {
        lcd.clear();
        mustclear = false;
      }
      if (!quality.equals("0")) {
        switch (displaystate ) {
          case 0:
            lcd.setCursor(0, 0);
            lcd.print(utc.substring(0, 2));
            lcd.print(":");
            lcd.print(utc.substring(2, 4));
            lcd.print(":");
            lcd.print(utc.substring(4, 6));
            lcd.setCursor(0, 1);
            lcd.print(calcLocator(calcDegrees(lat), calcDegrees(lon)));
            DEBUGln(calcDegrees(lat), 5);
            DEBUGln(calcDegrees(lon), 5);
            DEBUGln(calcLocator(calcDegrees(lat), calcDegrees(lon)));

            lcd.setCursor(9, 0);
            lcd.print("S:");
            lcd.setCursor(11, 0);
            lcd.print(numOfSat);
            lcd.setCursor(14, 0);
            lcd.print("Q");
            lcd.setCursor(15, 0);
            lcd.print(quality);

            if (alt.length() == 7) {
              lcd.setCursor(9, 1);
            } else if (alt.length() == 6) {
              lcd.setCursor(10, 1);
            } else if (alt.length() == 5) {
              lcd.setCursor(11, 1);
            } else if (alt.length() == 4) {
              lcd.setCursor(12, 1);
            } else {
              lcd.setCursor(0, 1);
            }
            lcd.print(alt);
                  DEBUGln(alt);
            break;
          case 1:
            DEBUGln(calcGMS(lat, SER));
            DEBUGln(calcGMS(lon, SER));

            lcd.setCursor(0, 0);
            lcd.print(calcGMS(lat, LCD));
            lcd.setCursor(0, 1);
            lcd.print(calcGMS(lon, LCD));
            write_AnzSatQual(numOfSat, quality);
            break;
          case 2:
            float fllat = calcDegrees(lat);
            float fllon = calcDegrees(lon);
            DEBUGln(fllat, 6);
            DEBUGln(fllon, 6);
        
            lcd.setCursor(0, 0);
            lcd.print(fllat, 6);
            lcd.print(lat.substring(lat.length() - 1));
            lcd.setCursor(0, 1);
            if (fllat >= 100) {lcd.print("0");}
            if (fllon < 10) {lcd.print("0");}
            lcd.print(fllon, 6);
            lcd.print(lon.substring(lon.length() - 1));
            write_AnzSatQual(numOfSat, quality);
            break;
        }
      } else {
        if (mustclear) {
          lcd.clear();
          mustclear = false;
        }
        lcd.setCursor(0, 0);
        lcd.print(utc.substring(0, 2));
        lcd.print(":");
        lcd.print(utc.substring(2, 4));
        lcd.print(":");
        lcd.print(utc.substring(4, 6)); 
      
        write_AnzSatQual(numOfSat, quality);
        lcd.setCursor(0, 1);
                   /*12345678901234567 */
          lcd.print("keine Pos.");

      }
    }
  }
  else
  {
    if (  (millis() - noGPSDataCounter)  > 10000 && !changehelligkeit)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Keine GPS Daten"));
      DEBUGln(F("Keine GPS Daten"));
      DEBUGln(noGPSDataCounter);
      noGPSDataCounter = millis();
      firstrun = true;
    }
  }


  if ( changehelligkeit ) {
    //changehelligkeit = false;
    analogWrite(DisplayBeleuchtungPin, pwmdat[int(helligkeit)]);
  }

}

// NMEA Format (d)ddmm.mmmmm
// (d)dd + (mm.mmmmm / 60) = degrees
float calcDegrees(String input) {
  String direction = input.substring(input.length() - 1);
  String dddSt = input.substring(0, input.indexOf('.'));
  dddSt.remove(dddSt.length() - 2, dddSt.length() - 1);
  int ddd = dddSt.toInt();
  String mSt = input.substring(input.indexOf('.') - 2, input.length() - 1);
  float m = mSt.toFloat();
  float result = ddd + (m / 60);
  if (direction.equals("W") || direction.equals("S")) {
    result *= (-1);
  }

  return result;
}

String calcGMS(String input, int Ausgabetype) {
  char Ausgabegradzeichen[2] = {0xdf, 0xb0};
  String direction = input.substring(input.length() - 1);
  String dddSt = input.substring(0, input.indexOf('.'));
  dddSt.remove(dddSt.length() - 2, dddSt.length() - 1);
  int ddd = dddSt.toInt();
  String mSt = input.substring(input.indexOf('.') - 2, input.length() - 1);
  float m = mSt.toFloat();
  float s = (m - (int(m))) * 60;
  String gms = "";
  gms = ((round(ddd) > 9) ? "" : "0") +  String(ddd) + char(Ausgabegradzeichen[Ausgabetype] )  +  ((round(m) > 9) ? "" : "0") + String(round(m)) + "'" +  ((round(s) > 9) ? "" : "0" ) +  String(s, 1) + "\"" + direction;
  return gms;
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
