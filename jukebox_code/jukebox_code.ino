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
#define HIDDEN_MENU 4

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


struct genreListInfo {  //used for creating songMenus
  Song *genreList;
  uint8_t length;
};


Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

LiquidCrystal_I2C lcd(0x3F, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

ClickEncoder *encoder;
int16_t last;

void timerIsr() {   //used by the ClickEncoder
  encoder->service();
}

int backButtonState;

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

bool hiddenMenuTrigger = false;   //used to enter the hidden menu
bool canEditQueue = false;  //feature which can be activated in hidden menu

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
uint8_t mainMenuCurrentIndex = 0;

//create queue
Song *queue[MAX_QUEUE_LENGTH] = {};
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
uint8_t genreMenuCurrentIndex = 0;
const genreListInfo genreLists[] PROGMEM =    //used for creating new songMenus
{
  {metalList, sizeof(metalList) / sizeof(Song)},
  {rockList, sizeof(rockList) / sizeof(Song)},
  {popList, sizeof(popList) / sizeof(Song)},
  {jazzList, sizeof(jazzList) / sizeof(Song)},
  {classicalList, sizeof(classicalList) / sizeof(Song)},
  {rapList, sizeof(rapList) / sizeof(Song)},
  {electronicList, sizeof(electronicList) / sizeof(Song)},
};

//create songMenu
Song *songMenuPtr;
uint8_t songMenuCurrentIndex = 0;
uint8_t songMenuListLength = 0;

//create hiddenMenu
const char hiddenMenuItem0[] PROGMEM = "Pause/Play";
const char hiddenMenuItem1[] PROGMEM = "Skip Song";
const char hiddenMenuItem2[] PROGMEM = "Edit Queue";
const char hiddenMenuItem3[] PROGMEM = "Clear Queue";
const char *const hiddenMenuItems[] PROGMEM = {hiddenMenuItem0, hiddenMenuItem1, hiddenMenuItem2, hiddenMenuItem3};
uint8_t hiddenMenuCurrentIndex = 0;

//create large SRAM variables
char stringSRAM[MAX_STRING_LENGTH];
char menuCurrentItemString[MAX_STRING_LENGTH];
Song menuCurrentItemSong;

void setup() {
  Serial.begin(9600);

  pinMode(BACK_BUTTON_PIN, INPUT);  //set up back button

  //set current item for main menu
  strcpy_P(menuCurrentItemString, (char *)pgm_read_word(&(mainMenuItems[0])));

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
      memcpy_P(&menuCurrentItemSong, queue[0], sizeof(Song));

      musicPlayer.startPlayingFile(menuCurrentItemSong.filename);
      wasPlaying = true;
    }
  }

  if (menuState == MAIN_MENU) {   //logic for the mainMenu
    setDisplayText(menuCurrentItemString, "");

    if (encoderValueChange != 0) {  //if the rotary encoder has been turned, changes the current menu item based on which direction it was turned
      uint8_t itemLen = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);
      if (mainMenuCurrentIndex + 1 >= itemLen && encoderValueChange == 1) { //set current item to first element if previous element was the last and change is +1
        mainMenuCurrentIndex = 0;
      } else if (mainMenuCurrentIndex <= 0 && encoderValueChange == -1) { //set the current item to last element if previous element was the first and change is -1
        mainMenuCurrentIndex = itemLen - 1;
      } else {  //set the current item based on the value of encoderValueChange
        mainMenuCurrentIndex += encoderValueChange;
      }

      strcpy_P(menuCurrentItemString, (char *)pgm_read_word(&(mainMenuItems[mainMenuCurrentIndex]))); //copy the current item into SRAM

      encoderValueChange = 0;
    }

    if (encoderButton != ClickEncoder::Open) {
      switch (encoderButton) {
        case ClickEncoder::Clicked:
          Serial.println(F("Button Clicked"));

          if (strcmpProgmem(menuCurrentItemString, &(mainMenuItems[0]))) {
            menuState = GENRE_MENU;
            strcpy_P(menuCurrentItemString, (char *)pgm_read_word(&(genreMenuItems[genreMenuCurrentIndex]))); //set the current item for genre menu
            Serial.println(F("menu is now genreMenu"));
          }

          if (strcmpProgmem(menuCurrentItemString, &(mainMenuItems[1]))) {
            menuState = QUEUE_MENU;
            memcpy_P(&menuCurrentItemSong, queue[0], sizeof(Song));  //set the current item for queue menu
            Serial.println(F("menu is now queueMenu"));
          }
          break;
      }
    }
  } //end mainMenu logic

  else if (menuState == GENRE_MENU) {   //logic for genreMenu
    setDisplayText(menuCurrentItemString, "");

    if (encoderValueChange != 0) {  //if the rotary encoder has been turned, changes the current item based on which direction it was turned
      uint8_t itemLen = sizeof(genreMenuItems) / sizeof(genreMenuItems[0]);
      if (genreMenuCurrentIndex + 1 >= itemLen && encoderValueChange == 1) { //set current item to first element if previous element was the last and change is +1
        genreMenuCurrentIndex = 0;
      } else if (genreMenuCurrentIndex <= 0 && encoderValueChange == -1) { //set the current item to last element if previous element was the first and change is -1
        genreMenuCurrentIndex = itemLen - 1;
      } else {  //set the current item based on the value of encoderValueChange
        genreMenuCurrentIndex += encoderValueChange;
      }

      strcpy_P(menuCurrentItemString, (char *)pgm_read_word(&(genreMenuItems[genreMenuCurrentIndex]))); //copy the current item into SRAM

      encoderValueChange = 0;
    }

    if (encoderButton != ClickEncoder::Open) {
      switch (encoderButton) {
        case ClickEncoder::Clicked:
          Serial.println(F("Button Clicked"));
          menuState = SONG_MENU;
          
          //checks the current item in the genre menu against each genre, and then sets the song menu pointer to the first element of the list for the selected genre
          for (int i = 0; i < sizeof(genreMenuItems) / sizeof(genreMenuItems[0]); i++) {
            if (strcmpProgmem(menuCurrentItemString, &(genreMenuItems[i]))) {
              songMenuPtr = pgm_read_word(&(genreLists[i].genreList));    //read data out of progmem
              songMenuListLength = pgm_read_word(&(genreLists[i].length));  //same thing here
            }
          }

          memcpy_P(&menuCurrentItemSong, songMenuPtr, sizeof(Song));  //set current item for song menu
          songMenuCurrentIndex = 0;
          break;
      }
    }

    if (backButtonState == HIGH) {  //go back to mainMenu
      menuState = MAIN_MENU;
      strcpy_P(menuCurrentItemString, (char *)pgm_read_word(&(mainMenuItems[mainMenuCurrentIndex]))); //set current item for main menu
      Serial.println(F("back button pressed"));
    }

  }   //end genreMenu logic

  else if (menuState == SONG_MENU) {    //logic for songMenu
    setDisplayText(menuCurrentItemSong.title, menuCurrentItemSong.artist);

    if (encoderValueChange != 0) {  //if the rotary encoder has been turned, changes the current item based on which direction it was turned
      if (songMenuCurrentIndex + 1 >= songMenuListLength && encoderValueChange == 1) { //set current item to first element if previous element was the last and change is +1
        songMenuCurrentIndex = 0;
      } else if (songMenuCurrentIndex <= 0 && encoderValueChange == -1) { //set the current item to last element if previous element was the first and change is -1
        songMenuCurrentIndex = songMenuListLength - 1;
      } else {  //set the current item based on the value of encoderValueChange
        songMenuCurrentIndex += encoderValueChange;
      }

      memcpy_P(&menuCurrentItemSong, songMenuPtr + songMenuCurrentIndex, sizeof(Song)); //copy the current item into SRAM

      encoderValueChange = 0;
    }

    if (encoderButton != ClickEncoder::Open) {
      switch (encoderButton) {
        case ClickEncoder::Clicked:
          Serial.println(F("Button Clicked"));

          if (queueMenuCurrentLength < MAX_QUEUE_LENGTH) {  //if queue isnt full, adds the displayed song to queue
            queue[queueMenuCurrentLength] = songMenuPtr + songMenuCurrentIndex; //add new song to last index
            queueMenuCurrentLength += 1;
            menuState = MAIN_MENU;
            strcpy_P(menuCurrentItemString, (char *)pgm_read_word(&(mainMenuItems[mainMenuCurrentIndex]))); //set current item for main menu
            if (musicPlayer.paused() && queueMenuCurrentLength == 0) {  //starts playing the song if playback is paused and there isn't any additional song in the queue
              memcpy_P(&menuCurrentItemSong, queue[0], sizeof(Song));

              musicPlayer.startPlayingFile(menuCurrentItemSong.filename);
              wasPlaying = true;
            }
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Song Added"));
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
      strcpy_P(menuCurrentItemString, (char *)pgm_read_word(&(genreMenuItems[genreMenuCurrentIndex]))); //set the current item for genre menu
      delay(500);   //makes sure that the back button isn't triggered in the genreMenu as well
    }
  }   //end songMenu logic

  else if (menuState == QUEUE_MENU) {   //logic for queueMenu
    if (queueMenuCurrentLength == 0) {
      setDisplayText(F("Queue Empty"), F("Add more songs"));
    } else {
      sprintf(stringSRAM, "%i. %s", queueMenuCurrentIndex + 1, menuCurrentItemSong.title);  //add position in queue

      setDisplayText(stringSRAM, menuCurrentItemSong.artist);

      if (encoderValueChange != 0) {  //if the rotary encoder has been turned, changes the current item based on which direction it was turned
        if (queueMenuCurrentIndex + 1 >= queueMenuCurrentLength && encoderValueChange == 1) { //set current item to first element if previous element was the last and change is +1
          queueMenuCurrentIndex = 0;
        } else if (queueMenuCurrentIndex <= 0 && encoderValueChange == -1) { //set the current item to last element if previous element was the first and change is -1
          queueMenuCurrentIndex = queueMenuCurrentLength - 1;
        } else {  //set the current item based on the value of encoderValueChange
          queueMenuCurrentIndex += encoderValueChange;
        }

        memcpy_P(&menuCurrentItemSong, queue[queueMenuCurrentIndex], sizeof(Song)); //copy the current item into SRAM

        encoderValueChange = 0;
      }
    }

    if (encoderButton != ClickEncoder::Open) {
      switch (encoderButton) {
        case ClickEncoder::Clicked:
          Serial.println(F("Button Clicked"));
          if (canEditQueue) {
            Song *newQueue[queueMenuCurrentLength];
            for (uint8_t i = 0; i < queueMenuCurrentLength; i++) {
              if (i == queueMenuCurrentIndex) {
                continue;
              } else if (i > queueMenuCurrentIndex) {
                newQueue[i - 1] = queue[i];
              } else {
                newQueue[i] = queue[i];
              }
            }
            queueMenuCurrentLength -= 1;
            memcpy(&queue, &newQueue, sizeof(newQueue));

            if (queueMenuCurrentIndex == 0) { //stops playing if the first song in the queue was removed (the song which was playing)
              musicPlayer.stopPlaying();
              wasPlaying = false;   //makes sure there isn't a double skip
            }

            queueMenuCurrentIndex = 0;
            memcpy_P(&menuCurrentItemSong, queue[queueMenuCurrentIndex], sizeof(Song)); //copy the current item into SRAM

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Removed Song"));
            delay(1500);
          }
          break;
        
        case ClickEncoder::Held:
          Serial.println(F("Button Held"));

          if (!hiddenMenuTrigger) {
            hiddenMenuTrigger = true;
          } else {
            hiddenMenuTrigger = false;
          }
          delay(1000); //otherwise it just gets triggered repeatedly
          break;

        case ClickEncoder::DoubleClicked:
          Serial.println(F("Button DoubleClicked"));

          if (hiddenMenuTrigger) {
            menuState = HIDDEN_MENU;
            hiddenMenuTrigger = false;
            canEditQueue = false;
            strcpy_P(menuCurrentItemString, (char *)pgm_read_word(&(hiddenMenuItems[hiddenMenuCurrentIndex]))); //set current item for hidden menu
            Serial.println(F("Entered Hidden Menu"));
          }
          break;
      }
    }

    if (backButtonState == HIGH) {  //go back to mainMenu
      menuState = MAIN_MENU;
      hiddenMenuTrigger = false; //reset trigger
      canEditQueue = false;
      strcpy_P(menuCurrentItemString, (char *)pgm_read_word(&(mainMenuItems[mainMenuCurrentIndex]))); //set current item for main menu
      Serial.println(F("back button pressed"));
    }
  }   //end queueMenu logic

  else if (menuState == HIDDEN_MENU) {    //logic for hidden menu -- used to pause/play playback, play next song, and clear and edit the queue
    setDisplayText(menuCurrentItemString, "");

    if (encoderValueChange != 0) {  //if the rotary encoder has been turned, changes the current menu item based on which direction it was turned
      uint8_t itemLen = sizeof(hiddenMenuItems) / sizeof(hiddenMenuItems[0]);
      if (hiddenMenuCurrentIndex + 1 >= itemLen && encoderValueChange == 1) { //set current item to first element if previous element was the last and change is +1
        hiddenMenuCurrentIndex = 0;
      } else if (hiddenMenuCurrentIndex <= 0 && encoderValueChange == -1) { //set the current item to last element if previous element was the first and change is -1
        hiddenMenuCurrentIndex = itemLen - 1;
      } else {  //set the current item based on the value of encoderValueChange
        hiddenMenuCurrentIndex += encoderValueChange;
      }

      strcpy_P(menuCurrentItemString, (char *)pgm_read_word(&(hiddenMenuItems[hiddenMenuCurrentIndex]))); //copy the current item into SRAM

      encoderValueChange = 0;
    }

    if (encoderButton!= ClickEncoder::Open) {
      switch (encoderButton) {
      case ClickEncoder::Clicked:
        Serial.println(F("Button Clicked"));

        if (strcmpProgmem(menuCurrentItemString, &(hiddenMenuItems[0]))) {   //check if pause/play
          if (musicPlayer.stopped()) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Can't pause"));
            lcd.setCursor(0, 1);
            lcd.print(F("Not playing"));
            delay(1500);
          } else if (!musicPlayer.paused()) {
            musicPlayer.pausePlaying(true);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Paused playback"));
            delay(1500);
          } else {
            musicPlayer.pausePlaying(false);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Resumed playback"));
            delay(1500);
          }
        }

        if (strcmpProgmem(menuCurrentItemString, &(hiddenMenuItems[1]))) {   //check if skip song
          if (queueMenuCurrentLength != 0) {
            musicPlayer.stopPlaying();
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Song skipped"));
            delay(2000);
          } else {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("No Song to Skip"));
            lcd.setCursor(0, 1);
            lcd.print(F("Queue Empty"));
            delay(2000);
          }
        }

        if (strcmpProgmem(menuCurrentItemString, &(hiddenMenuItems[2]))) {   //check if queue edit mode
          if (queueMenuCurrentLength != 0) {
            canEditQueue = true;
            menuState = QUEUE_MENU;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Queue editing"));
            lcd.setCursor(0, 1);
            lcd.print(F("mode activated"));
            delay(2000);
          } else {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Queue Empty"));
            lcd.setCursor(0, 1);
            lcd.print(F("Can't edit"));
            delay(2000);
          }
        }

        if (strcmpProgmem(menuCurrentItemString, &(hiddenMenuItems[3]))) {   //check if clear queue
          if (queueMenuCurrentLength != 0) {
            queueMenuCurrentLength = 0;
            queueMenuCurrentIndex = 0;
            queueEmptyAlert = true;
            Song *newQueue[MAX_QUEUE_LENGTH];
            memcpy(&queue, &newQueue, sizeof(newQueue));
            musicPlayer.stopPlaying();
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Queue cleared"));
            delay(2000);
          } else {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Queue Empty"));
            lcd.setCursor(0, 1);
            lcd.print(F("Already clear"));
            delay(2000);
          }
        }
        break;
      }
    }

    if (backButtonState == HIGH) {
      menuState = QUEUE_MENU;
      memcpy_P(&menuCurrentItemSong, queue[queueMenuCurrentIndex], sizeof(Song)); //copy the current item into SRAM      
      Serial.println(F("back button pressed"));
      delay(500);
    }
  }
}


bool strcmpProgmem(const char (& toCompareSRAM)[MAX_STRING_LENGTH], const char* const* toComparePROGMEM) {
  strcpy_P(stringSRAM, (char *)pgm_read_word(toComparePROGMEM)); //saves SRAM by using existing allocated memory

  if (strcmp(toCompareSRAM, stringSRAM) == 0) {
    return true;
  } else {
    return false;
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
