/*
Name:        MayhemMagSpoof
Purpose:     Source code for the MayhemMagSpoof project
By:          Kevin Bong & Michael Vieau
Created:     02.28.2017
Modified:    07.14.2017
Rev Level    0.7

MayhemMagSpoof Pin Mappings:
PIN D2  - Transmit LED
PIN D7  - Transmit Switch Read, INPUT_PULLUP
PIN D8  - Transmit Switch Signal, OUTPUT LOW
PIN D9  - Coil Signal 1
PIN D10 - Coil Signal 2
PIN D16 - MOSI SPI Interface
PIN D14 - MISO SPI Interface
PIN D15 - SCK/SCLK SPI Interface
PIN A0  - Digital Mode Coil Enable when High
PIN A1  - Available/External
PIN TX1 - To BC-05 Bluetooth RX
PIN RX1 - To BC-05 Bluetooth TX  
 */

#include "ascii.h"
#include <EEPROM.h>

#define CLOCK_US 200
#define BETWEEN_ZERO 53 // 53 zeros between track1 & 2

// Absolute min and max eeprom addresses.
// Actual values are hardware-dependent.
// These values can be changed e.g. to protect
// eeprom cells outside this range.
const int EEPROM_MIN_ADDR = 0;
const int EEPROM_MAX_ADDR = 1023;

const byte coilEnablePin = A0;
const byte coil1Pin = 9;
const byte coil2Pin = 10;
const byte ledPin = 2;
const byte switchoutPin = 8;
const byte switchinPin = 7;

const int sublen[] = { 32, 48, 48 };  //Track 1 skips the first 32 ascii chars, Track 2 skips the first 48 ASCII characters
const int bitlen[] = { 7, 5, 5 };  // Track 1 uses 7 bits to encode, Track 2 uses 5 bits to encode

byte currentCardNumber = 1; // The slot for the current active card
byte trackToPlay = 0; // 0=both tracks, 1=Track 1 only, 2=Track 2 only
byte ledOnOff = 1; // 0=off, 1=on
char currentTrack1[80]; // Space to store the current track 1
char currentTrack2[41]; // Space to store the current track 2

char _incomingByte;   // Incoming byte (over the serial port)
byte keepFuzzing = 0; // Indicates when to shutdown the fuzzer, 0 = Stop fuzzing.
int dir;

void setup() 
{
  pinMode (coilEnablePin, OUTPUT); 
  digitalWrite (coilEnablePin, LOW);
  pinMode (coil1Pin, OUTPUT); 
  digitalWrite (coil1Pin, LOW);
  pinMode (coil2Pin, OUTPUT); 
  digitalWrite (coil2Pin, LOW);
  pinMode (ledPin, OUTPUT); 
  digitalWrite (ledPin, LOW);
  pinMode (switchoutPin, OUTPUT); 
  digitalWrite (switchoutPin, LOW);
  pinMode (switchinPin, INPUT_PULLUP); 

  loadSettings(); // Load settings from EEPROM

  //Serial1.begin(38400); // Setup Bluetooth serial port on Serial1 (for the pro micro)
  Serial1.begin(9600); // Setup Bluetooth serial port on Serial1 (for the pro micro)

}

void loop() 
{
  // Check if menu input is waiting
  char choice = getmenuchoice();
  if (choice > 0 )
  {
    switch (choice)
    {
//      case 0x00:
//        break;
     
      case ASCII_1:
        setCardSelected(1);
        break;
        
      case ASCII_2:
        setCardSelected(2);
        break;

      case ASCII_3:
        setCardSelected(3);
        break;
        
      case ASCII_4:
        setCardSelected(4);
        break;

      case ASCII_5:
        setCardSelected(5);
        break;
        
      case ASCII_V:
        displayCurrentCards();
        break;

      case ASCII_Z:
        advancedMenu();
        break;

      case ASCII_M:
        displaymainmenu();
        break;
        
      case ASCII_I:
        initEEPROMPrompt();
        break;

      case ASCII_D:
        eeprom_serial_dump_column();
        break;

      case ASCII_E:
        flushSerialBuffer();
        editCard();
        break;   

      case ASCII_L:
        LEDonoff();
        break;   
        
      case ASCII_A:
        About();
        break;
        
      /*  
      case ASCII_F:
        if (keepFuzzing) // Already turned on, turn it off.
        {
          keepFuzzing = 0;
          Serial1.println (F("Stopping fuzzer"));
          displaymainmenu();
        }
        else
        {
          keepFuzzing = 1;
          Serial1.println (F("Echoing characters from serial input to Spoofer"));
          Serial1.println (F("Serial input should come in on Pro Micro USB port and be formatted:"));
          Serial1.println (F("string delay between (milliseconds)<tab>string track (1 or 2)<tab>string data<newline>"));
          Serial1.println (F("For example:"));
          Serial1.println (F("500  1 %B1234567890123445^PADILLA/L.                ^99011200000000000000**XXX******?"));
          Serial1.println (F("Waiting for serial input, Press F again to break"));
          serialFuzz();
        }
        break;
        */
      default:
          Serial1.println (F("**Invalid Entry**\n"));
          displaymainmenu();
        break;
    }
  } 
  // Check if button is pressed
  if (digitalRead(switchinPin) == 0)
  {
    playtrackdata();
    delay(200);
  }
}

