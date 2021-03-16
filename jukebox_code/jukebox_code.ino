/*

Written by Paul Schakel, March 2021

This is the code for Paul and Rafael's Jukebox project. It implements a full-featured interface for choosing and playing a selection of songs.

More information about the interface and how it works is available here: https://github.com/rareval48/JukeboxRepo#code---paul

*/

#include <SPI.h>
#include <ClickEncoder.h>          //https://github.com/0xPIT/encoder
#include <TimerOne.h>
#include <Adafruit_VS1053.h>       //https://github.com/adafruit/Adafruit_VS1053_Library
#include <LiquidCrystal_I2C.h>     //https://github.com/johnrickman/LiquidCrystal_I2C
#include <MemoryFree.h>            //https://github.com/mpflaga/Arduino-MemoryFree

// These are the pins used for the music maker shield
#define SHIELD_RESET  -1      // VS1053 reset pin
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)
#define CARDCS 4     // Card chip select pin
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

#define BACK_BUTTON_PIN 9

// These are the different menu states which are possible
#define MAIN_MENU 0
#define QUEUE_MENU 1
#define GENRE_MENU 2
#define SONG_MENU 3

//set maximums
#define MAX_QUEUE_LENGTH 12
#define MAX_STRING_LENGTH 36
#define SHORTER_STRING_LENGTH 15

//define structure datatypes
struct Song {
  char title[MAX_STRING_LENGTH];
  char filename[SHORTER_STRING_LENGTH - 1];
  char artist[SHORTER_STRING_LENGTH + 3];
};


Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

