# My_NES_Emulator
Yet Another NES Emulator (WIP)

One day this could be an emulator capable of playing a commercial NES game.

It emulates about 100% of the official opcodes of the 6052 CPU.

It implements a simple display and keyboard input like one of the virtual machines you can find [here](http://skilldrick.github.io/easy6502/). It can run the snake game you can find there, and the snake game I wrote six years ago that's here on Github.

It's also beginning to implement the NES.

## Screenshots Taken During Development
![Screenshot running my snake game](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot1.png "Emulator Screenshot")
![Screenshot running Donky Kong](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot2.png "Emulator Screenshot of Donkey Kong")
![Screenshot running Super Mario Bros](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot3.png "Emulator Screenshot of Super Mario Bros")
![Screenshot running nestest.nes](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot4.png "Emulator Screenshot of nestest.nes")
![Screenshot running Super Mario Bros with color](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot5.png "Emulator Screenshot of Super Mario Bros. with color")
![Screenshot running Donky Kong](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot6.png "Emulator Screenshot of Donkey Kong with color")

## Goals
* Should be capable of running a commercial NES game, not one of the really hard-to-emulate ones like Battletoads, but maybe one of the medium-difficulty ones like SMB. 
* Should be able to compile for and run on MyOS and Windows

## Acknowledgements
* Very much inspired by [javidx9's series](https://www.youtube.com/playlist?list=PLrOv9FMX8xJHqMvSGB_9G9nZZ_4IgteYf) of Youtube videos, but this is my own independent implementation.
* This project utilizes a modified version of [SDL_picofont](http://nurd.se/~noname/?section=sdl_picofont) by Fredrik Hultin with patches from Saul D "KrayZier" Beniquez, released under GPLv2.
* Uses [SDL 2](https://www.libsdl.org/)