void displaymainmenu() 
{
  loadSettings();

  Serial1.println(F("---------------------------------"));
  Serial1.println(F("Currently Enabled"));
  Serial1.print(F("Card "));
  Serial1.print (currentCardNumber);
  Serial1.print (F(" Track "));
  if (trackToPlay == 0)
  {
    Serial1.println (F("1&2"));    
  }
  else
  {
    Serial1.println(trackToPlay);
  }

  Serial1.println(F("-------------------------------------------"));
  Serial1.println(F("      Main Menu        "));
  Serial1.println(F("1-5) Change card to use"));
  Serial1.println(F("                       "));
  Serial1.println(F("E) Edit a card         "));
  Serial1.println(F("V) Display stored cards"));
  Serial1.println(F("L) LED on/off          "));
  //Serial1.println(F("F) Fuzz from USB serial"));
  Serial1.println(F("Z) Enter advanced menu "));
  Serial1.println(F("-------------------------------------------"));

  flushSerialBuffer(); // clear the serial buffer before asking for input
  
  Serial1.println(F("Select a choice: "));

}

void advancedMenu()
{
  
  Serial1.println(F("-------------------------------------------"));
  Serial1.println(F("      Advanced Menu        "));
  Serial1.println(F("A) About               "));
  Serial1.println(F("D) Dump EEPROM         "));  
  Serial1.println(F("I) Initialize EEPROM   "));
  Serial1.println(F("M) Back to main menu   "));
  Serial1.println(F("-------------------------------------------"));

  flushSerialBuffer(); // clear the serial buffer before asking for input
  
  Serial1.println(F("Select a choice: "));
  
}
char getmenuchoice () 
{
  // Returns next serial character if available, otherwise returns null byte;
  if(Serial1.available() > 0 )
  {
    _incomingByte = Serial1.read();
  }
  else
  {
    _incomingByte = 0x00;
  }
  return toUpperCase(_incomingByte);
}

void About ()
{
  Serial1.println(F("                 "));
  Serial1.println(F("-------------------------------------------"));
  Serial1.println(F("About the        "));
  Serial1.println(F("Mayhem MagSpoofer"));
  Serial1.println(F("-------------------------------------------"));
  Serial1.println(F("Version:  0.7    "));
  Serial1.println(F("                 "));
  Serial1.println(F("By: Kevin Bong & Michael Vieau"));
  Serial1.println(F("                 "));
  Serial1.println(F("More info at:    "));
  Serial1.println(F("www.mayhemlab.net"));
  Serial1.println(F("                 "));
  Serial1.println(F("M) Back to main menu"));
  Serial1.println(F("-------------------------------------------"));

  flushSerialBuffer(); // clear the serial buffer before asking for input
  
  Serial1.println(F("Select a choice: "));
}
void LEDonoff ()
{
  byte ledOnOff = 2;
  Serial1.println(F("This will allow you to turn the LED"));
  Serial1.println(F("on of off when the card is transmitted"));
  Serial1.println(F("-------------------------------------------"));
  Serial1.println(F("0) Off"));
  Serial1.println(F("1) On"));
 
  while ( ledOnOff > 1 )
  {
    if(Serial1.available() > 0 )
    {
      ledOnOff = Serial1.read() - 48;
    }
      delay (100); // wait a bit to check for a character again
  }
  EEPROM.write(5,ledOnOff);
  displaymainmenu();
}

