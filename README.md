# JukeboxRepo

This is the repo for Rafael and Paul's Jukebox Project.

<br>

## Table of Contents
* [Overview](#overview)
* [Design](#design)
* [Code Prototype](#code-prototype---paul)
* [Code](#code---paul)
* [CAD Design](#cad-design---rafael)
* [Assembly](#Assembly)

<br>
<br>

## Overview

Having completed the preliminary CAD and code assignments for Engineering II, the next assignment was to create a **Sensor Robot**. This robot had to be a device which would use some kind of sensor as an input, and use some kind of output which would be affected by the input. There was a certain amount of flexibility with the requirements, so the sensor could just be sensing an input from the user.

For our Sensor Robot, we (Rafael Arevalo and Paul Schakel) decided to create a jukebox. When completed, this jukebox would let the user select songs and add them to a queue which would play them one by one. For more details on our original design and timeline visit our [planning document](docs/plan.md).

<br>

### Tools used

- CAD - [Onshape](https://www.onshape.com/en/)

- Code - [Ardiuno IDE](https://www.arduino.cc/en/software) and [VS Code](https://code.visualstudio.com/)

- Wiring Diagram - [Fritzing](https://fritzing.org/)

- Image Editing - [GIMP](https://www.gimp.org/)

- Minor Video Editing - [Kdenlive](https://kdenlive.org/en/)

<br>
<br>

## Design

Our final product differed greatly from the original concept we came up with, as we completely reworked the user interface and decided to leave some features out. The original design was going to use four buttons for the user to navigate the menus, but at the suggestion of Mr. Helmstetter, we swapped out three of the buttons for one rotary encoder. The rotary encoder replaced the **next**, **previous**, and **select** buttons with the rotation of the knob and the button within the encoder. 

<img src="/images/design_comparison.png" height=360px alt="Comparison of original and final design">

Additionally, our original design was going to have a coin slot which the user would use to pay for songs. However, this feature proved too complicated and after one round of attempted prototyping we scrapped the idea.

<br>
<br>

## Code Prototype - Paul

Before going to the trouble of creating all of the menus and other user interface features, we decided to create a prototype to make sure that all the components were functional. While most of the code in the prototype didn't make it to the final code sketch, the displayText() function was complete at this stage, so it was copied over. 

#### displayText Function

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

Basically, this function displays the two lines of text on the LCD screen, scrolling them when neccesary. It does this with three variables: **stringStart**, **stringStop**, and **scrollCursor**. **stringStart** and **stringStop** are used to determine the portion of string which is displayed, because the function is only displaying a portion of the string beginning at the index determined by **stringStart** and ending at the index determined by **stringStop**. The movement of the text is accomplished by **scrollCursor**, so that if only the first 2 characters are being displayed they will be printed at the far right of the screen.

### Links and Images

[Link to Code Prototype](/code_prototyping/code_prototyping.ino)

[![Short demo video of jukebox prototype](http://img.youtube.com/vi/Xu1ld7YbddA/0.jpg)](http://www.youtube.com/watch?v=Xu1ld7YbddA "Short demo video of jukebox prototype")

### Reflection

The prototype wasn't too difficult since it was just basically just copying code from examples and cobbling it together with some logic. There was a moment where I was concerned that playing music in the background wouldn't work, since I had hooked up the musicPlayer to play the song which was displayed on the LCD screen, and it wasn't playing. However. I soon realized that what my code was actually doing was trying to play the song thousands of times per second, and once I had put in some logic to only play the displayed song when I pressed a button, it worked perfectly.

<br>
<br>

## Code - Paul

The code for this project was rather complex, and the plan which I had created in the [pseudocode](/pseudocode) turned out to be far more complicated than it needed to be. Originally, there were going to be several classes stored within a library called JukeboxUtils. However, once I started running out of SRAM halfway through the making of the code, I realized that I needed to cut some corners to make the program fit on the Arduino. After lots of missteps and malfunctions, I settled on a design which would, instead of using classes, just make each menu a collection of variables tied together within the main loop. I did create one [struct](https://en.wikipedia.org/wiki/Struct_(C_programming_language)) which would contain the information about each song, but all of the other classes were removed.

### Item Storage

In order to keep SRAM usage below critical levels (usually around 80%) I had to use a variety of different approaches to contain the items in each menu. For the genre menu and the main menu, I stored the value of each item (which were char arrays) in program memory, and created an array of those items which was also stored in program memory. To access the items, I stored the current item in a char array stored in SRAM. 

For the song menu, I took a different approach, creating an array of Song structs for each genre, and then using a pointer to access the values in each array. Similarly to my approach for accessing items in the genre and main menus, I used a Song struct stored in SRAM to contain the current item. 

Unfortunately, the approach I used for the song menu was impossible to use again for the queue and the queue menu because the array of songs in the queue had to be able to contain any song which the user selects, in any arrangement. In order to save SRAM, I stored the items in the queue as an array of pointers to Songs. This approach solved the problem nicely, and I could also use the current item approach I used for the song menu in the queue menu.

### Songs

The songs on the jukebox are organized by genre, and then sorted alphabetically by the artist's name within each genre. A list of all of the songs we added can be found [here](/docs/songs.md)

### Links and Images

[Link to Code](/jukebox_code/jukebox_code.ino)

<img src="/images/jukebox_wiring.png" height=360px>

Wiring diagram for the Jukebox

### Reflection

This was the most complex project I've done in engineering, code wise, and as such I met with a significant number of difficulties while working on it. The greatest difficulty I came across was keeping the program within the tiny constraints of the SRAM of the Arduino. I had to take many steps to optimize the program for low memory usage, which include making functions pass arguments [by reference](https://www.learncpp.com/cpp-tutorial/passing-arguments-by-reference/), using [pointers](https://www.youtube.com/watch?v=rtgwvkaYt1A) to point to data rather than copying the data, using char arrays instead of Strings to reduce heap fragmentation, and using [program memory](https://www.arduino.cc/reference/en/language/variables/utilities/progmem/) to store large pieces of data. Adafruit has a [useful guide](https://learn.adafruit.com/memories-of-an-arduino/optimizing-sram) which details these concepts.

Overall, this project has been very good for my development as a programmer since I got the opportunity to learn a lot more about C++. Towards the end of the project I really began to appreciate the control that C++ gives to the programmer especially with memory allocation. I usually write code which runs on modern systems with plenty of resources, and programming a microcontroller with such limiting constraints forced me to think outside the box. 

<br>
<br>

## CAD Design - Rafael



### Links and Images

[Link to CAD](https://cvilleschools.onshape.com/documents/5302f12635f173b4517b5b74/w/e0544944f1d230a23635893f/e/f6586d6b7905c4d1f824e306)



### Reflection



<br>
<br>

## Assembly



### Links and Images



### Reflection
