/*
    JukeboxUtils.cpp - Library which includes classes pertinent to Paul and Rafael's Jukebox.
    Created by Paul Schakel, Feb 2021
*/


#include "Arduino.h"
#include "JukeboxUtils.h"
#include <Adafruit_VS1053.h>


Song::Song() {
  //this had to be a 0 argument constructor becuase otherwise lists of Songs would break
}

void Song::setValues(String title, const char* filename, String genre, String artist) {   //this is the equivalent of a constructor for the Song class
  _title = title;
  _filename = filename;
  _genre = genre;
  _artist = artist;
}


Menu::Menu(String items[], int numberOfItems) {
    memcpy(_items, items, sizeof(items[0]) * numberOfItems);  //copies the array passed as an argument to the array stored within the class
    _currentItem = _items[0];
    _numberOfItems = numberOfItems;
}

Menu::~Menu() { delete[] _items;}   //destructor for the Menu class

int Menu::indexInItems(String itemToFind) {   //finds the index of an item within an array
    int i  = 0;
    while (i < sizeof(_items)) {
        if (_items[i] == itemToFind) {
            break;
        }
        i++;
    }

    if (i < sizeof(_items)) {
        return i;
    } else {
        return 0;   //if this else clause is ever triggered I'll eat my hat
    }
}

void Menu::nextItem() {   //sets the _currentItem to the next item in the array
    int currentIndex = indexInItems(_currentItem);
    if (currentIndex + 1 >= sizeof(_items)) {
        _currentItem = _items[0];
    } else {
        _currentItem = _items[currentIndex + 1];
    }
}

void Menu::previousItem() {   //sets the _currentItem to the next item in the array
    int currentIndex = indexInItems(_currentItem);
    if (currentIndex == 0) {
        _currentItem = sizeof(_items) - 1;
    } else {
        _currentItem = _items[currentIndex - 1];
    }
}


SongMenu::SongMenu(Song items[], int numberOfItems, String genre) {
    memcpy(_items, items, sizeof(items[0]) * numberOfItems);    //copies the array passed as an argument to the array stored within the class
    _currentItem = items[0];
    _genre = genre;
    _numberOfItems = numberOfItems;
}

SongMenu::~SongMenu() { delete[] _items;}     //destructor for the SongMenu class

int SongMenu::indexInItems(Song itemToFind) {     //finds the index of an item within an array - its the same as the one in Menu but it works with Song objects
    int i  = 0;
    while (i < sizeof(_items)) {
        if (_items[i]._title == itemToFind._title) {
            break;
        }
        i++;
    }

    if (i < sizeof(_items)) {
        return i;
    } else {
        return 0;   //if this else clause is ever triggered I'll eat my hat
    }
}

void SongMenu::nextItem() {   //sets the _currentItem to the next item in the array
    int currentIndex = indexInItems(_currentItem);
    if (currentIndex + 1 >= sizeof(_items)) {
        _currentItem = _items[0];
    } else {
        _currentItem = _items[currentIndex + 1];
    }
}

void SongMenu::previousItem() {   //sets the _currentItem to the previous item in the array
    int currentIndex = indexInItems(_currentItem);
    if (currentIndex == 0) {
        _currentItem = _items[sizeof(_items) - 1];
    } else {
        _currentItem = _items[currentIndex - 1];
    }
}


QueueMenu::QueueMenu(Song items[], int numberOfItems): SongMenu(items, numberOfItems, "testGenre") {
    memcpy(_items, items, sizeof(items[0]) * numberOfItems);    //copies the array passed as an argument to the array stored within the class
    _currentItem = _items[0];
    _numberOfItems = numberOfItems;
}

QueueMenu::~QueueMenu() { delete[] _items;}   //destructor for the QueueMenu class

void QueueMenu::playNextSong(Adafruit_VS1053_FilePlayer musicPlayer) {    //plays the next song in the Queue
    int currentIndex = indexInItems(_currentItem);
    if (currentIndex + 1 >= sizeof(_items)) {
        _currentItem = _items[0];
    } else {
        _currentItem = _items[currentIndex + 1];

    musicPlayer.startPlayingFile(_currentItem._filename);
    }
}