void setCardSelected (byte cardno)
{
  Serial1.print(F("Card "));
  Serial1.print(cardno);
  Serial1.println (F(" was selected\n"));
  currentCardNumber = cardno;
  EEPROM.write(4,cardno);

  flushSerialBuffer();
  Serial1.println(F("Select track [0 = both]: "));
  byte trackToPlay = 4;
  while ( trackToPlay > 3 )
  {
    if(Serial1.available() > 0 )
    {
      trackToPlay = Serial1.read() - 48;
    }
      delay (100); // wait a bit to check for a character again
  }
  EEPROM.write(6,trackToPlay);
  displaymainmenu();
}

void flushSerialBuffer()
{
  delay(100);
  // clear the serial buffer before asking for input
  while(Serial1.available())
  { 
    char getData = Serial1.read(); // don't do anything with it.
  } 
}

void playtrackdata()
{
  //Plays the currently configured track data through the coil
  Serial1.print(F("Button Pressed, Playing Track Data for Card "));
  Serial1.print (currentCardNumber);
  Serial1.print (F(" Track"));
  if (trackToPlay == 0)
  {
    Serial1.println (F("s 1&2"));
    playBuffer(1, currentTrack1, 0);
    //delay(200);
    playBuffer(2, currentTrack2, 1); // 1 means play backward
  }
  else if (trackToPlay == 1)
  {
    Serial1.println (" 1");  
    playBuffer(1, currentTrack1, 0);
  }
  else
  {
    Serial1.println (" 2");
    playBuffer(2, currentTrack2, 0);
  }
}

void loadSettings()
{
  // Loads settings from EEPROM

  currentCardNumber = EEPROM.read(4); // The slot for the current active card
  ledOnOff = EEPROM.read(5); // 0=off, 1=on
  trackToPlay = EEPROM.read(6); // 0=both tracks, 1=Track 1 only, 2=Track 2 only
  getCardFromEEPROM(currentTrack1, currentCardNumber, 1);
  getCardFromEEPROM(currentTrack2, currentCardNumber, 2);

}

void displayCurrentCards()
{
  // Shows the cards in Slots 1 through 5
  char buffer[80];
  for (byte x = 1; x < 6; x++)
  {
    Serial1.print ("Card: ");
    Serial1.println (x);
    Serial1.print ("Trk 1:   ");
    getCardFromEEPROM(buffer, x, 1);
    Serial1.println (buffer);
    Serial1.print ("Trk 2:   ");
    getCardFromEEPROM(buffer, x, 2);
    Serial1.println (buffer);
    Serial1.println("");
  }
  displaymainmenu();
}

