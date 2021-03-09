// Paul Schakel
// The sketch for the Jukebox Project.


#include <SPI.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <Adafruit_VS1053.h>
#include <LiquidCrystal_I2C.h>
#include <MemoryFree.h>

// These are the pins used for the music maker shield
#define SHIELD_RESET  -1      // VS1053 reset pin
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)
#define CARDCS 4     // Card chip select pin
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

// These are the different menu states which are possible
#define MAIN_MENU 0
#define QUEUE_MENU 1
#define GENRE_MENU 2
#define SONG_MENU 3

//set maximums
#define MAX_QUEUE_LENGTH 10
#define MAX_STRING_LENGTH 32
#define SHORTER_STRING_LENGTH 15

//define structure datatypes
struct Song {
  char title[MAX_STRING_LENGTH];
  char filename[SHORTER_STRING_LENGTH];
  char genre[SHORTER_STRING_LENGTH];
  char artist[SHORTER_STRING_LENGTH];
};

/*
  class Menu {
  private:

  public:
    char *itemsPtr;
    uint8_t numItems;
    char *currentItem;

    Menu(char *newItems, uint8_t newNumItems) {
      itemsPtr = newItems;
      numItems = newNumItems;
      currentItem = itemsPtr;
    }
  };

  class SongMenu {
  private:

  public:
    Song *itemsPtr;
    uint8_t numItems;
    Song *currentItem;

    SongMenu(Song *newItems, uint8_t newNumItems) {
      itemsPtr = newItems;
      numItems = newNumItems;
      currentItem = itemsPtr;
    }
  };*/

Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

LiquidCrystal_I2C lcd(0x3F, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

ClickEncoder *encoder;
int16_t last;

void timerIsr() {   //used by the ClickEncoder
  encoder->service();
}

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

/*//create songList
  const Song songList[] PROGMEM =
  {
  {"Money For Nothing", "/track001.mp3", "Rock", "Dire Straits"},
  {"Blue In Green", "/track002.mp3", "Jazz", "Miles Davis"},
  {"Fade To Black", "/track003.mp3", "Heavy Metal", "Metallica"},
  {"Pull Me Under", "/track004.mp3", "Heavy Metal", "Dream Theater"},
  {"Where The Streets Have No Name", "/track005.mp3", "Rock", "U2"},
  };*/

//create genreLists
const Song rockList[] PROGMEM =
{
  {"Money For Nothing", "/track001.mp3", "Rock", "Dire Straits"},
  {"Where The Streets Have No Name", "/track005.mp3", "Rock", "U2"},
};

const Song metalList[] PROGMEM =
{
  {"Fade To Black", "/track003.mp3", "Heavy Metal", "Metallica"},
  {"Pull Me Under", "/track004.mp3", "Heavy Metal", "Dream Theater"},
};

const Song jazzList[] PROGMEM =
{
  {"Blue In Green", "/track002.mp3", "Jazz", "Miles Davis"},
};

const Song popList[] PROGMEM =
{

};

const Song classicalList[] PROGMEM =
{

};

const Song rapList[] PROGMEM =
{

};

const Song electronicList[] PROGMEM =
{

};

const Song otherList[] PROGMEM =
{

};

//create mainMenu
const char mainMenuItem0[] PROGMEM = "Go to Genre Menu";
const char mainMenuItem1[] PROGMEM = "Go to Queue";
const char *const mainMenuItems[] PROGMEM = {mainMenuItem0, mainMenuItem1};
char mainMenuCurrentItem[MAX_STRING_LENGTH];
uint8_t mainMenuCurrentIndex = 0;

//create queue
Song queue[MAX_QUEUE_LENGTH] = {};
Song nowPlaying;

//create genreMenu
const char genre0[] PROGMEM = "Heavy Metal";
const char genre1[] PROGMEM = "Rock";
const char genre2[] PROGMEM = "Pop";
const char genre3[] PROGMEM = "Jazz";
const char genre4[] PROGMEM = "Classical";
const char genre5[] PROGMEM = "Hip Hop/Rap";
const char genre6[] PROGMEM = "Other";
const char genre7[] PROGMEM = "Electronic";
const char *const genreMenuItems[] PROGMEM = {genre0, genre1, genre2, genre3, genre4, genre5, genre6, genre7};
char genreMenuCurrentItem[MAX_STRING_LENGTH];
uint8_t genreMenuCurrentIndex = 0;

//create songMenu
Song *songMenuPtr;
Song songMenuCurrentItem;
uint8_t songMenuCurrentIndex = 0;

//SRAM song struct
Song songSRAM;

//SRAM String
char stringSRAM[MAX_STRING_LENGTH];

void setup() {
  Serial.begin(9600);

  //set currentItems
  strcpy_P(genreMenuCurrentItem, (char *)pgm_read_word(&(genreMenuItems[0])));
  strcpy_P(mainMenuCurrentItem, (char *)pgm_read_word(&(mainMenuItems[0])));


  lcd.init();   // initialise the lcd
  lcd.backlight();

  //reserve space for manipulating these strings -- reduces heap fragmentation
  textLine1.reserve(33);
  textLine2.reserve(33);

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

  musicPlayer.setVolume(20, 20);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT);   //lets it play in the background

  Serial.println(freeMemory());
  //Serial.println(songOne.artist);
}


