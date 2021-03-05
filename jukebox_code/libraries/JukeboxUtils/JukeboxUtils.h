/*
    JukeboxUtils.h - Library which includes classes pertinent to Paul and Rafael's Jukebox.
    Created by Paul Schakel, Feb 2021
*/

#ifndef JukeboxUtils_h
#define JukeboxUtils_h

#include "Arduino.h"
#include <Adafruit_VS1053.h>


class Song      //keeps track of the information about a song
{
    public:
        Song(const String& title = "", const char* filename = "", const String& genre = "", const String& artist = "");   //had to set defaults so that lists of songs wouldn't break
        String _title;
        const char* _filename;
        String _genre;
        String _artist;
    private:

};

class Menu   //keeps track of items and has methods to change the current item
{
    public:
        Menu(String* items);
        ~Menu();
        int indexInItems(String& itemToFind);
        void nextItem();
        void previousItem();

        String _currentItem;
        String _items[8];      //sets the maximum number of items in a Menu
    private:

};

class SongMenu     //does the same thing as Menu but with Song objects
{
    public:
        SongMenu(Song* items);
        ~SongMenu();
        int indexInItems(Song& itemToFind);
        void nextItem();
        void previousItem();

        Song _currentItem;
        Song _items[6];     //sets the maximum number of songs which can be in a SongMenu
        String _genre;
    private:

};

class QueueMenu      //stores the upcoming queue of songs
{
    public:
        QueueMenu(Song* items);
        ~QueueMenu();
        int indexInItems(Song& itemToFind);
        void playNextSong(Adafruit_VS1053_FilePlayer* musicPlayer);

        Song _currentItem;
        Song _items[6];    //sets the maximum amount of songs which can be in the queue
    private:

};
 #endif