void editCard()
{
  flushSerialBuffer();
  Serial1.println (F("Enter card slot to edit (1-5)"));
  byte cardslot = 0;
  while (cardslot < 1)
  {
    if(Serial1.available() > 0 )
    {
      cardslot = Serial1.read() - 48;
    }
    delay (100); // wait a bit to check for a character again
  }
  if (cardslot > 5 )
  {
    Serial1.println(F("That is not a valid cardslot"));
    return;
  }
  flushSerialBuffer();
  Serial1.println (F("Enter track to edit (1-2) [4 for both]"));
  byte track = 0;
  while (track < 1)
  {
    if(Serial1.available() > 0 )
    {
      track = Serial1.read() - 48;
    }
    delay (100); // wait a bit to check for a character again
  }
  if (track > 4 )
  {
    Serial1.println(F("Track Too Big in Edit Card"));
    return;
  }
  flushSerialBuffer();
  
  if (track == 4 )
  {
    Serial1.println(F("Enter track 1 and track 2 sperated by a semicolon ( ; )"));
    char trackdata[160]; // For both tracks
    int offset = 0;
    trackdata[offset] = 1;
    while (trackdata[offset] != 0x00) // Keep reading until end of string
    {
      byte _incomingByte;
      if(Serial1.available() > 0 )
      {
        _incomingByte = Serial1.read();
        if ((_incomingByte == ASCII_ENTER) or (_incomingByte == ASCII_NEWLINE))
       {
         trackdata[offset] = 0x00; // End the string with the null character
       }
       else if (offset > 156)
       {
          // Too many characters input maybe, just chop it off here
          Serial1.println(F("Too many characters entered for card in Edit Card"));
         trackdata[offset] = 0x00;
       }
       else
       {
          trackdata[offset] = _incomingByte;
         offset++;
         trackdata[offset] = 1; // The next character goes here, right now its gibberish but it can't be 0 or we stop reading characters
       }
       }
     }
      // http://forum.arduino.cc/index.php?topic=41215.0
     char *p = trackdata;
     char *str;
     int track = 0;
     
     while ((str = strtok_r(p,";",&p))!=NULL)
     {
      track++;
        if (track == 1)
        {
          offset = 121 * cardslot; // Track 1
          eeprom_write_string(offset, str);
        }
        else if (track == 2)
        {
          offset = (121 * cardslot) + 80; //Track 2
          eeprom_write_string(offset, ";");
          offset = (121 * cardslot) + 81; //Track 2
          eeprom_write_string(offset, str);
        }
     }
     Serial1.println(F("Done editing card"));
     loadSettings();
     displayCurrentCards();
     flushSerialBuffer();
  }
  
  Serial1.println (F("Enter the card data, followed by a return"));
  char trackdata[80];
  int offset = 0;
  trackdata[offset] = 1;

  while (trackdata[offset] != 0x00) // Keep reading until end of string
  {
    byte _incomingByte;
    if(Serial1.available() > 0 )
    {
      _incomingByte = Serial1.read();
      if ((_incomingByte == ASCII_ENTER) or (_incomingByte == ASCII_NEWLINE))
      {
        trackdata[offset] = 0x00; // End the string with the null character
      }
      else if (offset > 78)
      {
        // Too many characters input maybe, just chop it off here
        Serial1.println(F("Too many characters entered for card in Edit Card"));
        trackdata[offset] = 0x00;
      }
      else
      {
        trackdata[offset] = _incomingByte;
        offset++;
        trackdata[offset] = 1; // The next character goes here, right now its gibberish but it can't be 0 or we stop reading characters
      }
    }
  }

  if (track == 1)
  {
    offset = 121 * cardslot; // Track 1
    eeprom_write_string(offset, trackdata);
  }
  else if (track == 2)
  {
    offset = (121 * cardslot) + 80; //Track 2
    eeprom_write_string(offset, trackdata);
  }
  Serial1.println(F("Done editing card"));
  loadSettings();
  displayCurrentCards();
}

void getCardFromEEPROM(char* buffer, byte cardslot, byte track)
{
  int offset;
  if (track == 1)
  {
    offset = 121 * cardslot; // Track 1
    eeprom_read_string(offset, buffer, 80);
  }
  else if (track == 2)
  {
    offset = (121 * cardslot) + 80; //Track 2
    eeprom_read_string(offset, buffer, 41);
  }
  else
  {
    Serial1.println (F("Bad track number sent to getCardFromEEPROM"));
    return;
  }
}

void initEEPROMPrompt()
{
  flushSerialBuffer();
  Serial1.println(F("This will Erase ALL stored cards. Are you sure? (y/n)"));
  char choice = 0;
  while (choice == 0 )
  {
    choice = getmenuchoice();
    delay (300);
  }
  switch (choice)
  {
    case 0x00:
      break;
      
    case ASCII_y:
      Serial1.println (F("y was chosen\n"));
      initEEPROM();
      break;
      
    case ASCII_Y:
      Serial1.println (F("Y was chosen\n"));
      initEEPROM();
      break;
      
    default:
        Serial1.println (F("Cancelled, returning to main menu\n"));
        displaymainmenu();
      break;
  } 
}

//
// Dump eeprom memory contents over serial port.
// For each byte, address and value are written.
//
void eeprom_serial_dump_column() {
  // counter
  int i;

  // byte read from eeprom
  byte b;

  // buffer used by sprintf
  char buf[10];

  for (i = EEPROM_MIN_ADDR; i <= EEPROM_MAX_ADDR; i++) {
    b = EEPROM.read(i);
    sprintf(buf, "%03X: %02X", i, b);
    Serial1.println(buf);
  }
}

