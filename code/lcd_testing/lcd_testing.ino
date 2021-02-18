// Paul Schakel
// This is my prototyping sandbox for the LCD Display and some other stuff for the Jukebox Project

#include <LiquidCrystal_I2C.h>
#include <ClickEncoder.h>
#include <TimerOne.h>

LiquidCrystal_I2C lcd(0x3F, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

ClickEncoder *encoder;
int16_t last;

void timerIsr() {
  encoder->service();
}

int screenWidth = 16;
int screenHeight = 2;
String textLine1 = "No Line On The Horizon";
String textLine2 = "U2";
int stringStart = 0;
int stringStop = screenWidth;
int scrollCursor = 0;
int counter = 0;
unsigned long clock;

void setup() {
  lcd.init();                      // initialize the lcd
  lcd.backlight();
  Serial.begin(9600);
  encoder = new ClickEncoder(A1, A0, A2);
  
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); 
  last = -1;
}

void loop() {
  clock = millis();
  //Serial.println(clock);
  counter += encoder->getValue();

  if (counter != last) {
    last = counter;
    Serial.print("Encoder Value: ");
    Serial.println(counter);
  }

  if (clock % 500 == 0) {
      displayText();
      //Serial.println("scrolled text");
    }
  
  if (counter / 4 == 1) {
    newTextDisplay("Master of Puppets (Remastered)", "Metallica");
  } else if (counter / 4 == 2) {
    newTextDisplay("I Don't Wanna Be Me", "Type O Negative");
  } else if (counter / 4 == 3) {
    newTextDisplay("Hallowed Be Thy Name", "Iron Maiden");
  } else if (counter / 4 == 4) {
    newTextDisplay("Civil War", "Guns 'n Roses");
  } else if (counter / 4 == 5) {
    newTextDisplay("Symphony 8", "Johann Sebastian Bach");
  }
  //Serial.println(counter);
  //Serial.print(stringStart);
  //Serial.print(" and ");
  //Serial.println(stringStop);
  //Serial.println(scrollCursor);  
}

void newTextDisplay(String line1, String line2) {
  //reset scrolling variables
  if (line1 != textLine1 && line2 != textLine2) {
  stringStart = 0;
  stringStop = screenWidth;
  scrollCursor = 0;
  
  //set text variables
  textLine1 = line1;
  textLine2 = line2;
  }
}

int displayText() {
  String toScroll;
  int lineToScroll;
  int length1 = textLine1.length();
  int length2 = textLine2.length();
  
  lcd.clear();
  if (length1 > 16 && length2 <= 16) {
    toScroll = textLine1;
    lineToScroll = 0;
    lcd.setCursor(0, 1);
    lcd.print(textLine2);
  } else if (length1 <= 16 && length2 > 16) {
    toScroll = textLine2;
    lineToScroll = 1;
    lcd.setCursor(0, 0);
    lcd.print(textLine1);
  } else if (length1 > 16 && length2 > 16) {
    lcd.setCursor(scrollCursor, 1);
    lcd.print(textLine1.substring(stringStart,stringStop));
    lcd.setCursor(scrollCursor, 1);
    lcd.print(textLine2.substring(stringStart,stringStop));
    if (length1 > length2) {
      toScroll = textLine1;
    } else {
      toScroll = textLine2;
    }
  } else {
    lcd.setCursor(0, 0);
    lcd.print(textLine1);
    lcd.setCursor(0, 1);
    lcd.print(textLine2);
    return 1;
  }
  lcd.setCursor(scrollCursor, lineToScroll); 
  lcd.print(toScroll.substring(stringStart,stringStop));
  if (stringStart > stringStop) {
    stringStart = 0;
    stringStop = 0;
    scrollCursor = 0;
  } if(stringStart == 0 && scrollCursor > 0){
    scrollCursor--;
    stringStop++;
  } else if (stringStart == stringStop) {
    stringStart = stringStop = 0;
    scrollCursor = screenWidth;
  } else if (stringStop == toScroll.length() && scrollCursor == 0) {
    stringStart++;
  } else {
    stringStart++;
    stringStop++;
  }
  return 0;
}
