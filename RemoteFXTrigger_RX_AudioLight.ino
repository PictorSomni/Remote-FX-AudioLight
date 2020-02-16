//Ada_remoteFXTrigger_RX_NeoPixel
//Remote Effects Trigger Box Receiver
//by John Park
//for Adafruit Industries
//
// Button box receiver with NeoPixels
//
//
//MIT License
//
//Edited by Charles Dessenius
//for the win !

#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <RH_RF69.h>
#include <Wire.h>
#include <SD.h>
#include <Adafruit_VS1053.h>


// These are the pins used
#define VS1053_RESET   -1     // VS1053 reset pin (not used!)
#define LED 13


/************ Audio Setup **************/
#define VS1053_CS       6     // VS1053 chip select pin (output)
#define VS1053_DCS     10     // VS1053 Data/command select pin (output)
#define CARDCS          5     // Card chip select pin
// DREQ should be an Int pin *if possible* (not possible on 32u4)
#define VS1053_DREQ     9     // VS1053 Data request, ideally an Interrupt pin
uint8_t VOLUME = 9;            // Set volume -> lower numbers == louder volume!
char FILENAME[] = "ghost01.wav";


/********** NeoPixel Setup *************/
#define PIN            12  //this is the default, you can change if you adjust jumpers on the NeoPixel FeatherWing
#define NUMPIXELS      32
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int delayval = 50; // delay
uint8_t BRIGHTNESS = 127; // 0 - 255


/************ Radio Setup ***************/
// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 434.0

#define RFM69_CS      8
#define RFM69_INT     3
#define RFM69_RST     4


// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

bool oldState = HIGH;
int showType = 0;

// Instance of the music player
Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);
//pinMode(8, INPUT_PULLUP);



////////////////////////////////////////
///              SETUP               ///
////////////////////////////////////////
void setup() {
  delay(500);
  Serial.begin(115200);

  randomSeed(analogRead(0));

  /////////////////////////////////////////////////////////
  // while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer
  /////////////////////////////////////////////////////////

  pixels.begin(); // This initializes the NeoPixel library.
  pixels.setBrightness(BRIGHTNESS);

  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, 0, 0, 0); //clear them
  }

  //pixels.setPixelColor(0,0,buf[3],0);
  pixels.show();

  pinMode(LED, OUTPUT);

  ////////////////////////////////////////
  ///               RADIO              ///
  ////////////////////////////////////////
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Feather RFM69 RX/TX Test!");

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);

  if (!rf69.init()) {
    Serial.println("RFM69 radio init failed");
    while (1);
  }

  Serial.println("RFM69 radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(14, true);

  /*
    The encryption key has to be the same as the one in the server
    uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    rf69.setEncryptionKey(key);
  */

  pinMode(LED, OUTPUT);

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");
  delay(500);

  ////////////////////////////////////////
  ///           MUSIC PLAYER           ///
  ////////////////////////////////////////
  if (! musicPlayer.begin()) { // initialise the music player
    Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
    while (1);
  }

  Serial.println(F("VS1053 found"));

  //  musicPlayer.sineTest(0x44, 500);    // Make a tone to indicate VS1053 is working
  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }

  Serial.println("SD OK!");

  // list files
  printDirectory(SD.open("/"), 0);

  // Volume
  musicPlayer.setVolume(VOLUME, VOLUME); // for left, right channels.

  // If DREQ is on an interrupt pin we can dobackground
  // audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int

  // INTRO SOUND
  FILENAME[6] = random(1, 7) + '0';
  musicPlayer.playFullFile(FILENAME);

  // Shut the red LED off
  digitalWrite(LED, LOW);
}