void initEEPROM()
{
  Serial1.println(F("Initializing EEPROM..."));

  // EEPROM Structure is as follows:
  // Bytes 0-120: Basic/Reserved
  //   Byte 0-3: "1701"
  //   Byte 4: Card number of active card
  //   Byte 5: LED Enable (1=on, 0=off)
  //   Byte 6: TrackToPlay (0=both, 1=1, 2=2)
  // Bytes 120-199: 80 bytes for Card 1 Track 1 (79 characters plus one null termination byte)
  // Bytes 200-239: 41 bytes for Card 1 Track 2 (40 characters plus one null termination byte)
  // Bytes 241-321: 80 byttes for Card 2 Track 1
  // And so on..
  // Offset for Card[x] Track[y] is ((121 * x) + (80 * (y-1)))

  // Set magic bytes
  EEPROM.write(0, 1);
  EEPROM.write(1, 7);
  EEPROM.write(2, 0);
  EEPROM.write(3, 1);

  
  // Set card 1 as active
  EEPROM.write(4, 1);

  // Enable LED
  EEPROM.write(5, 1);

  // Play only track 2
  EEPROM.write(6, 2);

  for (int cardslot = 1; cardslot < 6; cardslot++)
  {
    int offset = 121 * cardslot; // Track 1
    eeprom_write_string (offset, "%B4111111111111111^CARD/TEST^2105521000000000000000541000000?"); // Replace this with a default/test card
    offset = (121 * cardslot) + 80; //Track 2
    eeprom_write_string (offset, ";4111111111111111=21055210000054100000?"); // Replace this with a default/test card, max 40 chars
  }
  Serial1.println(F("Done Initializing EEPROM..."));
  displaymainmenu();
}

//
// Returns true if the address is between the
// minimum and maximum allowed values,
// false otherwise.
//
// This function is used by the other, higher-level functions
// to prevent bugs and runtime errors due to invalid addresses.
//
boolean eeprom_is_addr_ok(int addr) {
  return ((addr >= EEPROM_MIN_ADDR) && (addr <= EEPROM_MAX_ADDR));
}
//
// Writes a sequence of bytes to eeprom starting at the specified address.
// Returns true if the whole array is successfully written.
// Returns false if the start or end addresses aren't between
// the minimum and maximum allowed values.
// When returning false, nothing gets written to eeprom.
//
boolean eeprom_write_bytes(int startAddr, const byte* array, int numBytes) {
  // counter
  int i;

  // both first byte and last byte addresses must fall within
  // the allowed range  
  if (!eeprom_is_addr_ok(startAddr) || !eeprom_is_addr_ok(startAddr + numBytes)) {
    return false;
  }

  for (i = 0; i < numBytes; i++) {
    EEPROM.write(startAddr + i, array[i]);
  }

  return true;
}
//
// Reads the specified number of bytes from the specified address into the provided buffer.
// Returns true if all the bytes are successfully read.
// Returns false if the star or end addresses aren't between
// the minimum and maximum allowed values.
// When returning false, the provided array is untouched.
//
// Note: the caller must ensure that array[] has enough space
// to store at most numBytes bytes.
//
boolean eeprom_read_bytes(int startAddr, byte array[], int numBytes) {
  int i;

  // both first byte and last byte addresses must fall within
  // the allowed range  
  if (!eeprom_is_addr_ok(startAddr) || !eeprom_is_addr_ok(startAddr + numBytes)) {
    return false;
  }

  for (i = 0; i < numBytes; i++) {
    array[i] = EEPROM.read(startAddr + i);
  }

  return true;
}
//
// Writes a string starting at the specified address.
// Returns true if the whole string is successfully written.
// Returns false if the address of one or more bytes
// fall outside the allowed range.
// If false is returned, nothing gets written to the eeprom.
//
boolean eeprom_write_string(int addr, char* string) {
  // actual number of bytes to be written
  int numBytes;

  // we'll need to write the string contents
  // plus the string terminator byte (0x00)
  numBytes = strlen(string) + 1;

  return eeprom_write_bytes(addr, (const byte*)string, numBytes);
}

