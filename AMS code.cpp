#include <SPI.h>
#include <SD.h>
 
// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;
 
// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
// MKRZero SD: SDCARD_SS_PIN
const int chipSelect = 4;
 
void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
 
 
  Serial.print("\nInitializing SD card...");
 
  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    while (1);
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }
 
  // print the type of card
  Serial.println();
  Serial.print("Card type:         ");
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }
 
  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    while (1);
  }
 
  Serial.print("Clusters:          ");
  Serial.println(volume.clusterCount());
  Serial.print("Blocks x Cluster:  ");
  Serial.println(volume.blocksPerCluster());
 
  Serial.print("Total Blocks:      ");
  Serial.println(volume.blocksPerCluster() * volume.clusterCount());
  Serial.println();
 
  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("Volume type is:    FAT");
  Serial.println(volume.fatType(), DEC);
 
  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)
  Serial.print("Volume size (Kb):  ");
  Serial.println(volumesize);
  Serial.print("Volume size (Mb):  ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Gb):  ");
  Serial.println((float)volumesize / 1024.0);
 
  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  root.openRoot(volume);
 
  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);
}
 
void loop(void) {
}
#include <LiquidCrystal.h>
LiquidCrystal lcd(3, 2, A0, A1, A2, A3);
 
#include <MFRC522.h> // for the RFID
#include <SPI.h> // for the RFID and SD card module
#include <SD.h> // for the SD card
#include <RTClib.h> // for the RTC
 
// define pins for RFID
#define CS_RFID 10
#define RST_RFID 9
// define select pin for SD card module
#define CS_SD 4 
 
// Create a file to store the data
File myFile;
 
// Instance of the class for RFID
MFRC522 rfid(CS_RFID, RST_RFID); 
 
// Variable to hold the tag's UID
String uidString;
 
// Instance of the class for RTC
RTC_DS1307 rtc;
 
// Define check in time
const int checkInHour = 9;
const int checkInMinute = 5;
 
//Variable to hold user check in
int userCheckInHour;
int userCheckInMinute;
 
// Pins for LEDs and buzzer
const int redLED = 6;
const int greenLED = 7;
const int buzzer = 5;
 
void setup() {
  
  // Set LEDs and buzzer as outputs
  pinMode(redLED, OUTPUT);  
  pinMode(greenLED, OUTPUT);
  pinMode(buzzer, OUTPUT);
  
  // Init Serial port
  Serial.begin(9600);
  lcd.begin(16,2);
  while(!Serial); // for Leonardo/Micro/Zero
  
  // Init SPI bus
  SPI.begin(); 
  // Init MFRC522 
  rfid.PCD_Init(); 
 
  // Setup for the SD card
  Serial.print("Initializing SD card...");
  lcd.print("Initializing ");
  lcd.setCursor(0, 1);
  lcd.print("SD card...");
  delay(3000);
  lcd.clear();
  if(!SD.begin(CS_SD)) {
    Serial.println("initialization failed!");
    lcd.print("Initializing ");
    lcd.setCursor(0, 1);
    lcd.print("failed!");
    return;
  }
  Serial.println("initialization done.");
  lcd.print("Initialization ");
  lcd.setCursor(0, 1);
  lcd.print("Done...");
 
  // Setup for the RTC  
  if(!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    lcd.clear();
    lcd.print("Couldn't find RTC");
    while(1);
  }
  else {
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  if(!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    lcd.clear();
    lcd.print("RTC Not Running!");
  }
}
 
void loop() {
  //look for new cards
  if(rfid.PICC_IsNewCardPresent()) {
    readRFID();
    logCard();
    verifyCheckIn();
  }
  delay(10);
}
 
void readRFID() {
  rfid.PICC_ReadCardSerial();
  lcd.clear();
  Serial.print("Tag UID: ");
  lcd.print("Tag UID: ");
  uidString = String(rfid.uid.uidByte[0]) + " " + String(rfid.uid.uidByte[1]) + " " + 
    String(rfid.uid.uidByte[2]) + " " + String(rfid.uid.uidByte[3]);
  Serial.println(uidString);
  lcd.setCursor(0, 1);
  lcd.print(uidString);
  delay(2000);
 
  // Sound the buzzer when a card is read
  tone(buzzer, 2000); 
  delay(200);        
  noTone(buzzer);
  
  delay(200);
}
 
void logCard() {
  // Enables SD card chip select pin
  digitalWrite(CS_SD,LOW);
  
  // Open file
  myFile=SD.open("DATA.txt", FILE_WRITE);
 
  // If the file opened ok, write to it
  if (myFile) {
    Serial.println("File opened ok");
    lcd.clear();
    lcd.print("File opened ok");
    delay(2000);
    myFile.print(uidString);
    myFile.print(", ");   
    
    // Save time on SD card
    DateTime now = rtc.now();
    myFile.print(now.year(), DEC);
    myFile.print('/');
    myFile.print(now.month(), DEC);
    myFile.print('/');
    myFile.print(now.day(), DEC);
    myFile.print(',');
    myFile.print(now.hour(), DEC);
    myFile.print(':');
    myFile.println(now.minute(), DEC);
    
    // Print time on Serial monitor
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.println(now.minute(), DEC);
    Serial.println("sucessfully written on SD card");
 
    lcd.clear();
    lcd.print(now.year(), DEC);
    lcd.print(':');
    lcd.print(now.month(), DEC);
    lcd.print(':');
    lcd.print(now.day(), DEC);
    lcd.print(' ');
    lcd.setCursor(11, 0);
    lcd.print(now.hour(), DEC);
    lcd.print(':');
    lcd.print(now.minute(), DEC);
    lcd.setCursor(0, 1);
    lcd.print("Written on SD...");
    delay(2000);
    
    myFile.close();
 
    // Save check in time;
    userCheckInHour = now.hour();
    userCheckInMinute = now.minute();
  }
  else {
    
    Serial.println("error opening data.txt");  
    lcd.clear();
    lcd.print("error opening data.txt");
  }
  // Disables SD card chip select pin  
  digitalWrite(CS_SD,HIGH);
}
 
void verifyCheckIn(){
  if((userCheckInHour < checkInHour)||((userCheckInHour==checkInHour) && (userCheckInMinute <= checkInMinute))){
    digitalWrite(greenLED, HIGH);
    delay(2000);
    digitalWrite(greenLED,LOW);
    Serial.println("You're welcome!");
    lcd.clear();
    lcd.print("You're Welcome!");
  }
  else{
    digitalWrite(redLED, HIGH);
    delay(2000);
    digitalWrite(redLED,LOW);
    Serial.println("You are late...");
    lcd.clear();
    lcd.print("You are Late...");
    delay(3000);
    lcd.clear();
    lcd.print("Put RFID to Scan");
    
  }
}