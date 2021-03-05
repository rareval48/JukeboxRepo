// Paul Schakel
// The sketch for the Jukebox Project.


#include <JukeboxUtils.h>
#include <SPI.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <Adafruit_VS1053.h>
#include <LiquidCrystal_I2C.h>
#include <avr/pgmspace.h>
#include <PGMWrap.h>

// These are the pins used for the music maker shield
#define SHIELD_RESET  -1      // VS1053 reset pin (unused!)
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)
#define CARDCS 4     // Card chip select pin
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

// These are the different menu states which are possible
#define MAIN_MENU 0
#define QUEUE_MENU 1
#define GENRE_MENU 2
#define SONG_MENU 3

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
uint8_t encoderValuePrevChange = 0; //used to check how much encoderValue changed last cycle

int menuState = MAIN_MENU;

//define all songs
const char song001Title[] PROGMEM = "Money For Nothing";
const char song001Filename[] PROGMEM = "/track001.mp3";
const char song001Genre[] PROGMEM = "Rock";
const char song001Artist[] PROGMEM = "Dire Straits";
Song song001 = Song(song001Title, song001Filename, song001Genre, song001Artist);

const char song002Title[] PROGMEM = "Blue In Green";
const char song002Filename[] PROGMEM = "/track002.mp3";
const char song002Genre[] PROGMEM = "Jazz";
const char song002Artist[] PROGMEM = "Miles Davis";
Song song002 = Song(song002Title, song002Filename, song002Genre, song002Artist);

const char song003Title[] PROGMEM = "Fade To Black";
const char song003Filename[] PROGMEM = "/track003.mp3";
const char song003Genre[] PROGMEM = "Heavy Metal";
const char song003Artist[] PROGMEM = "Metallica";
Song song003  = Song(song003Title, song003Filename, song003Genre, song003Artist);

const char song004Title[] PROGMEM = "Pull Me Under";
const char song004Filename[] PROGMEM = "/track004.mp3";
const char song004Genre[] PROGMEM = "Heavy Metal";
const char song004Artist[] PROGMEM = "Dream Theater";
Song song004  = Song(song004Title, song004Filename, song004Genre, song004Artist);

const char song005Title[] PROGMEM = "Where The Streets Have No Name";
const char song005Filename[] PROGMEM = "/track005.mp3";
const char song005Genre[] PROGMEM = "Rock";
const char song005Artist[] PROGMEM = "U2";
Song song005  = Song(song005Title, song005Filename, song005Genre, song005Artist);

//create songList
Song songList[5] = {song001, song002, song003, song004, song005};

//create mainMenu
const char mainMenuItem0[] PROGMEM = "Go to Genre Menu";
const char mainMenuItem1[] PROGMEM = "Go to Queue";
String mainMenuItems[2] = {mainMenuItem0, mainMenuItem1};
const Menu mainMenu = Menu(mainMenuItems);

//create queueMenu
Song queueMenuItems[0]  = {};
const QueueMenu queueMenu  = QueueMenu(queueMenuItems);

//create genreMenu
const char genre0[] PROGMEM = "Heavy Metal";
const char genre1[] PROGMEM = "Rock";
const char genre2[] PROGMEM = "Pop";
const char genre3[] PROGMEM = "Jazz";
const char genre4[] PROGMEM = "Classical";
const char genre5[] PROGMEM = "Hip Hop/Rap";
const char genre6[] PROGMEM = "Misc.";
const char genre7[] PROGMEM = "Electronic";
String genreMenuItems[8]  = {genre0, genre1, genre2, genre3, genre4, genre5, genre6, genre7};
const Menu genreMenu  = Menu(genreMenuItems);

//create songMenu
Song songMenuItems[0]  = {};
const SongMenu songMenu  = SongMenu(songMenuItems);

void setup() {
  Serial.begin(9600);

  lcd.init();                      // initialise the lcd
  lcd.backlight();

  //reserve space for manipulating these strings
  textLine1.reserve(33);
  textLine2.reserve(33);

  encoder = new ClickEncoder(A1, A0, A2);

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);
  last = -1;

  Serial.println(F("help"));


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

  //Serial.println(songOne._title);
  //Serial.println(songOne._artist);
  Serial.println(mainMenu._items[0]);
}

void loop() {
  //keep track of timings for LCD scrolling
  clockPrev = clock;
  clock = millis();

  //manage rotary encoder
  int encoderValueChange = encoder->getValue();
  encoderValuePrev = encoderValue;
  encoderValue += encoderValueChange;
  if (encoderValuePrevChange != 0 && encoderValuePrev != encoderValue) {  //makes sure that moving the rotary encoder one click only moves the menu by one
    encoderValueChange = 0;
  }
  encoderValuePrevChange = encoderValue - encoderValuePrev;
  ClickEncoder::Button encoderButton = encoder->getButton();

  if (clock % 500 <= 100 && clockPrev % 500 > 100) {  //scrolls the text twice a second
    displayText();
    Serial.println(F("scrolled text"));
  }


  if (menuState == MAIN_MENU) {
    char currentItem[16];
    strcpy_P(currentItem, (char *)pgm_read_word(&(mainMenu._currentItem)));
    setDisplayText(currentItem, "");
    Serial.println(currentItem);
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


void addSongToQueue() {   //adds the current song in the songMenu to the back of the queue
  int itemLen = sizeof(queueMenu._items) / sizeof(queueMenu._items);
  if (itemLen <= 6) {
    Song newQueue[itemLen + 1];  //new queue with one extra index

    memcpy(newQueue, queueMenu._items, sizeof queueMenu._items);  //copy values of old queue

    newQueue[itemLen + 1] = songMenu._currentItem; //add new song to last index

    memcpy(queueMenu._items, newQueue, sizeof newQueue);   //replace old queue with the new one
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Queue full"));
    lcd.setCursor(0, 1);
    lcd.print(F("Let song end"));
  }
}


void createSongMenu() {
  Song songsInGenre[6];
  int index = 0;

  for (int i = 0; i < sizeof(songList); i++) {
    Song currentSong = songList[i];
    if (currentSong._genre == genreMenu._currentItem) {
      songsInGenre[index] = currentSong;
      index++;
    }
  }

  songMenu = SongMenu(songsInGenre);
}
