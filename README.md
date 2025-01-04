# AATiS AS342 Locator & Uhr / Praxisheft 32, S. 36
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

