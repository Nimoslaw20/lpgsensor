#include "Adafruit_FONA.h"
#define FONA_RX 7
#define FONA_TX 8
#define FONA_RST 6

char replybuffer[255];

#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(8, 7);
SoftwareSerial *fonaSerial = &fonaSS;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

uint8_t type;



//DHT libraries
#include "dht.h"
#define dht_apin A2
dht DHT;

//LCD
#include <LiquidCrystal.h>
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


//MQ-5 Sensor and LEDs
const int gas = A0;
int gc = 0;
const int red = 9;
const int gre = 10;

bool gprs = false;
void pinModeDef() {
  pinMode(red, OUTPUT);
  pinMode(gre, OUTPUT);
  pinMode(gas, INPUT);
}


void setup() {
  // put your setup code here, to run once:
  lcd.begin(16, 2);
  //setupSMS();                      //SETUP ARDUINO AND SIM800L
  pinModeDef();                    //PINMODE DECLARATIONS
  //SendData();


  while (!Serial);

  Serial.begin(115200);
  Serial.println(F("FONA basic test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }
  type = fona.type();
  Serial.println(F("FONA is OK"));
  Serial.print(F("Found "));
  switch (type) {
    case FONA800L:
      Serial.println(F("FONA 800L")); break;
    case FONA800H:
      Serial.println(F("FONA 800H")); break;
    case FONA808_V1:
      Serial.println(F("FONA 808 (v1)")); break;
    case FONA808_V2:
      Serial.println(F("FONA 808 (v2)")); break;
    case FONA3G_A:
      Serial.println(F("FONA 3G (American)")); break;
    case FONA3G_E:
      Serial.println(F("FONA 3G (European)")); break;
    default:
      Serial.println(F("???")); break;
  }

  // Print module IMEI number.
  char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("Module IMEI: "); Serial.println(imei);
  }
  delay(1000);

  fona.setGPRSNetworkSettings(F("Internet"));
  delay(3000);
}

void loop() {
  // put your main code here, to run repeatedly:
  //CHECKING FOR GAS CONCENTRATION
  //gc = analogRead(gas);
  /*
    while (!gprs || !fona.enableGPRS(true))
    Serial.print(F("."));
    gprs = true;*/

  uint8_t n = fona.getNetworkStatus();
  Serial.print(F("Network status "));
  Serial.print(n);
  Serial.print(F(": "));
  if (n == 0) Serial.println(F("Not registered"));
  if (n == 1) {
    Serial.println(F("Registered (home)"));
    if (!gprs) {
      if (!fona.enableGPRS(true)) {
        Serial.println("Failed to activate GPRS");
      }
      else {
        Serial.println("GPRS activated");
        gprs = true;
      }
    }
  }
  if (n == 2) Serial.println(F("Not registered (searching)"));
  if (n == 3) Serial.println(F("Denied"));
  if (n == 4) Serial.println(F("Unknown"));
  if (n == 5) Serial.println(F("Registered roaming"));

  int gc = analogRead(gas);
  int chk = DHT.read11(dht_apin);


  //first line displays gas concentration
  lcd.setCursor(1, 0);
  //lcd.print(1,0);
  lcd.print("LPG:");
  lcd.print(gc);
  lcd.print(" ppm");


  //second line displays temperature and humidity
  lcd.setCursor(0, 1);
  lcd.print("T:");
  lcd.print(DHT.temperature, 0);
  lcd.print((char)223);
  lcd.print("C  ");
  //lcd.setCursor(0,1);
  lcd.print("H:");
  lcd.print(DHT.humidity, 0);
  lcd.print("%");
  delay(1000);

  //Checking threshold of gas
  if (gc <= 50) {
    digitalWrite(gre, HIGH);
    digitalWrite(red, LOW);
  }
  if (gc > 11) {
    digitalWrite(gre, LOW);
    digitalWrite(red, HIGH);
    if(n == 1)
      SMS(gc);
    //    sendSMS();   //SEND SMS IF GAS CONCENTRATION IS TOO HIGH
    delay(10000);
  }
  if (gprs) {
    uint16_t statuscode, length;
    String url = "http://lpg-mon.herokuapp.com/update?channel=oieurnkrp002-yk&conc=" + String(gc) + "&temperature=" + String(DHT.temperature);
    char con_url[90];

    url.toCharArray(con_url, sizeof(con_url));
    //Serial.println(con_url);
    if (!fona.HTTP_GET_start(con_url, &statuscode, (uint16_t *)&length)) {
      Serial.println("Failed to push data to cloud");
    }
    else {
      Serial.println("Successfully pushed readings to cloud");
    }
    fona.HTTP_GET_end();
    delay(9000);
  }
}




void SMS(int input) {
  char sendto[21] = "+233247145596";
  String message = "LPG concentration is too high.\nCurrent value: " + String(input);
  char newMessage[141];
  message.toCharArray(newMessage, sizeof(newMessage)); 

  if (!fona.sendSMS(sendto, newMessage)) {
    Serial.println(F("Failed to send emergency SMS"));
  } else {
    Serial.println(F("Emergency SMS Sent!"));
  }
}