LiquidCrystal_I2C lcd(0x3F, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

ClickEncoder *encoder;
int16_t last;

void timerIsr() {   //used by the ClickEncoder
  encoder->service();
}

int backButtonState;
int backButtonStatePrev;

//used for text scrolling in displayText()
uint8_t screenWidth = 16;
uint8_t screenHeight = 2;
uint8_t stringStart = 0;
uint8_t stringStop = screenWidth;
uint8_t scrollCursor = 0;

//these two variables set what is displayed on the lcd screen, they are set by setDisplayText
String textLine1 = "";
String textLine2 = "";

//keep track of time
unsigned long clock;
unsigned long clockPrev;  //needed to check if a specific moment in time has passed

int encoderValue = 0;
int encoderValuePrev = 0;  //used to check if encoderValue went up or down
int encoderValueChange = 0;
bool encoderChangeDetected = false;
long timeChangeDetected = 0;

int menuState = MAIN_MENU;


//create genreLists
const Song rockList[] PROGMEM =
{
  {"Brothers In Arms", "/track014.mp3", "Dire Straits"},
  {"Money For Nothing", "/track001.mp3", "Dire Straits"},
  {"Sultans Of Swing", "/track015.mp3", "Dire Straits"},
  {"DREAM", "/track021.mp3", "Ocean Grove"},
  {"No Line On The Horizon", "/track008.mp3", "U2"},
  {"Where The Streets Have No Name", "/track005.mp3", "U2"},
  {"With Or Without You", "/track031.mp3", "U2"},
};

const Song metalList[] PROGMEM =
{
  {"Live Or Die", "/track009.mp3", "Apocalyptica"},
  {"Hail To The King", "/track033.mp3", "Avenged Sevenfold"},
  {"Pull Me Under", "/track004.mp3", "Dream Theater"},
  {"Hallowed Be Thy Name", "/track017.mp3", "Iron Maiden"},
  {"Surfing with the Alien", "/track025.mp3", "Joe Satriani"},
  {"Peace Sells", "/track022.mp3", "Megadeth"},
  {"Symphony Of Destruction", "/track026.mp3", "Megadeth"},
  {"Washington Is Next!", "/track028.mp3", "Megadeth"},
  {"Fade To Black", "/track003.mp3", "Metallica"},
  {"Mr. Crowley", "/track007.mp3", "Ozzy Osbourne"},
  {"Hearts Of Iron", "/track018.mp3", "Sabaton"},
  {"Toxicity", "/track027.mp3", "System Of A Down"},
};

const Song jazzList[] PROGMEM =
{
  {"Pittsburgh", "/track023.mp3", "Ahmad Jamal"},
  {"Take Five", "/track012.mp3", "Dave Brubeck"},
  {"In A Sentimental Mood", "/track016.mp3", "Duke Ellington"},
  {"Blue In Green", "/track002.mp3", "Miles Davis"},
  {"So What", "/track020.mp3", "Miles Davis"},
  {"'Round Midnight", "/track024.mp3", "Thelonious Monk"},
};

const Song popList[] PROGMEM =
{
  {"Baby Shark", "/track038.mp3", "Pinkfong"},  //this is a rickroll
};

const Song classicalList[] PROGMEM =
{
  {"Hungarian Dance No. 5", "/track010.mp3", "Brahms"},
  {"Claire de Lune", "/track011.mp3", "Debussy"},
  {"Soviet National Anthem", "/track037.mp3", "Red Army Choir"},
  {"Danse Macabre", "/track006.mp3", "Saint-Saens"},
  {"Blue Danube Waltz", "/track039.mp3", "Strauss"},
  {"1812 Overture", "/track032.mp3", "Tchaikovsky"},
};

const Song rapList[] PROGMEM =
{

};

const Song electronicList[] PROGMEM =
{
  {"Harder, Better, Faster, Stronger", "/track034.mp3", "Daft Punk"},
  {"SATRN", "/track013.mp3", "deadmau5"},
  {"Strobe", "/track035.mp3", "deadmau5"},
  {"Resonance", "/track029.mp3", "Home"},
  {"Pacific Coast Highway", "/track036.mp3", "Kavinsky"},
  {"66 Mhz", "/track030.mp3", "Waveshaper"},
  {"Spectrum", "/track040.mp3", "Zedd"},
};

//create mainMenu
const char mainMenuItem0[] PROGMEM = "Go to Genre Menu";
const char mainMenuItem1[] PROGMEM = "Go to Queue";
const char *const mainMenuItems[] PROGMEM = {mainMenuItem0, mainMenuItem1};
char mainMenuCurrentItem[MAX_STRING_LENGTH];
uint8_t mainMenuCurrentIndex = 0;

//create queue
Song *queue[MAX_QUEUE_LENGTH] = {};
Song queueMenuCurrentItem;
uint8_t queueMenuCurrentIndex = 0;
uint8_t queueMenuCurrentLength = 0;
bool queueEmptyAlert = false;
bool wasPlaying = false;

//create genreMenu
const char genre0[] PROGMEM = "Heavy Metal";
const char genre1[] PROGMEM = "Rock";
const char genre2[] PROGMEM = "Pop";
const char genre3[] PROGMEM = "Jazz";
const char genre4[] PROGMEM = "Classical";
const char genre5[] PROGMEM = "Hip Hop/Rap";
const char genre6[] PROGMEM = "Electronic";
const char *const genreMenuItems[] PROGMEM = {genre0, genre1, genre2, genre3, genre4, genre5, genre6};
char genreMenuCurrentItem[MAX_STRING_LENGTH];
uint8_t genreMenuCurrentIndex = 0;

//create songMenu
Song *songMenuPtr;
Song songMenuCurrentItem;
uint8_t songMenuCurrentIndex = 0;
uint8_t songMenuListLength = 0;

//SRAM String
char stringSRAM[MAX_STRING_LENGTH];

void setup() {
  Serial.begin(9600);

  pinMode(BACK_BUTTON_PIN, INPUT);  //set up back button

  //set currentItems
  strcpy_P(genreMenuCurrentItem, (char *)pgm_read_word(&(genreMenuItems[0])));
  strcpy_P(mainMenuCurrentItem, (char *)pgm_read_word(&(mainMenuItems[0])));


  lcd.init();   // initialise the lcd
  lcd.backlight();

  //reserve space for manipulating these strings -- reduces heap fragmentation
  textLine1.reserve(MAX_STRING_LENGTH + 5);
  textLine2.reserve(MAX_STRING_LENGTH);

  encoder = new ClickEncoder(A1, A0, A2);
  encoder->setAccelerationEnabled(false);

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);
  last = -1;

  if (! musicPlayer.begin()) { // initialise the music player
    Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
    while (1);
  }
  Serial.println(F("VS1053 found"));

  if (!SD.begin(CARDCS)) {  //initialise SD card
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }

  musicPlayer.setVolume(30, 30);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT);   //lets it play in the background

  Serial.println(freeMemory());
}


