# AS342_aatis
Modifizierter Sketch für den AATiS Bausatz AS342 Locator &amp; Uhr  / Praxisheft 32, S. 36


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
