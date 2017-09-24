// SETUP

// SEKCJA TERMOMETRU
// Biblioteki do obsługi termometru
#include <OneWire.h>
#include <DS18B20.h>

// Pin do obsługi termometru
#define ONEWIRE_PIN 5

// Adres czujnika temperatury
byte address[8] = {0x28, 0xD7, 0x96, 0x4F, 0x7, 0x0, 0x0, 0x74};

// Uruchomienie funkcji termometru
OneWire onewire(ONEWIRE_PIN);
DS18B20 sensors(&onewire);

// Deklaracja zmiennej temperaturowej
// Używana przy pomiarach i podawana na wyświetlacz
float temperature = 0;

// SEKCJA POMIARU CZASU
// Istotna ze względu na wykonwanie pomiarów temperatury co określony czas

// Przechowuje ostatnią wartość pomiaru czasu
long previousMillis = 0;

// Przechowuje obecną wartość czasu
unsigned long currentMillis = 0;

// Interwał między kolejnymi pomiarami (ms)
#define INTERVAL 5000


// SEKCJA WYŚWIETLACZA
//the pins we are using
int SR_LATCH = 8;
int SR_CLOCK = 9;
int SR_DATA = 10;
int LD1 = 13;
int LD2 = 12;
int LD3 = 11;
int digits[] = {0b11111100, 0b01100000, 0b11011010, 0b11110010, 0b01100110, 0b10110110, 0b10111110, 0b11100000, 0b11111110, 0b11110110, 0b11111111};

// SEKCJA PRZYCISKU POKRĘTŁA
// Załącz bibliotekę obsługi klikania przycisku z debouncingiem
#include <Button.h>
// Podłącz przycisk do pinu nr 4 (pin - przycisk - GND)
Button middleButton = Button(4,PULLUP);

// SEKCJA BLUETOOTH
// Importuj bibliotekę obsługi portu szergowego
#include <SoftwareSerial.h>
// Utwórz stałe odpowiadające portom RX TX do komunikacji z modułem BT
#define RX 3
#define TX 4
// Utwórz obiekt komunikacji BT
SoftwareSerial Bluetooth(RX, TX);

// PEŁNA KONFIGURACJA ENKODERA
// Na podstawie https://github.com/0xPIT/encoder/blob/arduino/examples/ClickEncoderTest/ClickEncoderTest.ino
#include <TimerOne.h>
#include <ClickEncoder.h>
ClickEncoder *encoder;
int16_t last, value;
void timerIsr() {
  encoder->service();
}

// SEKCJA PAMIĘCI TRWAŁEJ
// Zapisywanie ustawień temperatury i dokładności
#include <EEPROM.h>
// Adresy pamięci gdzie mają być zapisywane wartości
#define MEMORY_SET_TEMP 0
#define MEMORY_SET_HYSTERESIS 1

// USTAWIENIA REGULATORA
// Wartość zadana temperatury
int setPointTemp = 23;
// Szerokość pętli histerezy
float hysteresisWidth = 2;
// Wartości skrajne
const int MAX_SETPOINTTEMP = 35;
const int MIN_SETPOINTTEMP = 15;
const float MAX_HYSTERESISWIDTH = 3.0;
const float MIN_HYSTERESISWIDTH = 0.5;
// Grzanie
bool heating = false;

void setup() {
  // KONIFGURACJA REJESTRU PRZESUWNEGO
  pinMode(SR_LATCH, OUTPUT);
  pinMode(SR_CLOCK, OUTPUT);
  pinMode(SR_DATA, OUTPUT);
  // KONIFGURACJA EKRANU LED
  pinMode(LD1, OUTPUT);
  pinMode(LD2, OUTPUT);
  pinMode(LD3, OUTPUT);
  
  // KONFIGURACJA TERMOMETRU
  // Rozdzielczość 11 bitów - dokładność do 0,125 stopnia (http://akademia.nettigo.pl/ds18b20/)
  sensors.begin(11);
  sensors.request(address);
  while(!Serial);
    Serial.begin(9600);
  // Pobierz pierwszy raz temperaturę, aby nie czekać aż minie interwał
  sensors.request(address);
  temperature = sensors.readTemperature(address);
  
  // KONFIGURACJA ENKODERA
  // Na podstawie https://github.com/0xPIT/encoder/blob/arduino/examples/ClickEncoderTest/ClickEncoderTest.ino
  // 3. wartość kreatora to przycisk enkodera
  // 4. wartość enkodera to ilość sygnałów po jakim dany jest przyrost
  encoder = new ClickEncoder(A1, A2, A0, 4);
  encoder->setAccelerationEnabled(false);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); 
  
  // KONFIGURACJA BLUETOOTH
  Bluetooth.begin(9600);
  
  // ŁADOWANIE USTAWIEŃ Z PAMIĘCI
  // Wczytywanie temperatury zadanej
  setPointTemp = EEPROM.read(MEMORY_SET_TEMP);
  // Jeśli temperatura nie mieści się w przedziale (bo np pamięć była pusta)
  // To skoryguj wartość zadaną
  if ((setPointTemp < MIN_SETPOINTTEMP) || (setPointTemp > MAX_SETPOINTTEMP))
    setPointTemp = MAX_SETPOINTTEMP;
  
  // Wczytywanie szerokości pętli histerezy
  hysteresisWidth = EEPROM.read(MEMORY_SET_HYSTERESIS);
  // Dzielimy wartość przez 10, bo w pamięci jest przechowywany int bez przecinka
  hysteresisWidth = hysteresisWidth/10;
  // Jeśli wartość nie mieści się w przedziale (bo np pamięć była pusta)
  // To skoryguj wartość dokładności
  if ((hysteresisWidth < MIN_HYSTERESISWIDTH) || (hysteresisWidth > MAX_HYSTERESISWIDTH))
    hysteresisWidth = MAX_HYSTERESISWIDTH;
  
}
// Zmienna oznaczajaca opcje menu
// 0) Wyswietlanie aktualnej temperatury
// 1) Zmiana zadanej temperatury
// 2) Zmiana szerokosci petli histerezy
int menuOption = 0;

