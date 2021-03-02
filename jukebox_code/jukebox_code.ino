// Paul Schakel
// The sketch for the Jukebox Project.


#include <JukeboxUtils.h>
#include <SPI.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <Adafruit_VS1053.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>


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

LiquidCrystal_I2C lcd(0x3F, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

ClickEncoder *encoder;
int16_t last;

void timerIsr() {   //used by the ClickEncoder
  encoder->service();
}

//used for text scrolling in displayText()
int screenWidth = 16;
int screenHeight = 2;
String textLine1 = " ";
String textLine2 = " ";
int stringStart = 0;
int stringStop = screenWidth;
int scrollCursor = 0;

int menuState = MAIN_MENU;

String mainMenuItems[] = {"Go to Genre Menu", "Go to Queue"};
Menu mainMenu = Menu(mainMenuItems, 2);
//QueueMenu queueMenu;
//Menu genreMenu;
//SongMenu songMenu;

void setup() {
  Serial.begin(9600);
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

  if (!SD.begin(CARDCS)) {  //initialise SD card
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }

  musicPlayer.setVolume(20, 20);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT);   //lets it play in the background

  //test song
  Song songOne;
  songOne.setValues("Blind", "/track051.mp3", "Heavy Metal", "Korn");
  Serial.println(songOne._title);
  Serial.println(songOne._artist);
}

void loop() {
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

void addSongToQueue() {   //adds the current song in the songMenu to the back of the queue
    Song newQueue[queueMenu._numberOfItems];  //new queue with one extra index

    Song copyingVar;
    for (i = 0; copyingVar = queueMenu._items[i]; i++) {  //copy values of old queue
      newQueue[i] = copyingVar;
    }
    newQueue[queueMenu._numberOfItems] = songMenu._currentItem; //add new song to last index

    memcpy(queueMenu._items, newQueue, sizeof(newQueue[0]) * queueMenu._numberOfItems);   //replace old queue with the new one
    queueMenu._numberOfItems++;   //update number of items
}


//void createSongMenu(songList) {
//    Song songsInGenre[20];
//    int index = 0;
//
//    for (int i = 0; i < songList.length(); i++) {
//      Song currentSong = songList[i]
//      if (currentSong._genre == _currentItem) {
//        songsInGenre[index] = currentSong;
//        index++;
//      }
//    }
//
//    return new SongMenu(songsInGenre, _currentItem, )
//}
