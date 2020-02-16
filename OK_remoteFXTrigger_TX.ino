//Ada_remoteFXTrigger_TX
//Remote Effects Trigger Box Transmitter
//by John Park
//for Adafruit Industries
//
// General purpose button box
// for triggering remote effects
// using packet radio Feather boards
//
//Edited by Charles Dessenius
//For The Win !
//
//MIT License


#include <SPI.h>
#include <RH_RF69.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_Trellis.h"
#include <Encoder.h>

/********* Encoder Setup ***************/
#define PIN_ENCODER_SWITCH 11
Encoder knob(10, 12);
uint8_t activeRow = 0;
long pos = -999;
long newpos;
int prevButtonState = HIGH;
bool needsRefresh = true;
bool advanced = false;
unsigned long startTime;


/********* Trellis Setup ***************/
#define MOMENTARY 0
#define LATCHING 1
#define MODE LATCHING //all Trellis buttons in latching mode
Adafruit_Trellis matrix0 = Adafruit_Trellis();
Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0);
#define NUMTRELLIS 1
#define numKeys (NUMTRELLIS * 16)
#define INTPIN A2

/************ OLED Setup ***************/
Adafruit_SSD1306 oled = Adafruit_SSD1306();
#define BUTTON_A 9
#define BUTTON_B 6
#define BUTTON_C 5
#define LED      13


/************ Radio Setup ***************/
// Can be changed to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 434.0

#define RFM69_CS      8
#define RFM69_INT     3
#define RFM69_RST     4


// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

int lastButton=17; //last button pressed for Trellis logic

int menuList[4]={1,2,3,4}; //for rotary encoder choices
int m = 0; //variable to increment through menu list
int lastTB[8] = {16, 16, 16, 16}; //array to store per-menu Trellis button

