//include header files
//include libraries for mp3 shield
//include libraries for lcd screen


//Song::Song(title, filename, genre, artist) {
	//_title = title
	//_filename = filename
	//_genre = genre
	//_artist = artist
}


//Menu::Menu(items) {
	//_items = items
	//_currentItem = _items[0]
	//define pins for LCD screens

}

//void Menu::display() {
	//do some stuff to display current menu item on lcd screen
}

//void Menu::nextItem() {
	//i = _currentItem.index

	//if i < _items.length {
		//_currentItem = _items[i + 1]
	}
	//else if i == _items.length {
		//_currentItem = _items[0]
	}
}

//void Menu::previousItem() {
	//i = _currentItem.index

	//_currentItem = _items[i - 1]
}

//GenreMenu::GenreMenu(items): Menu(items) {
	//do stuff
}

//SongMenu::SongMenu(items, genre): Menu(items){
	//define pins for mp3 shield
	//musicPlayer = musicPlayer(pins for mp3 shield)
}

//void SongMenu::playCurrentSong() {
	//MusicPlayer.playSong(_currentItem)
}

//SongMenu GenreMenu::createSongMenu(songList) {
	//SongMenu newSongMenu = SongMenu(songList, _currentItem)
	//return SongMenu
}