void loop() {
  //Serial.println(freeMemory());

  //keep track of timings for LCD scrolling and reading rotary encoder values
  clockPrev = clock;
  clock = millis();

  //get state of back button
  backButtonStatePrev = backButtonState;
  backButtonState = digitalRead(BACK_BUTTON_PIN);

  //manage rotary encoder
  encoderValue += encoder->getValue();
  ClickEncoder::Button encoderButton = encoder->getButton();

  if (encoderValue != encoderValuePrev && !encoderChangeDetected) { //checks if the encoder value has changed, but only if it hasn't detected any change recently
    encoderChangeDetected = true;
    Serial.println(F("change detected"));
    timeChangeDetected = clock;
  }

  if (encoderChangeDetected && clock - timeChangeDetected >= 200 && clockPrev - timeChangeDetected < 200) {   //adds delay for the rotary encoder to stop turning, then checks which direction it turned
    if (encoderValuePrev < encoderValue) {
      encoderValueChange = 1;
    } else if (encoderValuePrev > encoderValue) {
      encoderValueChange = -1;
    }
    Serial.print(F("EncoderValueChange: "));
    Serial.println(encoderValueChange);
    encoderValuePrev = encoderValue;
    encoderChangeDetected = false;
  }

  if (encoderValue != last) {
    last = encoderValue;
    Serial.print(F("Encoder Value: "));
    Serial.println(encoderValue);
  }

  if (clock % 500 <= 100 && clockPrev % 500 > 100) {  //scrolls the text twice a second
    displayText();
    //Serial.println(F("scrolled text"));
  }

  //handle playing of songs
  if (musicPlayer.stopped()) {
    if (queueEmptyAlert) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Queue Empty"));
      lcd.setCursor(0, 1);
      lcd.print(F("Add more songs"));
      delay(2000);
      queueEmptyAlert = false;
      wasPlaying = false;
    } if (wasPlaying) {   //removes the first item from the queue and shifts all the other items forward to fill the gap
      Song *newQueue[queueMenuCurrentLength];
      for (uint8_t i = 1; i < queueMenuCurrentLength; i++) {
        newQueue[i - 1] = queue[i];
      }
      queueMenuCurrentLength -= 1;
      memcpy(&queue, &newQueue, sizeof(newQueue));

      if (queueMenuCurrentLength == 0) {
        queueEmptyAlert = true;
      }
    } if (queueMenuCurrentLength > 0) {
      memcpy_P(&queueMenuCurrentItem, queue[0], sizeof(Song));

      musicPlayer.startPlayingFile(queueMenuCurrentItem.filename);
      wasPlaying = true;
    }
  }

  if (menuState == MAIN_MENU) {   //logic for the mainMenu
    setDisplayText(mainMenuCurrentItem, "");

    if (encoderValueChange != 0) {  //if the rotary encoder has been turned, changes the mainMenuCurrentItem based on which direction it was turned
      uint8_t itemLen = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);
      if (mainMenuCurrentIndex + 1 >= itemLen && encoderValueChange == 1) { //set current item to first element if previous element was the last and change is +1
        mainMenuCurrentIndex = 0;
      } else if (mainMenuCurrentIndex <= 0 && encoderValueChange == -1) { //set the current item to last element if previous element was the first and change is -1
        mainMenuCurrentIndex = itemLen - 1;
      } else {  //set the current item based on the value of encoderValueChange
        mainMenuCurrentIndex += encoderValueChange;
      }

      strcpy_P(mainMenuCurrentItem, (char *)pgm_read_word(&(mainMenuItems[mainMenuCurrentIndex]))); //copy the current item into SRAM

      encoderValueChange = 0;
    }

    if (encoderButton != ClickEncoder::Open) {
      switch (encoderButton) {
        case ClickEncoder::Clicked:
          Serial.println(F("Button Clicked"));

          if (strcmp(mainMenuCurrentItem, "Go to Genre Menu") == 0) {
            menuState = GENRE_MENU;
            Serial.println(F("menu is now genreMenu"));
          } else if (strcmp(mainMenuCurrentItem, "Go to Queue") == 0) {
            menuState = QUEUE_MENU;
            Serial.println(F("menu is now queueMenu"));
          }          
          break;
        case ClickEncoder::Held:    //a way to skip songs
          musicPlayer.stopPlaying();
          delay(500);
      }
    }
  } //end mainMenu logic

  else if (menuState == GENRE_MENU) {   //logic for genreMenu
    setDisplayText(genreMenuCurrentItem, "");

    if (encoderValueChange != 0) {  //if the rotary encoder has been turned, changes the genreMenuCurrentItem based on which direction it was turned
      uint8_t itemLen = sizeof(genreMenuItems) / sizeof(genreMenuItems[0]);
      if (genreMenuCurrentIndex + 1 >= itemLen && encoderValueChange == 1) { //set current item to first element if previous element was the last and change is +1
        genreMenuCurrentIndex = 0;
      } else if (genreMenuCurrentIndex <= 0 && encoderValueChange == -1) { //set the current item to last element if previous element was the first and change is -1
        genreMenuCurrentIndex = itemLen - 1;
      } else {  //set the current item based on the value of encoderValueChange
        genreMenuCurrentIndex += encoderValueChange;
      }

      strcpy_P(genreMenuCurrentItem, (char *)pgm_read_word(&(genreMenuItems[genreMenuCurrentIndex]))); //copy the current item into SRAM

      encoderValueChange = 0;
    }

    if (encoderButton != ClickEncoder::Open) {
      switch (encoderButton) {
        case ClickEncoder::Clicked:
          Serial.println(F("Button Clicked"));
          menuState = SONG_MENU;

          //checks the current item in the genre menu against each genre, and then sets the song menu pointer to the first element of the list for the selected genre
          strcpy_P(stringSRAM, (char *)pgm_read_word(&(genreMenuItems[0])));
          if (strcmp(genreMenuCurrentItem, stringSRAM) == 0) {  //check if metal
            songMenuPtr = metalList;  //same as songMenuPtr = &metalList[0]
            songMenuListLength = sizeof(metalList) / sizeof(Song);  //have to divide by sizeof(Song) bc sizeof() gets the size in bytes
          }

          strcpy_P(stringSRAM, (char *)pgm_read_word(&(genreMenuItems[1])));  //saves memory by reusing the same string every time
          if (strcmp(genreMenuCurrentItem, stringSRAM) == 0) {  //check if rock
            songMenuPtr = rockList;
            songMenuListLength = sizeof(rockList) / sizeof(Song);
          }

          strcpy_P(stringSRAM, (char *)pgm_read_word(&(genreMenuItems[2])));
          if (strcmp(genreMenuCurrentItem, stringSRAM) == 0) {  //check if pop
            songMenuPtr = popList;
            songMenuListLength = sizeof(popList) / sizeof(Song);
          }

          strcpy_P(stringSRAM, (char *)pgm_read_word(&(genreMenuItems[3])));
          if (strcmp(genreMenuCurrentItem, stringSRAM) == 0) {  //check if jazz
            songMenuPtr = jazzList;
            songMenuListLength = sizeof(jazzList) / sizeof(Song);
          }

          strcpy_P(stringSRAM, (char *)pgm_read_word(&(genreMenuItems[4])));
          if (strcmp(genreMenuCurrentItem, stringSRAM) == 0) {  //check if classical
            songMenuPtr = classicalList;
            songMenuListLength = sizeof(classicalList) / sizeof(Song);
          }

          strcpy_P(stringSRAM, (char *)pgm_read_word(&(genreMenuItems[5])));
          if (strcmp(genreMenuCurrentItem, stringSRAM) == 0) {  //check if rap
            songMenuPtr = rapList;
            songMenuListLength = sizeof(rapList) / sizeof(Song);
          }

          strcpy_P(stringSRAM, (char *)pgm_read_word(&(genreMenuItems[6])));
          if (strcmp(genreMenuCurrentItem, stringSRAM) == 0) {  //check if electronic
            Serial.println(F("hi"));
            songMenuPtr = electronicList;
            songMenuListLength = sizeof(electronicList) / sizeof(Song);
          }

          memcpy_P(&songMenuCurrentItem, songMenuPtr, sizeof(Song));  //set songMenuCurrentItem
          songMenuCurrentIndex = 0;
          break;
      }
    }

    if (backButtonState == HIGH && backButtonStatePrev == LOW) {  //go back to mainMenu
      menuState = MAIN_MENU;
      Serial.println(F("back button pressed"));
    }

  }   //end genreMenu logic

  else if (menuState == SONG_MENU) {    //logic for songMenu
    setDisplayText(songMenuCurrentItem.title, songMenuCurrentItem.artist);

    if (encoderValueChange != 0) {  //if the rotary encoder has been turned, changes the songMenuCurrentItem based on which direction it was turned
      if (songMenuCurrentIndex + 1 >= songMenuListLength && encoderValueChange == 1) { //set current item to first element if previous element was the last and change is +1
        songMenuCurrentIndex = 0;
      } else if (songMenuCurrentIndex <= 0 && encoderValueChange == -1) { //set the current item to last element if previous element was the first and change is -1
        songMenuCurrentIndex = songMenuListLength - 1;
      } else {  //set the current item based on the value of encoderValueChange
        songMenuCurrentIndex += encoderValueChange;
      }

      memcpy_P(&songMenuCurrentItem, songMenuPtr + songMenuCurrentIndex, sizeof(Song)); //copy the current item into SRAM

      encoderValueChange = 0;
    }

    if (encoderButton != ClickEncoder::Open) {
      switch (encoderButton) {
        case ClickEncoder::Clicked:
          Serial.println(F("Button Clicked"));

          if (queueMenuCurrentLength < MAX_QUEUE_LENGTH) {  //if queue isnt full, adds the displayed song to queue
            queueMenuCurrentLength += 1;
            queue[queueMenuCurrentLength - 1] = songMenuPtr + songMenuCurrentIndex; //add new song to last index
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Song Added"));
            menuState = MAIN_MENU;
            delay(1500);
          } else {    //queue is full
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Queue full"));
            lcd.setCursor(0, 1);
            lcd.print(F("Let song end"));
            delay(2000);
          }
          break;
      }
    }

    if (backButtonState == HIGH) {  //go back to genreMenu
      menuState = GENRE_MENU;
      //delay(500);   //makes sure that the back button isn't triggered in the genreMenu as well
    }
  }   //end songMenu logic

  else if (menuState == QUEUE_MENU) {
    if (queueMenuCurrentLength == 0) {
      setDisplayText(F("Queue Empty"), F("Add more songs"));
    } else {
      sprintf(stringSRAM, "%i. %s", queueMenuCurrentIndex + 1, queueMenuCurrentItem.title);  //add position in queue

      setDisplayText(stringSRAM, queueMenuCurrentItem.artist);

      if (encoderValueChange != 0) {  //if the rotary encoder has been turned, changes the queueMenuCurrentItem based on which direction it was turned
        if (queueMenuCurrentIndex + 1 >= queueMenuCurrentLength && encoderValueChange == 1) { //set current item to first element if previous element was the last and change is +1
          queueMenuCurrentIndex = 0;
        } else if (queueMenuCurrentIndex <= 0 && encoderValueChange == -1) { //set the current item to last element if previous element was the first and change is -1
          queueMenuCurrentIndex = queueMenuCurrentLength - 1;
        } else {  //set the current item based on the value of encoderValueChange
          queueMenuCurrentIndex += encoderValueChange;
        }

        memcpy_P(&queueMenuCurrentItem, queue[queueMenuCurrentIndex], sizeof(Song)); //copy the current item into SRAM

        encoderValueChange = 0;
      }
    }

    if (backButtonState == HIGH) {  //go back to mainMenu
      menuState = MAIN_MENU;
      Serial.println(F("back button pressed"));
    }
  }
}


void setDisplayText(const String& line1, const String& line2) {   //sets the two variables for what to display on the lcd screen and resets the scrolling variables
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

// https://bit.ly/3ernfRt