//
// Reads a string starting from the specified address.
// Returns true if at least one byte (even only the
// string terminator one) is read.
// Returns false if the start address falls outside
// or declare buffer size os zero.
// the allowed range.
// The reading might stop for several reasons:
// - no more space in the provided buffer
// - last eeprom address reached
// - string terminator byte (0x00) encountered.
// The last condition is what should normally occur.
//
boolean eeprom_read_string(int addr, char* buffer, int bufSize) {
  // byte read from eeprom
  byte ch;

  // number of bytes read so far
  int bytesRead;

  // check start address
  if (!eeprom_is_addr_ok(addr)) {
    return false;
  }

  // how can we store bytes in an empty buffer ?
  if (bufSize == 0) {
    return false;
  }

  // is there is room for the string terminator only,
  // no reason to go further
  if (bufSize == 1) {
    buffer[0] = 0;
    return true;
  }

  // initialize byte counter
  bytesRead = 0;

  // read next byte from eeprom
  ch = EEPROM.read(addr + bytesRead);

  // store it into the user buffer
  buffer[bytesRead] = ch;

  // increment byte counter
  bytesRead++;

  // stop conditions:
  // - the character just read is the string terminator one (0x00)
  // - we have filled the user buffer
  // - we have reached the last eeprom address
  while ( (ch != 0x00) && (bytesRead < bufSize) && ((addr + bytesRead) <= EEPROM_MAX_ADDR) ) {
    // if no stop condition is met, read the next byte from eeprom
    ch = EEPROM.read(addr + bytesRead);

    // store it into the user buffer
    buffer[bytesRead] = ch;

    // increment byte counter
    bytesRead++;
  }

  // make sure the user buffer has a string terminator
  // (0x00) as its last byte
  if ((ch != 0x00) && (bytesRead >= 1)) {
    buffer[bytesRead - 1] = 0;
  }

  return true;
}

// echo characters from serial input to Spoofer
// Serial input should come in on Serial (Pro Micro USB) port and be formatted:
// string delay between (milliseconds)<tab>string track (1 or 2)<tab>string data<newline>
// For example:
// 500  1 %B1234567890123445^PADILLA/L.                ^99011200000000000000**XXX******?
void serialFuzz()
{
  Serial.begin(115200);
  byte _incomingByte;

  char inbuffer[81];
  inbuffer[80] = 0x00;
  byte inbufferoffset = 0;
  int delaybetween = -1; //-1 means not set
  byte track = -1; // -1 or 0 means not set
  
  while (keepFuzzing)
  {
    if(Serial.available() > 0 )
    {
      _incomingByte = Serial.read();
      if (delaybetween < 0) // first character of the line
      {
        if (_incomingByte == ASCII_TAB)
        {
          Serial1.println(F("Input record for Serial fuzz did not start with an integer"));
        }
        else if (isDigit(_incomingByte))
        {
          delaybetween = atoi (_incomingByte);
        }
      }
      else if (track < 0) // still reading delaybetween
      {
        if (_incomingByte == ASCII_TAB)
        {
          track = 0;
        }
        else if (isDigit(_incomingByte))
        {
          delaybetween = (10* delaybetween) + atoi (_incomingByte);
        }
      }
      else if (track == 0 )
      {
        if (isDigit(_incomingByte)) // Todo also check if 1 or 2
        {
          track = atoi (_incomingByte);
        }
        else
        {
          Serial1.print(F("Input record for Serial fuzz did not contain a valid track number - "));
          Serial1.println(_incomingByte);
        }
      }
      else if (track > 0)
      {
        if (_incomingByte == ASCII_TAB)
        {
          // do nothing, skip the tab
        }
        else if (inbufferoffset < 81)
        {
          if (_incomingByte == ASCII_NEWLINE)
          {
            inbuffer[inbufferoffset] = 0x00;
            Serial1.print(F("Fuzzing with "));
            Serial1.println(inbuffer);
            playBuffer(track, inbuffer, 0);
            delay(delaybetween);
            delaybetween = -1; //reset for next
            track = -1; // reset for next                      
          }
          else
          {
            inbuffer[inbufferoffset] = _incomingByte;
            inbufferoffset++;
          }
        }
      }
      else // Track length greater than 80
      {
        Serial1.print("Input record for Serial fuzz appears too long - ");
        Serial1.println(inbuffer);
        delaybetween = -1; //reset for next
        track = -1; // reset for next                      
      }
    }
  }
}

