all:
	g++ src/system_info_window.cpp -o system_info_window -lSDL2 -lSDL2_ttf
	g++ src/file_explorer.cpp -o file_explorer -lSDL2 -lSDL2_ttf
	g++ src/start_menu_window.cpp -o start_menu_window -lSDL2 -lSDL2_ttf -lSDL2_image
