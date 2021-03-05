/*
    JukeboxUtils.cpp - Library which includes classes pertinent to Paul and Rafael's Jukebox.
    Created by Paul Schakel, Feb 2021
*/


#include "Arduino.h"
#include "JukeboxUtils.h"
#include <Adafruit_VS1053.h>


Song::Song(const String& title = "", const char* filename = "", const String& genre = "", const String& artist = "") {   //had to set defaults so that lists of songs wouldn't break
  _title = title;
  _filename = filename;
  _genre = genre;
  _artist = artist;
}


Menu::Menu(String items[]) {
    memcpy(_items, items, sizeof _items);  //copies the array passed as an argument to the array stored within the class
    _currentItem = _items[0];
}

Menu::~Menu() { delete[] _items;}   //destructor for the Menu class

int Menu::indexInItems(String& itemToFind) {   //finds the index of an item within an array
    int i  = 0;
    int itemLen = sizeof(_items)/sizeof(_items[0]);
    while (i < itemLen) {
        if (_items[i] == itemToFind) {
            break;
        }
        i++;
    }

    if (i < itemLen) {
        return i;
    } else {
        return 0;   //if this else clause is ever triggered I'll eat my hat
    }
}

void Menu::nextItem() {   //sets the _currentItem to the next item in the array
    int itemLen = sizeof(_items)/sizeof(_items[0]);
    int currentIndex = indexInItems(_currentItem);
    if (currentIndex + 1 >= itemLen) {
        _currentItem = _items[0];
    } else {
        _currentItem = _items[currentIndex + 1];
    }
}

void Menu::previousItem() {   //sets the _currentItem to the next item in the array
    int itemLen = sizeof(_items)/sizeof(_items[0]);
    int currentIndex = indexInItems(_currentItem);
    if (currentIndex == 0) {
        _currentItem = itemLen - 1;
    } else {
        _currentItem = _items[currentIndex - 1];
    }
}


SongMenu::SongMenu(Song items[]) {
    memcpy(_items, items, sizeof _items);    //copies the array passed as an argument to the array stored within the class
    _currentItem = items[0];
}

SongMenu::~SongMenu() { delete[] _items;}     //destructor for the SongMenu class

int SongMenu::indexInItems(Song& itemToFind) {     //finds the index of an item within an array - its the same as the one in Menu but it works with Song objects
    int i  = 0;
    int itemLen = sizeof(_items)/sizeof(_items[0]);
    while (i < itemLen) {
        if (_items[i]._title == itemToFind._title) {
            break;
        }
        i++;
    }

    if (i < itemLen) {
        return i;
    } else {
        return 0;   //if this else clause is ever triggered I'll eat my hat
    }
}

void SongMenu::nextItem() {   //sets the _currentItem to the next item in the array
    int currentIndex = indexInItems(_currentItem);
    int itemLen = sizeof(_items)/sizeof(_items[0]);
    if (currentIndex + 1 >= itemLen) {
        _currentItem = _items[0];
    } else {
        _currentItem = _items[currentIndex + 1];
    }
}

void SongMenu::previousItem() {   //sets the _currentItem to the previous item in the array
    int currentIndex = indexInItems(_currentItem);
    int itemLen = sizeof(_items)/sizeof(_items[0]);
    if (currentIndex == 0) {
        _currentItem = _items[itemLen - 1];
    } else {
        _currentItem = _items[currentIndex - 1];
    }
}


QueueMenu::QueueMenu(Song items[]) {
    memcpy(_items, items, sizeof _items);    //copies the array passed as an argument to the array stored within the class
    _currentItem = _items[0];
}

QueueMenu::~QueueMenu() { delete[] _items;}   //destructor for the QueueMenu class

void QueueMenu::playNextSong(Adafruit_VS1053_FilePlayer* musicPlayer) {    //plays the next song in the Queue
    int currentIndex = indexInItems(_currentItem);
    int itemLen = sizeof(_items)/sizeof(_items[0]);
    if (currentIndex + 1 >= itemLen) {
        _currentItem = _items[0];
    } else {
        _currentItem = _items[currentIndex + 1];

    musicPlayer->startPlayingFile(_currentItem._filename);
    }
}