// send a single bit out
void playBit(int sendBit)
{
  dir ^= 1;
  digitalWrite(coil1Pin, dir);
  digitalWrite(coil2Pin, !dir);
  delayMicroseconds(CLOCK_US);

  if (sendBit)
  {
    dir ^= 1;
    digitalWrite(coil1Pin, dir);
    digitalWrite(coil2Pin, !dir);
  }
  delayMicroseconds(CLOCK_US);

}

// plays out a the track contained in buffer, encoding based on trackno, calculating CRCs and LRC
// track is either 1 or 2
void playBuffer(int track, char* buffer, byte forwardOrBackward)
{
  int tmp, crc, lrc = 0;

  // Set up Ref Track
  char revTrack[80];
  storeRevTrack(track, buffer, revTrack);
  
  track--; // index 0

  // enable LED and motor driver IC
  digitalWrite(coilEnablePin, HIGH);
  if (ledOnOff == 1)
  {
    digitalWrite(ledPin, HIGH);
  }

  // First put out a bunch of leading zeros.
  for (int i = 0; i < 25; i++)
    playBit(0);

  if (forwardOrBackward == 0)
  {
    for (int i = 0; buffer[i] != '\0'; i++) // for each characer in the buffer until end of string (NULL/0x00)
    {
      crc = 1;
      tmp = buffer[i] - sublen[track];
  
      //ToDo check that tmp contains a character that is valid for the current track type.
  
      for (int j = 0; j < bitlen[track]-1; j++) // for each bit except the CRC bit
      {
        crc ^= tmp & 1;
        lrc ^= (tmp & 1) << j;
        playBit(tmp & 1);
        tmp >>= 1;
      }
      playBit(crc); // last bit is CRC
    } 
  
    // finish calculating and send last "byte" (LRC)
    tmp = lrc;
    crc = 1;
    for (int j = 0; j < bitlen[track]-1; j++)
    {
      crc ^= tmp & 1;
      playBit(tmp & 1);
      tmp >>= 1;
    }
    playBit(crc);
  }
  else
  {
    for (int i = 0; i < BETWEEN_ZERO; i++)
      playBit(0);
      
    playRevTrack((track + 1), revTrack); // track is 0 indexed but should be sent as track 1 = 1
  }
  
  // finish with 0's
  for (int i = 0; i < 5 * 5; i++)
    playBit(0);

  digitalWrite(coil1Pin, LOW);
  digitalWrite(coil2Pin, LOW);
  digitalWrite(coilEnablePin, LOW);
  if (ledOnOff == 1)
  {
    digitalWrite(ledPin, LOW);
  }
}

// stores a copy of a reverse of a track
void storeRevTrack(int track, char* inTrackBuffer, char* revTrackBuffer)
{
  int i, tmp, crc, lrc = 0;
  track--; // index 0
  dir = 0;
  
  for (i = 0; inTrackBuffer[i] != '\0'; i++)
  {
    crc = 1;
    tmp = inTrackBuffer[i] - sublen[track];

    for (int j = 0; j < bitlen[track]-1; j++)
    {
      crc ^= tmp & 1;
      lrc ^= (tmp & 1) << j;
      tmp & 1 ?
        (revTrackBuffer[i] |= 1 << j) :
        (revTrackBuffer[i] &= ~(1 << j));
      tmp >>= 1;
    }
    crc ?
      (revTrackBuffer[i] |= 1 << 4) :
      (revTrackBuffer[i] &= ~(1 << 4));
  }

  // finish calculating and send last "byte" (LRC)
  tmp = lrc;
  crc = 1;
  for (int j = 0; j < bitlen[track]-1; j++)
  {
    crc ^= tmp & 1;
    tmp & 1 ?
      (revTrackBuffer[i] |= 1 << j) :
      (revTrackBuffer[i] &= ~(1 << j));
    tmp >>= 1;
  }
  crc ?
    (revTrackBuffer[i] |= 1 << 4) :
    (revTrackBuffer[i] &= ~(1 << 4));

  i++;
  revTrackBuffer[i] = '\0';
}

void playRevTrack(int track, char* revTrackBuffer)
{
  int i = 0;
  track--; // index 0
  dir = 0;

  while (revTrackBuffer[i++] != '\0');
  i--;
  while (i--)
    for (int j = bitlen[track]-1; j >= 0; j--)
      playBit((revTrackBuffer[i] >> j) & 1);
}
