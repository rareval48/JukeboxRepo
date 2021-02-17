//Compatible with the Arduino IDE 1.0
//Library version:1.1
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

//void setup()
//{
//  Serial.begin(9600);
//  lcd.init();                      // initialize the lcd
  // Print a message to the LCD
//  lcd.backlight();
//  lcd.setCursor(5, 0);
//  lcd.setCursor(5, 1);
//}


int screenWidth = 16;
int screenHeight = 2;

// the two lines
// line1 = scrolling
String textLine1 = "Hallowed Be Thy Name";
// line2 = static
String textLine2 = "Iron Maiden";

// just some reference flags
int stringStart, stringStop = 0;
int scrollCursor = screenWidth;


int counter = 0;

void setup() {
  lcd.init();                      // initialize the lcd
  lcd.backlight();
  Serial.begin(9600);
}

void loop() {
  if (counter == 20) {
    newTextDisplay("Master of Puppets (Remastered)", "Metallica");
  } else if (counter == 40) {
    newTextDisplay("I Don't Wanna Be Me", "Type O Negative");
  } else if (counter == 60) {
    newTextDisplay("Hallowed Be Thy Name", "Iron Maiden");
    counter = 0;
  }
  counter++;
  //Serial.println(counter);
  Serial.print(stringStart);
  Serial.print(" and ");
  Serial.println(stringStop);
  Serial.println(scrollCursor);
  
  displayText();
}

void newTextDisplay(String line1, String line2) {
  //reset scrolling variables
  stringStart = 0;
  stringStop = 0;
  scrollCursor = screenWidth;
  
  //set text variables
  textLine1 = line1;
  textLine2 = line2;
}

void displayText() {
  lcd.setCursor(scrollCursor, 0);
  if (textLine1.length() > 16)
  lcd.print(textLine1.substring(stringStart,stringStop));
  lcd.setCursor(0, 1);
  lcd.print(textLine2);
  delay(500);
  lcd.clear();
  if (stringStart > stringStop) {
    Serial.println("stringStart > stringStop");
    stringStart = 0;
    stringStop = 0;
    scrollCursor = screenWidth;
  } if(stringStart == 0 && scrollCursor > 0){
    Serial.println("stringStart == 0 && scrollCursor > 0");
    scrollCursor--;
    stringStop++;
  } else if (stringStart == stringStop) {
    stringStart = stringStop = 0;
    scrollCursor = screenWidth;
  } else if (stringStop == textLine1.length() && scrollCursor == 0) {
    stringStart++;
  } else {
    stringStart++;
    stringStop++;
  }

}