void loop() {
  // MENU GŁÓWNE
  switch (menuOption) {
    // - 0: Wyświetlanie temperatury i kontrola sterowania
    case 0:
      // Obsługa termometru i grzejnika
      
      // Pobierz aktualną wartość czasu
      currentMillis = millis();
      // Porównaj odstęp pomiędzy pomiarami z interwałem pomiarowym
      if (currentMillis - previousMillis > INTERVAL) {
        // Zapisz ostatni czas pomiaru
        previousMillis = currentMillis;
        if (sensors.available()) {
          temperature = sensors.readTemperature(address);
          //Serial.println(temperature);
          sensors.request(address);
        }
        
        // Obliczanie odchyłki i ustawianie grzejnika
        checkHeating();
      }
      // Wyświetlanie temperatury
      displayTemp(temperature);
      break;
    // - 1: Ustawianie zadanej temperatury
    case 1:
      displayBlinking(setPointTemp, false);
      setPointTemp += encoder->getValue();
      // Kontrola granicznych wartości
      if (setPointTemp > MAX_SETPOINTTEMP)
        setPointTemp = MAX_SETPOINTTEMP;
      if (setPointTemp < MIN_SETPOINTTEMP)
        setPointTemp = MIN_SETPOINTTEMP;
      break;
    
    // - 2: Ustawianie szerokości pętli histerezy
    case 2:
      //displayBlinking(hysteresisWidth);
      displayBlinking(hysteresisWidth, true);
      hysteresisWidth += 0.5*encoder->getValue();
      // Kontrola granicznych wartości
      if (hysteresisWidth > MAX_HYSTERESISWIDTH)
        hysteresisWidth = MAX_HYSTERESISWIDTH;
      if (hysteresisWidth < MIN_HYSTERESISWIDTH)
        hysteresisWidth = MIN_HYSTERESISWIDTH;
      break;
  }
  
  // Obserwowanie naciśnięć przycisku middleButton
  if (middleButton.uniquePress())
    changeMenu();
  
  
  // Obserwacja naciśnięć z biblioteki enkodera
  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    switch (b) {
      case ClickEncoder::Clicked:
        changeMenu();
        break;
    }
  }  
}


void displayTemp(float temperature) {
  int waittime = 5;
  int digit = 0;
  int temp = 0;
  // Ponieważ temperatura jest mierzona z dokładnością do 0.5
  // Mnożę razy 10 żeby pozbyć się części ułamkowej
  // Rzutuję na int żeby można było dzielić modulo
  temp = (int) (temperature*10);

  // cyfra 1
      digitalWrite(LD3, LOW);
      digit = temp%10;
      digitalWrite(SR_LATCH, LOW);
      shiftOut(SR_DATA, SR_CLOCK, LSBFIRST, digits[digit]);
      digitalWrite(SR_LATCH, HIGH);
      digitalWrite(LD1, HIGH);
      delay(waittime);

  // cyfra 2
      digitalWrite(LD1, LOW);
      digit = (temp%100)/10;
      digitalWrite(SR_LATCH, LOW);
      // Dodajemy kropkę
      shiftOut(SR_DATA, SR_CLOCK, LSBFIRST, (digits[digit]|0b00000001));
      digitalWrite(SR_LATCH, HIGH);
      digitalWrite(LD2, HIGH);
      delay(waittime);

  // cyfra 3
    digitalWrite(LD2, LOW);
    digit = temp/100;
    digitalWrite(SR_LATCH, LOW);
    shiftOut(SR_DATA, SR_CLOCK, LSBFIRST, digits[digit]);
    digitalWrite(SR_LATCH, HIGH);
    digitalWrite(LD3, HIGH);
    delay(waittime);
}