void loop() {

  if (rf69.waitAvailableTimeout(100)) {
    // Should be a message for us now
    uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (! rf69.recv(buf, &len)) {
      Serial.println("Receive failed");
      return;
    }

    digitalWrite(LED, HIGH);
    rf69.printBuffer("Received: ", buf, len);
    buf[len] = 0;

    Serial.print("Got: "); Serial.println((char*)buf);
    Serial.print("RSSI: "); Serial.println(rf69.lastRssi(), DEC);


    // char radiopacket[20] = "Button #";//prep reply message to send


    if (buf[0] == 'A') { //the letter sent from the button
      musicPlayer.startPlayingFile("popopo.wav");
      while (musicPlayer.playingMusic) {
        digitalWrite(LED, LOW);
        startShow(7);
        for (int i = pixels.numPixels(); i >= 0; i--) {
          pixels.setPixelColor(i, 255, 255, 255);
          pixels.show();
          delay(15);
        }
      }
      startShow(0);
    }

    else if (buf[0] == 'a') {
      musicPlayer.startPlayingFile("thunder2.wav");
      while (musicPlayer.playingMusic) {
        digitalWrite(LED, LOW);
        startShow(10);
        delay(random(5000, 20000));
      }
      lightsOff();
    }

    else if (buf[0] == 'B') { //the letter sent from the button
      musicPlayer.startPlayingFile("fail.wav");
      while (musicPlayer.playingMusic) {
        digitalWrite(LED, LOW);
        startShow(4);
        for (int i = 0; i < pixels.numPixels(); i++) {
          pixels.setPixelColor(i, 255, 0, 0);
          pixels.show();
          delay(15);
        }
      }
      startShow(0);
    }

    else if (buf[0] == 'C') { //the letter sent from the button
      musicPlayer.startPlayingFile("victory.wav");
      while (musicPlayer.playingMusic) {
        digitalWrite(LED, LOW);
        startShow(5);
        for (int i = pixels.numPixels(); i >= 0; i--) {
          pixels.setPixelColor(i, 0, 255, 0);
          pixels.show();
          delay(15);
        }
      }
      startShow(0);
    }
    else if (buf[0] == 'D') { //the letter sent from the button
      musicPlayer.startPlayingFile("dadum.wav");
      while (musicPlayer.playingMusic) {
        digitalWrite(LED, LOW);
        startShow(6);
        for (int i = pixels.numPixels(); i >= 0; i--) {
          pixels.setPixelColor(i, 0, 255, 0);
          pixels.show();
          delay(15);
        }
      }
      startShow(0);
    }

    else if (buf[0] == '1') { //the letter sent from the button
      musicPlayer.startPlayingFile("silenthi.wav");
      while (musicPlayer.playingMusic) {
        digitalWrite(LED, LOW);
        startShow(3);
        for (int i = 0; i < pixels.numPixels(); i++) {
          pixels.setPixelColor(i, 0, 0, 255);
          pixels.show();
          delay(15);
        }
      }
      startShow(0);
    }

    else if (buf[0] == '2') { //the letter sent from the button
      musicPlayer.startPlayingFile("re_title.wav");
      while (musicPlayer.playingMusic) {
        digitalWrite(LED, LOW);
        startShow(2);
      }
      startShow(0);
    }

    else if (buf[0] == 'q') { //the letter sent from the button
      musicPlayer.playFullFile("knock.wav");
      // while (musicPlayer.playingMusic) {

      // }
      // radiopacket[8] = 'I';
    }

    else if (buf[0] == 'r') { //the letter sent from the button
      FILENAME[6] = random(1, 7) + '0';
      musicPlayer.playFullFile(FILENAME);
    }

    else if (buf[0] == 'Z') { //the letter sent from the button
      startShow(0);
    }

    // radiopacket[9] = 0;

    // Serial.print("Sending "); Serial.println(radiopacket);
    // rf69.send((uint8_t *)radiopacket, strlen(radiopacket));
    // rf69.waitPacketSent();

    digitalWrite(LED, LOW);
  }
}

/////////////////////////////////////////////////////////
void startShow(int i) {
  switch (i) {
    case 0: colorWipe(pixels.Color(0, 0, 0), 50);    // Black/off
      break;
    case 1: colorWipe(pixels.Color(255, 0, 0), 50);  // Red
      break;
    case 2: colorWipe(pixels.Color(0, 255, 0), 50);  // Green
      break;
    case 3: colorWipe(pixels.Color(0, 0, 255), 50);  // Blue
      break;
    case 4: theaterChase(pixels.Color(127, 0, 0), 50); // red
      break;
    case 5: theaterChase(pixels.Color(0,   127,   0), 50); // green
      break;
    case 6: theaterChase(pixels.Color(  0,   0, 127), 50); // blue
      break;
    case 7: rainbow(20);
      break;
    case 8: rainbowCycle(20);
      break;
    case 9: theaterChaseRainbow(50);
      break;
    case 10: lightning();
      break;
  }
}

// Fill the dots one after the other with a color
/////////////////////////////////////////////////////////
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelColor(i, c);
    pixels.show();
    delay(wait);
  }
}

/////////////////////////////////////////////////////////
void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel((i + j) & 255));
    }
    pixels.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
/////////////////////////////////////////////////////////
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
    }
    pixels.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
/////////////////////////////////////////////////////////
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j = 0; j < 3; j++) { //do 10 cycles of chasing
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < pixels.numPixels(); i = i + 3) {
        pixels.setPixelColor(i + q, c);  //turn every third pixel on
      }
      pixels.show();

      delay(wait);

      for (int i = 0; i < pixels.numPixels(); i = i + 3) {
        pixels.setPixelColor(i + q, 0);      //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
/////////////////////////////////////////////////////////
void theaterChaseRainbow(uint8_t wait) {
  for (int j = 0; j < 256; j++) {   // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < pixels.numPixels(); i = i + 3) {
        pixels.setPixelColor(i + q, Wheel( (i + j) % 255)); //turn every third pixel on
      }
      pixels.show();

      delay(wait);

      for (int i = 0; i < pixels.numPixels(); i = i + 3) {
        pixels.setPixelColor(i + q, 0);      //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
/////////////////////////////////////////////////////////
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

/// File listing helper
/////////////////////////////////////////////////////////
void printDirectory(File dir, int numTabs) {
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      //Serial.println("**nomorefiles**");
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

// Lightning effect
/////////////////////////////////////////////////////////
void lightning() {
  // Do it between 0 and 10 times
  for (int j = 0; j < random(3, 11); j++) {
    uint8_t c = random(0, 255); // Select random intensity for the lightning

    for (uint16_t i = 0; i < pixels.numPixels(); i++) {
      pixels.setPixelColor(i, pixels.Color(c, c, c));    // turn every pixel at the same intensity
    }
    pixels.show();
    delay(random(25, 100));
    lightsOff();
    delay(random(25, 100));
  }
}

void lightsOff() {
  for (uint8_t i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelColor(i, 0, 0, 0);
  }
  pixels.show();
}
