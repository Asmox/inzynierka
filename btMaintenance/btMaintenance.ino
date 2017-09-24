// SEKCJA BLUETOOTH
// Importuj bibliotekę obsługi portu szergowego
#include <SoftwareSerial.h>
// Utwórz stałe odpowiadające portom RX TX do komunikacji z modułem BT
#define RX 2
#define TX 3
// Utwórz obiekt komunikacji BT
SoftwareSerial Bluetooth(RX, TX);

// SEKCJA PRZEKAŹNIKA
#define RELAY_PIN 7

// SEKCJA POZOSTAŁYCH
// Zmienna - kontener na sygnały przychodzące z termostatu
char btChar;
String btMsg = "";

void setup() {
  Serial.begin(9600);
  // KONFIGURACJA PRZEKAŹNIKA
  pinMode(RELAY_PIN, OUTPUT);
  
  // KONFIGURACJA BLUETOOTH
  // this pin will pull the XM-15B CE pin HIGH to enable the AT mode
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  Bluetooth.begin(9600);
  delay(2000);
  
}

void loop()
{
  // Keep reading from HC-06 and send to Arduino Serial Monitor
    if (Bluetooth.available())
    {  
      Serial.write(Bluetooth.read());
        
    }
 
    // Keep reading from Arduino Serial Monitor and send to HC-06
    if (Serial.available())
    {
        Bluetooth.write(Serial.read());
    }
}