void loop() {
  //keep track of timings for LCD scrolling
  clockPrev = clock;
  clock = millis();

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
    timeChangeDetected = 0;
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
  /*
    if (encoderValueChange != 0) {
      changeCurrentItem(encoderValueChange);
      Serial.print(F("EncoderValueChange: "));
      Serial.println(encoderValueChange);
      encoderValueChange = 0;
    }*/

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

          getString(genreMenuItems[0]);
          if (strcmp(genreMenuCurrentItem, stringSRAM) == 0) {
            songMenuPtr = metalList;
          }

          getString(genreMenuItems[1]);
          if (strcmp(genreMenuCurrentItem, stringSRAM) == 0) {
            songMenuPtr = rockList;
          }

          getString(genreMenuItems[2]);
          if (strcmp(genreMenuCurrentItem, stringSRAM)) {
            songMenuPtr = popList;
          }

          getString(genreMenuItems[3]);
          if (strcmp(genreMenuCurrentItem, stringSRAM) == 0) {
            songMenuPtr = jazzList;
          }

          getString(genreMenuItems[4]);
          if (strcmp(genreMenuCurrentItem, stringSRAM) == 0) {
            songMenuPtr = classicalList;
          }

          getString(genreMenuItems[5]);
          if (strcmp(genreMenuCurrentItem, stringSRAM)) {
            songMenuPtr = rapList;
          }

          getString(genreMenuItems[6]);
          if (strcmp(genreMenuCurrentItem, stringSRAM) == 0) {
            songMenuPtr = otherList;
          }

          getString(genreMenuItems[7]);
          if (strcmp(genreMenuCurrentItem, stringSRAM)) {
            songMenuPtr = electronicList;
          }
          break;
      }
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
/*
  void createSongMenu() {
  int numSongs = 0;

  delete [] songMenuItems;

  for (int i = 0; i < sizeof(songList); i++) {
    getSong(songList[i]);
    if (strcmp(genreMenuCurrentItem, songSRAM.genre) == 0) {
      memcpy(&songMenuItems[numSongs], &songSRAM, sizeof(Song));
      numSongs++;
    }
  }

  memcpy_P(&songMenuCurrentItem, &songMenuItems[0], sizeof(Song));
  songMenuCurrentIndex = 0;
  }

  /*
  int indexInItems(char *itemToFind, const uint8_t& menuToCheck) {   //finds the index of an item within an array, its kinda ugly but it does the job (does it really tho?)
  int i  = 0;
  int itemLen;

  if (menuToCheck == MAIN_MENU) {
    itemLen = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);
    Serial.print(F("itemLen indexInItems "));
    Serial.println(itemLen);
    while (i < itemLen) {
      getString(mainMenuItems[i]);
      Serial.print(stringSRAM);
      Serial.print(" == ");
      if (strcmp(stringSRAM, itemToFind) == 0) {
        break;
      }
      i++;
    }
  } else if (menuToCheck == GENRE_MENU) {
    itemLen = sizeof(genreMenuItems) / sizeof(genreMenuItems[0]);
    while (i < itemLen) {
      getString(genreMenuItems[i]);
      if (strcmp(stringSRAM, itemToFind) == 0) {
        break;
      }
      i++;
    }
  } else if (menuToCheck == SONG_MENU) {
    itemLen = sizeof(songMenuItems) / sizeof(songMenuItems[0]);
    while (i < itemLen) {
      getSong(songMenuItems[i]);  //this will eventually set songSRAM to the song to find
      if (strcmp(songSRAM.title, itemToFind) == 0) {
        break;
      }
      i++;
    }
  } else if (menuToCheck == QUEUE_MENU) {
    itemLen = sizeof(queue) / sizeof(queue[0]);
    while (i < itemLen) {
      getSong(queue[i]);  //this will eventually set songSRAM to the song to find
      if (strcmp(songSRAM.title, itemToFind) == 0) {
        break;
      }
      i++;
    }
  }

  if (i < itemLen) {
    return i;
  } else {
    Serial.println(F("Error in indexInItems"));
    return 0;
  }
  }

  void changeCurrentItem(uint8_t amountOfChange) {   //sets the _currentItem to the next item in the array
  if (menuState == MAIN_MENU) {
    int itemLen = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);

    int i = 0;
    Serial.print(F("itemLen indexInItems "));
    Serial.println(itemLen);
    while (i < itemLen) {
      getString(mainMenuItems[i]);
      Serial.print(stringSRAM);
      Serial.print(F(" == "));
      if (strcmp(stringSRAM, mainMenuCurrentItem) == 0) {
        break;
      }
      i++;
    }

    int currentIndex = i;
    Serial.println(currentIndex);
    if (currentIndex + 1 >= itemLen && amountOfChange == 1) {
      Serial.println(F("currentIndex + 1 >= itemLen && amountOfChange == 1"));
      strcpy_P(mainMenuCurrentItem, (char *)pgm_read_word(&(mainMenuItems[0])));
    } else if (currentIndex <= 0 && amountOfChange == -1) {
      Serial.println(F("currentIndex <= 0 && amountOfChange == -1"));
      strcpy_P(mainMenuCurrentItem, (char *)pgm_read_word(&(mainMenuItems[itemLen - 1])));
    } else {
      Serial.println(F("else"));
      strcpy_P(mainMenuCurrentItem, (char *)pgm_read_word(&(mainMenuItems[currentIndex + amountOfChange])));
    }
  } else if (menuState == GENRE_MENU) {
    int itemLen = sizeof(genreMenuItems) / sizeof(genreMenuItems[0]);
    int currentIndex = indexInItems(genreMenuCurrentItem, GENRE_MENU);
    Serial.println(currentIndex);
    if (currentIndex + 1 >= itemLen && amountOfChange == 1) {
      strcpy_P(genreMenuCurrentItem, (char *)pgm_read_word(&(genreMenuItems[0])));
    } else if (currentIndex <= 0 && amountOfChange == -1) {
      strcpy_P(genreMenuCurrentItem, (char *)pgm_read_word(&(genreMenuItems[itemLen - 1])));
    } else {
      strcpy_P(genreMenuCurrentItem, (char *)pgm_read_word(&(genreMenuItems[currentIndex + amountOfChange])));
    }
  } else if (menuState == SONG_MENU) {
    int itemLen = sizeof(songMenuItems) / sizeof(Song);
    int currentIndex = indexInItems(songMenuCurrentItem.title, SONG_MENU);
    Serial.println(currentIndex);
    if (currentIndex + 1 >= itemLen && amountOfChange == 1) {
      memcpy_P(&songMenuCurrentItem, &songMenuItems[0], sizeof(Song));
    } else if (currentIndex <= 0 && amountOfChange == -1) {
      memcpy_P(&songMenuCurrentItem, &songMenuItems[itemLen - 1], sizeof(Song));
    } else {
      memcpy_P(&songMenuCurrentItem, &songMenuItems[currentIndex + amountOfChange], sizeof(Song));
    }
  } else if (menuState == QUEUE_MENU) {
    Serial.println(F("Shouldn't be using this function for the queueMenu. Try playNextSong()"));
  }
  }

  void previousItem() {   //sets the _currentItem to the next item in the array
  if (menuState == MAIN_MENU) {
    int itemLen = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);
    int currentIndex = indexInItems(mainMenuCurrentItem, MAIN_MENU);
    Serial.println(currentIndex);
    if (currentIndex <= 0) {
      strcpy_P(mainMenuCurrentItem, (char *)pgm_read_word(&(mainMenuItems[itemLen - 1])));
    } else {
      strcpy_P(mainMenuCurrentItem, (char *)pgm_read_word(&(mainMenuItems[currentIndex - 1])));
    }
  } else if (menuState == GENRE_MENU) {
    int itemLen = sizeof(genreMenuItems) / sizeof(genreMenuItems[0]);
    int currentIndex = indexInItems(genreMenuCurrentItem, GENRE_MENU);
    Serial.println(currentIndex);
    if (currentIndex <= 0) {
      strcpy_P(genreMenuCurrentItem, (char *)pgm_read_word(&(genreMenuItems[itemLen - 1])));
    } else {
      strcpy_P(genreMenuCurrentItem, (char *)pgm_read_word(&(genreMenuItems[currentIndex - 1])));
    }
  } else if (menuState == SONG_MENU) {
    int itemLen = sizeof(songMenuItems) / sizeof(Song);
    int currentIndex = indexInItems(songMenuCurrentItem.title, SONG_MENU);
    Serial.println(currentIndex);
    if (currentIndex <= 0) {
      memcpy_P(&songMenuCurrentItem, &songMenuItems[itemLen - 1], sizeof(Song));
    } else {
      memcpy_P(&songMenuCurrentItem, &songMenuItems[currentIndex - 1], sizeof(Song));
    }
  } else if (menuState == QUEUE_MENU) {
    Serial.println(F("Shouldn't be using this function for the queueMenu. Try playNextSong()"));
  }
  }*/