/*******************SETUP************/
void setup() {
  delay(500);
  Serial.begin(115200);
  //while (!Serial) { delay(1); } // wait until serial console is open, 
  //remove if not tethered to computer


  // INT pin on Trellis requires a pullup
  pinMode(INTPIN, INPUT);
  digitalWrite(INTPIN, HIGH);
  trellis.begin(0x70);  

  pinMode(PIN_ENCODER_SWITCH, INPUT_PULLUP);//set encoder push switch pin to input pullup
  
  digitalPinToInterrupt(10); //on M0, Encoder library doesn't auto set these as interrupts
  digitalPinToInterrupt(12);
  
  // Initialize OLED display
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  oled.setTextWrap(false);
  oled.display();
  delay(500);
  oled.clearDisplay();
  oled.display();
  oled.setTextSize(2);
  oled.setTextColor(WHITE);
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  pinMode(LED, OUTPUT);     
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

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

  // The encryption key has to be the same as the one in the server
  // uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  //                   0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  // rf69.setEncryptionKey(key);
  
  pinMode(LED, OUTPUT);

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");

  oled.setCursor(0,0);
  oled.println("RFM69 @ ");
  oled.print((int)RF69_FREQ);
  oled.println(" MHz");
  oled.display();
  delay(1200); //pause to let freq message be read by a human

  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.println("REMOTE FX");
  oled.setCursor(0,16);
  oled.println("TRIGGER");  
  oled.display();

  // light up all the LEDs in order
  for (uint8_t i=0; i<numKeys; i++) {
    trellis.setLED(i);
    trellis.writeDisplay();    
    delay(30);
  }
  // then turn them off
  for (uint8_t i=0; i<numKeys; i++) {
    trellis.clrLED(i);
    trellis.writeDisplay();    
    delay(30);
  }

  digitalWrite(LED, LOW);
}
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
void loop() {
  delay(30); // 30ms delay is required, dont remove me! (Trellis)

    /*************Rotary Encoder Menu***********/

    //check the encoder knob, set the current position as origin
    long newpos = knob.read() / 4;//divide for encoder detents
    
    /* // for debugging
    Serial.print("pos=");
    Serial.print(pos);
    Serial.print(", newpos=");
    Serial.println(newpos);
    */

    if(newpos != pos){
      int diff = newpos - pos;//check the different between old and new position
      if(diff>=1){
        m++; 
        m = (m+4) % 4;//modulo to roll over the m variable through the list size
      }

      if(diff==-1){ //rotating backwards
        m--;
        m = (m+4) % 4;
      }
      /* //uncomment for debugging or general curiosity
      Serial.print("Diff = ");
      Serial.print(diff);
      Serial.print("  pos= ");
      Serial.print(pos);
      Serial.print(", newpos=");
      Serial.println(newpos);
      Serial.println(menuList[m]);
      */

      pos = newpos;

      // Serial.print("m is: ");
      //Serial.println(m);


      //clear Trellis lights 
      for(int t=0;t<=16;t++){
        trellis.clrLED(t);
        trellis.writeDisplay();
      }
      //light last saved light for current menu
        trellis.clrLED(lastTB[m]);
        trellis.setLED(lastTB[m]);
        trellis.writeDisplay();
      
      //write to the display
      oled.setCursor(0,3);
      oled.clearDisplay();

      int p; //for drawing bullet point menu location pixels
      int q;

      if (m==0){
        for(p=0;p<8;p++){
          for(q=0;q<8;q++){
            oled.drawPixel(q,p,WHITE);
          }
        }
        oled.print(" AMBIANCE");
      }
      if (m==1){
        for(p=8;p<16;p++){
          for(q=0;q<8;q++){
            oled.drawPixel(q,p,WHITE);
          }
        }
        oled.print(" MJ");
      }
      if (m==2){
        for(p=16;p<24;p++){
          for(q=0;q<8;q++){
            oled.drawPixel(q,p,WHITE);
          }
        }
        oled.print(" INTERACTIF");
      }
      if (m==3){
        for(p=24;p<32;p++){
          for(q=0;q<8;q++){
            oled.drawPixel(q,p,WHITE);
          }
        }
        oled.print(" EFFETS");
      }      
      oled.display();
    }

// remember that the switch is active low
    int buttonState = digitalRead(PIN_ENCODER_SWITCH);
    if (buttonState == LOW) {
        unsigned long now = millis();
        if (prevButtonState == HIGH) {
            prevButtonState = buttonState;
            startTime = now;
           // Serial.println("button pressed");
            trellis.clrLED(lastTB[m]);
            trellis.writeDisplay();    
            lastTB[m]=17;//set this above the physical range so 
            //next button press works properly
        } 
    } 
    else if (buttonState == HIGH && prevButtonState == LOW) {
      //Serial.println("button released!");
      prevButtonState = buttonState;
    }

  /*************Trellis Button Presses***********/
  if (MODE == MOMENTARY) {
    if (trellis.readSwitches()) { // If a button was just pressed or released...
      for (uint8_t i=0; i<numKeys; i++) { // go through every button
        if (trellis.justPressed(i)) { // if it was pressed, turn it on
        //Serial.print("v"); Serial.println(i);
          trellis.setLED(i);
        } 
        if (trellis.justReleased(i)) { // if it was released, turn it off
          //Serial.print("^"); Serial.println(i);
          trellis.clrLED(i);
        }
      }
      trellis.writeDisplay(); // tell the trellis to set the LEDs we requested
    }
    char  radiopacket[20];
  }

  if (MODE == LATCHING) {
    if (trellis.readSwitches()) { // If a button was just pressed or released...
      for (uint8_t i=0; i<numKeys; i++) { // go through every button
        if (trellis.justPressed(i)) { // if it was pressed...
          //Serial.print("v"); Serial.println(i);

          // Alternate the LED unless the same button is pressed again
          //if(i!=lastButton){
          if(i!=lastTB[m]){ 
            if (trellis.isLED(i)){
              trellis.clrLED(i);
              lastTB[m]=i; //set the stored value for menu changes
            }
            else{
              trellis.setLED(i);
              //trellis.clrLED(lastButton);//turn off last one
              trellis.clrLED(lastTB[m]);
              lastTB[m]=i; //set the stored value for menu changes
            }
            trellis.writeDisplay();
          }
          char  radiopacket[20];
            
        /************** AMBIANCE **************/
        //check the rotary encoder menu choice
        if(m==0){//first menu item
          if (i==0){ //button 0 sends button A command
            radiopacket[0] = 'a';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("AMBIANCE");
            oled.setCursor(0,16);
            oled.print("01");
            oled.display();  
          }
          if (i==1){ //button 1 sends button B command
            radiopacket[0] = 'b';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("AMBIANCE");
            oled.setCursor(0,16);
            oled.print("02");
            oled.display(); 
          }
          if (i==2){ //
            radiopacket[0] = 'c';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("AMBIANCE");
            oled.setCursor(0,16);
            oled.print("03");
            oled.display(); 
          } 
          if (i==3){ //
            radiopacket[0] = 'd';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("AMBIANCE");
            oled.setCursor(0,16);
            oled.print("04");
            oled.display(); 
          }
          if (i==4){ //
            radiopacket[0] = 'e';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("AMBIANCE");
            oled.setCursor(0,16);
            oled.print("05");
            oled.display(); 
          }
          if (i==5){ //
            radiopacket[0] = 'f';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("AMBIANCE");
            oled.setCursor(0,16);
            oled.print("06");
            oled.display(); 
          }
          if (i==6){ //
            radiopacket[0] = 'g';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("AMBIANCE");
            oled.setCursor(0,16);
            oled.print("07");
            oled.display(); 
          }
          if (i==7){ //
            radiopacket[0] = 'h';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("AMBIANCE");
            oled.setCursor(0,16);
            oled.print("08");
            oled.display(); 
          }
          if (i==8){ //
            radiopacket[0] = 'i';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("AMBIANCE");
            oled.setCursor(0,16);
            oled.print("09");
            oled.display(); 
          }
          if (i==9){ //
            radiopacket[0] = 'j';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("AMBIANCE");
            oled.setCursor(0,16);
            oled.print("10");
            oled.display(); 
          }
          if (i==10){ //
            radiopacket[0] = 'k';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("AMBIANCE");
            oled.setCursor(0,16);
            oled.print("11");
            oled.display(); 
          }
          if (i==11){ //
            radiopacket[0] = 'l';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("AMBIANCE");
            oled.setCursor(0,16);
            oled.print("12");
            oled.display(); 
          }
          if (i==12){ //
            radiopacket[0] = 'm';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("AMBIANCE");
            oled.setCursor(0,16);
            oled.print("13");
            oled.display(); 
          }
          if (i==13){ //
            radiopacket[0] = 'n';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("AMBIANCE");
            oled.setCursor(13,16);
            oled.print("14");
            oled.display(); 
          }
          if (i==14){ //
            radiopacket[0] = 'o';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("AMBIANCE");
            oled.setCursor(0,16);
            oled.print("15");
            oled.display(); 
          }
          if (i==15){ //
            radiopacket[0] = 'p';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("AMBIANCE");
            oled.setCursor(0,16);
            oled.print("16");
            oled.display(); 
          }
        }
        /************** MJ **************/
        if(m==1){//next menu item
          if (i==0){ //button 0 sends button A command
            radiopacket[0] = 'A';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("MJ");
            oled.setCursor(0,16);
            oled.print("01");
            oled.display();  
          }
          if (i==1){ //button 1 sends button B command
            radiopacket[0] = 'B';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("MJ");
            oled.setCursor(0,16);
            oled.print("02");
            oled.display(); 
          }
          if (i==2){ //
            radiopacket[0] = 'C';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("MJ");
            oled.setCursor(0,16);
            oled.print("03");
            oled.display(); 
          } 
          if (i==3){ //
            radiopacket[0] = 'D';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("MJ");
            oled.setCursor(0,16);
            oled.print("04");
            oled.display(); 
          }
          if (i==4){ //
            radiopacket[0] = 'E';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("MJ");
            oled.setCursor(0,16);
            oled.print("05");
            oled.display(); 
          }
          if (i==5){ //
            radiopacket[0] = 'F';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("MJ");
            oled.setCursor(0,16);
            oled.print("06");
            oled.display(); 
          }
          if (i==6){ //
            radiopacket[0] = 'G';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("MJ");
            oled.setCursor(0,16);
            oled.print("07");
            oled.display(); 
          }
          if (i==7){ //
            radiopacket[0] = 'H';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("MJ");
            oled.setCursor(0,16);
            oled.print("08");
            oled.display(); 
          }
          if (i==8){ //
            radiopacket[0] = 'I';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("MJ");
            oled.setCursor(0,16);
            oled.print("09");
            oled.display(); 
          }
          if (i==9){ //
            radiopacket[0] = 'J';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("MJ");
            oled.setCursor(0,16);
            oled.print("10");
            oled.display(); 
          }
          if (i==10){ //
            radiopacket[0] = 'K';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("MJ");
            oled.setCursor(0,16);
            oled.print("11");
            oled.display(); 
          }
          if (i==11){ //
            radiopacket[0] = 'L';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("MJ");
            oled.setCursor(0,16);
            oled.print("12");
            oled.display(); 
          }
          if (i==12){ //
            radiopacket[0] = 'M';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("MJ");
            oled.setCursor(0,16);
            oled.print("13");
            oled.display(); 
          }
          if (i==13){ //
            radiopacket[0] = 'N';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("MJ");
            oled.setCursor(0,16);
            oled.print("14");
            oled.display(); 
          }
          if (i==14){ //
            radiopacket[0] = 'O';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("MJ");
            oled.setCursor(0,16);
            oled.print("15");
            oled.display(); 
          }
          if (i==15){ //
            radiopacket[0] = 'P';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("MJ");
            oled.setCursor(0,16);
            oled.print("16");
            oled.display(); 
          }
        }
        /************** INTERACTIF **************/
        if(m==2){//next menu item
          if (i==0){ //button 0 sends button A command
            radiopacket[0] = 'q';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("INTERACTIF");
            oled.setCursor(0,16);
            oled.print("01");
            oled.display();  
          }
          if (i==1){ //button 1 sends button B command
            radiopacket[0] = 'r';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("INTERACTIF");
            oled.setCursor(0,16);
            oled.print("02");
            oled.display(); 
          }
          if (i==2){ //
            radiopacket[0] = 's';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("INTERACTIF");
            oled.setCursor(0,16);
            oled.print("03");
            oled.display(); 
          } 
          if (i==3){ //
            radiopacket[0] = 't';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("INTERACTIF");
            oled.setCursor(0,16);
            oled.print("04");
            oled.display(); 
          }
          if (i==4){ //
            radiopacket[0] = 'w';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("INTERACTIF");
            oled.setCursor(0,16);
            oled.print("05");
            oled.display(); 
          }
          if (i==5){ //
            radiopacket[0] = 'x';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("INTERACTIF");
            oled.setCursor(0,16);
            oled.print("06");
            oled.display(); 
          }
          if (i==6){ //
            radiopacket[0] = 'y';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("INTERACTIF");
            oled.setCursor(0,16);
            oled.print("07");
            oled.display(); 
          }
          if (i==7){ //
            radiopacket[0] = 'z';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("INTERACTIF");
            oled.setCursor(0,16);
            oled.print("08");
            oled.display(); 
          }
          if (i==8){ //
            radiopacket[0] = 'Q';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("INTERACTIF");
            oled.setCursor(0,16);
            oled.print("09");
            oled.display(); 
          }
          if (i==9){ //
            radiopacket[0] = 'R';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("INTERACTIF");
            oled.setCursor(0,16);
            oled.print("10");
            oled.display(); 
          }
          if (i==10){ //
            radiopacket[0] = 'S';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("INTERACTIF");
            oled.setCursor(0,16);
            oled.print("11");
            oled.display(); 
          }
          if (i==11){ //
            radiopacket[0] = 'T';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("INTERACTIF");
            oled.setCursor(0,16);
            oled.print("12");
            oled.display(); 
          }
          if (i==12){ //
            radiopacket[0] = 'W';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("INTERACTIF");
            oled.setCursor(0,16);
            oled.print("13");
            oled.display(); 
          }
          if (i==13){ //
            radiopacket[0] = 'X';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("INTERACTIF");
            oled.setCursor(0,16);
            oled.print("14");
            oled.display(); 
          }
          if (i==14){ //
            radiopacket[0] = 'Y';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("INTERACTIF");
            oled.setCursor(0,16);
            oled.print("15");
            oled.display(); 
          }
          if (i==15){ //
            radiopacket[0] = 'Z';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("INTERACTIF");
            oled.setCursor(0,16);
            oled.print("16");
            oled.display(); 
          }   
        }
        /************** EFFETS **************/
        if(m==3){//next menu item
          if (i==0){ //button 0 sends button A command
            radiopacket[0] = '1';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("EFFET");
            oled.setCursor(0,16);
            oled.print("01");
            oled.display();  
          }
          if (i==1){ //button 1 sends button B command
            radiopacket[0] = '2';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("EFFET");
            oled.setCursor(0,16);
            oled.print("02");
            oled.display(); 
          }
          if (i==2){ //
            radiopacket[0] = '3';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("EFFET");
            oled.setCursor(0,16);
            oled.print("03");
            oled.display(); 
          } 
          if (i==3){ //
            radiopacket[0] = '4';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("EFFET");
            oled.setCursor(0,16);
            oled.print("04");
            oled.display(); 
          }
          if (i==4){ //
            radiopacket[0] = '5';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("EFFET");
            oled.setCursor(0,16);
            oled.print("05");
            oled.display(); 
          }
          if (i==5){ //
            radiopacket[0] = '6';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("EFFET");
            oled.setCursor(0,16);
            oled.print("06");
            oled.display(); 
          }
          if (i==6){ //
            radiopacket[0] = '7';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("EFFET");
            oled.setCursor(0,16);
            oled.print("07");
            oled.display(); 
          }
          if (i==7){ //
            radiopacket[0] = '8';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("EFFET");
            oled.setCursor(0,16);
            oled.print("08");
            oled.display(); 
          }
          if (i==8){ //
            radiopacket[0] = '9';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("EFFET");
            oled.setCursor(0,16);
            oled.print("09");
            oled.display(); 
          }
          if (i==9){ //
            radiopacket[0] = '10';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("EFFET");
            oled.setCursor(0,16);
            oled.print("10");
            oled.display(); 
          }
          if (i==10){ //
            radiopacket[0] = '11';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("EFFET");
            oled.setCursor(0,16);
            oled.print("11");
            oled.display(); 
          }
          if (i==11){ //
            radiopacket[0] = '12';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("EFFET");
            oled.setCursor(0,16);
            oled.print("12");
            oled.display(); 
          }
          if (i==12){ //
            radiopacket[0] = '13';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("EFFET");
            oled.setCursor(0,16);
            oled.print("13");
            oled.display(); 
          }
          if (i==13){ //
            radiopacket[0] = '14';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("EFFET");
            oled.setCursor(0,16);
            oled.print("14");
            oled.display(); 
          }
          if (i==14){ //
            radiopacket[0] = '15';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("EFFET");
            oled.setCursor(0,16);
            oled.print("15");
            oled.display(); 
          }
          if (i==15){ //
            radiopacket[0] = '16';
            oled.clearDisplay();
            oled.setCursor(0,0);
            oled.print("EFFET");
            oled.setCursor(0,16);
            oled.print("16");
            oled.display(); 
          }
          
//          if(i>=0 && i<=15){
//            oled.clearDisplay();
//            oled.setCursor(0,0);
//            oled.print("EFFECTS");
//            oled.setCursor(0,16);
//            oled.print("OH YEAH");
//            oled.display();  
//          }
        }
            
          Serial.print("Sending "); 
          Serial.println(radiopacket[0]);

          rf69.send((uint8_t *)radiopacket, strlen(radiopacket));
          rf69.waitPacketSent(); 
          //reset packet so unassigned buttons don't send last command
          radiopacket[0]='Z'; //also being used to turn off NeoPixels 
          //from any unused button

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
            
            //Serial.print("TX Got: "); 
            //Serial.println((char*)buf);
            Serial.print("RSSI: "); 
            Serial.println(rf69.lastRssi(), DEC);

            //delay(1000);//chill for a moment before returning the message to RX unit

            /*************Reply message from RX unit***********/
            oled.clearDisplay();
            oled.print((char*)buf[0]);
            oled.print("RSSI: "); oled.print(rf69.lastRssi());
            oled.display(); 
            
            
            digitalWrite(LED, LOW);
          }

          //lastButton=i;//set for next pass through to turn this one off
        } 
      }
      // tell the trellis to set the LEDs we requested
      trellis.writeDisplay();
    }
  }
}
