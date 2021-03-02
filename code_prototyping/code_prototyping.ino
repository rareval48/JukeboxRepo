// Paul Schakel
// This is my prototyping sandbox for the Jukebox Project

#include <LiquidCrystal_I2C.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

// These are the pins used for the music maker shield
#define SHIELD_RESET  -1      // VS1053 reset pin (unused!)
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)

// These are common pins between breakout and shield
#define CARDCS 4     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

LiquidCrystal_I2C lcd(0x3F, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

ClickEncoder *encoder;
int16_t last;

void timerIsr() {
  encoder->service();
}

int screenWidth = 16;
int screenHeight = 2;
String textLine1 = " ";
String textLine2 = " ";
int stringStart = 0;
int stringStop = screenWidth;
int scrollCursor = 0;
int counter = 0;
unsigned long clock;
unsigned long clockPrev;

void setup() {
  lcd.init();                      // initialize the lcd
  lcd.backlight();
  Serial.begin(9600);
  encoder = new ClickEncoder(A1, A0, A2);

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);
  last = -1;

  if (! musicPlayer.begin()) { // initialise the music player
    Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
    while (1);
  }
  Serial.println(F("VS1053 found"));

  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }

  musicPlayer.setVolume(20, 20);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT);
}

void loop() {
  clockPrev = clock;
  clock = millis();
  //Serial.println(clock);
  counter += encoder->getValue();

  if (counter != last) {
    last = counter;
    Serial.print("Encoder Value: ");
    Serial.println(counter);
  }

  if (clock % 500 <= 100 && clockPrev % 500 > 100) {
    displayText();
    Serial.println("scrolled text");
  }

  if (musicPlayer.stopped()) {
    Serial.println("music player isnt playing");
  }

  if (counter / 4 == 1) {
    newTextDisplay("Money for Nothing", "Dire Straits");
  } else if (counter / 4 == 2) {
    newTextDisplay("Love Over Gold", "Dire Straits");
  } else if (counter / 4 == 3) {
    newTextDisplay("Your Latest Trick", "Dire Straits");
  } else if (counter / 4 == 4) {
    newTextDisplay("A Journey Begins", "Soulside Eclipse");
  } else if (counter / 4 == 5) {
    newTextDisplay("Blue In Green", "Miles Davis");
  }

  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    switch (b) {
      case ClickEncoder::Clicked:
        Serial.println("Button Clicked");
        if (counter / 4 == 1) {
          musicPlayer.stopPlaying();
          musicPlayer.startPlayingFile("/track005.mp3");
        } else if (counter / 4 == 2) {
          musicPlayer.stopPlaying();
          musicPlayer.startPlayingFile("/track002.mp3");
        } else if (counter / 4 == 3) {
          musicPlayer.stopPlaying();
          musicPlayer.startPlayingFile("/track003.mp3");
        } else if (counter / 4 == 4) {
          musicPlayer.stopPlaying();
          musicPlayer.startPlayingFile("/track001.mp3");
        } else if (counter / 4 == 5) {
          musicPlayer.stopPlaying();
          musicPlayer.startPlayingFile("/track004.mp3");
        }
        break;
      case ClickEncoder::Held:
        Serial.println("Button Held");
        lcd.noBacklight();
        delay(500);
        lcd.backlight();
        break;
    }
  }
}


void newTextDisplay(String line1, String line2) {
  //reset scrolling variables
  if (line1 != textLine1 || line2 != textLine2) {
    stringStart = 0;
    stringStop = screenWidth;
    scrollCursor = 0;

    //set text variables
    textLine1 = line1;
    textLine2 = line2;
  }
}

void displayText() {        // This function is responsible for displaying menu text to the screen.
  int lineToScroll;         // It can display up to 2 lines of text, and will scroll each line individually if its length exeeds 16 characters
  String toScroll;
  int length1 = textLine1.length();
  int length2 = textLine2.length();

  lcd.clear();  //clears previous displayed text

  if (length1 > 16 && length2 <= 16) {   //only scrolls line 1 if true
    toScroll = textLine1;
    lineToScroll = 0;
    lcd.setCursor(0, 1);
    lcd.print(textLine2);
  } else if (length1 <= 16 && length2 > 16) {   //only scrolls line 2 if true
    toScroll = textLine2;
    lineToScroll = 1;
    lcd.setCursor(0, 0);
    lcd.print(textLine1);
  } else if (length1 > 16 && length2 > 16) {   //scrolls both lines if true
    lcd.setCursor(scrollCursor, 0);
    lcd.print(textLine1.substring(stringStart, stringStop));
    lcd.setCursor(scrollCursor, 1);
    lcd.print(textLine2.substring(stringStart, stringStop));
    if (length1 > length2) {
      toScroll = textLine1;
      lineToScroll = 0;
    } else {
      toScroll = textLine2;
      lineToScroll = 1;
    }
  } else {   //doesnt scroll either line
    lcd.setCursor(0, 0);
    lcd.print(textLine1);
    lcd.setCursor(0, 1);
    lcd.print(textLine2);
  }

  // general code which scrolls the specified line when only one line is too long
  lcd.setCursor(scrollCursor, lineToScroll);
  lcd.print(toScroll.substring(stringStart, stringStop));

  if (stringStart > stringStop) {   //resets variables, its basically an error checking measure
    stringStart = 0;
    stringStop = 0;
    scrollCursor = 0;
  } if (stringStart == 0 && scrollCursor > 0) {  //shows an additional character when scrolling in from the side
    scrollCursor--;
    stringStop++;
  } else if (stringStart == stringStop) {   //restarts the cycle
    stringStart = stringStop = 0;
    scrollCursor = screenWidth;
  } else if (stringStop == toScroll.length() && scrollCursor == 0) {    //makes the string go off the screen to the right
    stringStart++;
  } else {  //happens if screen is full and keeps text scrolling
    stringStart++;
    stringStop++;
  }
}