void addSongToQueue() {   //adds the current song in the songMenu to the back of the queue
  int itemLen = sizeof(queue) / sizeof(queue);

  if (itemLen <= MAX_QUEUE_LENGTH) {
    Song newQueue[itemLen + 1];  //new queue with one extra index
    memcpy(newQueue, queue, sizeof queue);  //copy values of old queue
    newQueue[itemLen + 1] = songMenuCurrentItem; //add new song to last index
    memcpy(queue, newQueue, sizeof newQueue);   //replace old queue with the new one
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Queue full"));
    lcd.setCursor(0, 1);
    lcd.print(F("Let song end"));
    delay(2000);
  }
}

void playNextSong() {    //plays the next song in the Queue
  int itemLen = sizeof(queue) / sizeof(Song);

  Song newQueue[itemLen - 1];
  for (int i = 1; i < itemLen; i++) {
    memcpy(&newQueue[i - 1], &queue[i], sizeof(Song));
  }

  if (itemLen == 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Queue Empty"));
    lcd.setCursor(0, 1);
    lcd.print(F("Add more songs"));
    delay(2000);
  } else {
    nowPlaying = queue[0];

    musicPlayer.startPlayingFile(nowPlaying.filename);
  }
}

void getSong(const Song &toGet) {
  memcpy_P(&songSRAM, &toGet, sizeof(Song));  //saves the specified Song to SRAM
}

void getString(const String &toGet) {
  strcpy_P(stringSRAM, (char *)pgm_read_word(&toGet));
}
