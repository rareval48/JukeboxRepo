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
        Song();
        void setValues(String title, const char* filename, String genre, String artist);

        String _title;
        const char* _filename;
        String _genre;
        String _artist;
    private:

};

class Menu   //keeps track of items and has methods to change the current item
{
    public:
        Menu(String* items, int numberOfItems);
        ~Menu();
        int indexInItems(String itemToFind);
        void nextItem();
        void previousItem();

        String _currentItem;
        String *_items;
        int _numberOfItems;
    private:

};

class SongMenu     //does the same thing as Menu but with Song objects
{
    public:
        SongMenu(Song* items, int numberOfItems, String genre);
        ~SongMenu();
        int indexInItems(Song itemToFind);
        void nextItem();
        void previousItem();

        Song _currentItem;
        Song *_items;
        String _genre;
        int _numberOfItems;
    private:

};

class QueueMenu: public SongMenu      //stores the upcoming queue of songs
{
    public:
        QueueMenu(Song* items, int numberOfItems);
        ~QueueMenu();
        void playNextSong(Adafruit_VS1053_FilePlayer musicPlayer);

        Song _currentItem;
        Song *_items;
        int _numberOfItems;
    private:

};
 #endif