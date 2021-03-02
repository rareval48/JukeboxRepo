# JukeboxRepo

This is the repo for Rafael and Paul's Jukebox Project.

<br>

## Table of Contents
* [Plan Document](docs/plan.md)
* [Code Prototype](#code-prototype)
* [Code](#code)
* [CAD Design](#cad-design)
* [Assembly](#Assembly)

<br>
<br>

## Code Prototype

### Description & Code

Before going to the trouble of creating all of the menus and other user interface stuff, we created a prototype to make sure that all the components were functional. While most of the code in the prototype didn't make it to the final code sketch, the displayText() function was complete at this stage, so it was copied over. 

##### displayText()

```c++
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
```

Basically, this function displays the two lines of text on the LCD screen, scrolling them when neccesary. It does this with three variables: **stringStart**, **stringStop**, and **scrollCursor**. **stringStart** and **stringStop** are used to determine the portion of string which is displayed, becuase the function is only displaying a portion of the string beginning at the index determined by **stringStart** and ending at the index determined by **stringStop**. The movement of the text is accomplished by **scrollCursor**, so that if only the first 2 characters are being displayed they will be printed at the far right of the screen.

### Evidence

[Link to Code Prototype](/code_prototyping/code_prototyping.ino)

### Reflection

The prototype wasn't too difficult since it was just basically just copying code from examples and cobbling it together with some logic. There was a moment where I was concerned that playing music in the background wouldn't work, since I had hooked up the musicPlayer to play the song which was displayed on the LCD screen, and it wasn't playing. However. I soon realized that what my code was actually doing was trying to play the song thousands of times per second, and once I had put in some logic to only play the displayed song when I pressed a button, it worked perfectly.

<br>
<br>



## Code

### Description & Code



### Evidence



### Images



### Reflection



<br>
<br>

## CAD Design

### Description



### Evidence

[Link to CAD](https://cvilleschools.onshape.com/documents/5302f12635f173b4517b5b74/w/e0544944f1d230a23635893f/e/f6586d6b7905c4d1f824e306)

### Images



### Reflection



<br>
<br>

## Assembly

### Description



### Evidence



### Images



### Reflection