// Procedury zmiany pozycji menu
// Używa zmiennej: menuOption
void changeMenu() {
  // Jeśli odbywało się ustawianie temp. zadanej
  if (menuOption == 1) {
    // Zapisanie ustawionej temperatury do pamięci
    // Ponieważ ustawiona temperatura mieści się w przedziale 0, 255 można bezpiecznie zapisać
    EEPROM.write(MEMORY_SET_TEMP, setPointTemp);
    //Serial.print("Zapisano temperature: ");
    //value = EEPROM.read(MEMORY_SET_TEMP);
    //Serial.println(value);
  }
  // Jeśli odbywało się wcześniej ustawianie dokładności
  if (menuOption == 2) {
    // Zapisanie ustawionej wart. p. histerezy do pamięci
    // Najpierw należy ją przekonwertować na int
    // Ustawiona wartość mieści się w przedziale 0, 255 więc można bezpiecznie zapisać
    int value = hysteresisWidth*10;
    EEPROM.write(MEMORY_SET_HYSTERESIS, value);
    //Serial.print("Zapisano szerokosc: ");
    //value = EEPROM.read(MEMORY_SET_HYSTERESIS);
    //Serial.println(value);
  }
  menuOption++;
  if (menuOption > 2)
    menuOption = 0;
  Serial.print("Zmieniono opcje menu na: ");
  Serial.println(menuOption);
}

// Funkcja wyświetlająca mrygającą cyfrę
// Używana do pokazywania mrygającej temperatury
// i szerokości pętli histerezy w celu ustawienia wartości
void displayBlinking(float input, boolean fNum) {
  int waittime = 5;
  int digit = 0;
  int number = 0;
  // Funkcja może wyświetlać liczby naturalne oraz z 1 miejscem po przecinku
  // Aby dostosować wyświetlanie liczba z przecinkiem jest mnożona przez 10
  if (fNum)
   number = 10*input;
  else
    number = input;

  // Pętla wyświetlająca kilka przebiegów wyświetlania
  // kończy się chwilą pauzy, aby wartość mrygała
  for (int i=0; i<10; i++) {
    // cyfra 1
        digitalWrite(LD3, LOW);
        digit = number%10;
        digitalWrite(SR_LATCH, LOW);
        shiftOut(SR_DATA, SR_CLOCK, LSBFIRST, digits[digit]);
        digitalWrite(SR_LATCH, HIGH);
        digitalWrite(LD1, HIGH);
        delay(waittime);
  
    // cyfra 2
        digitalWrite(LD1, LOW);
        digit = (number%100)/10;
        digitalWrite(SR_LATCH, LOW);
        if (fNum)
          shiftOut(SR_DATA, SR_CLOCK, LSBFIRST, (digits[digit]|0b00000001));
        else
          shiftOut(SR_DATA, SR_CLOCK, LSBFIRST, digits[digit]);
        digitalWrite(SR_LATCH, HIGH);
        digitalWrite(LD2, HIGH);
        delay(waittime);
  
    // cyfra 3
      digitalWrite(LD2, LOW);
      digit = number/100;
      digitalWrite(SR_LATCH, LOW);
      shiftOut(SR_DATA, SR_CLOCK, LSBFIRST, digits[digit]);
      digitalWrite(SR_LATCH, HIGH);
      digitalWrite(LD3, HIGH);
      delay(waittime);
  }
  // Po zakończeniu kilku przebiegów pętli wygaszamy na chwilę cyfry
  digitalWrite(LD3, LOW);
  delay(50);
}

// Funkcja checkHeating
// Używa zmiennych globalnych:
// - temperature (temperatura aktualna)
// - setPointTemp (tempratura zadana)
// - hysteresisWidth (szerokość pętli histerezy)
// Ustawia wartość następującej zmiennej:
// - heating (włączanie grzejnika)
void checkHeating() {
  // Odchyłka regulacji (regulator w trybie normalnym)
  float e = temperature - setPointTemp;
  Serial.print("Odchylka regulacji: ");
  Serial.println(e);
  if (e >= hysteresisWidth && heating == true) {
    heating = false;
    
  }
  
  if (e <= -1*hysteresisWidth && heating == false) {
    heating = true;
  }
  
  if (heating) {
    Serial.println("Heating ON");
    Bluetooth.println("Heating ON");
  }
  else {
    Serial.println("Heating OFF");
    Bluetooth.println("Heating OFF");
  }
    
}
