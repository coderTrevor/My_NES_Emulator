# My_NES_Emulator
Yet Another NES Emulator (WIP)

This is a functional NES emulator capable of playing Super Mario Bros. to completion.

In addition to SMB, it can also play Donkey Kong, and probably some other games. It still won't run the majority of the NES library however; it was never intended to be a fully-featured emulator.

It emulates 100% of the official opcodes of the 6052 CPU.

It began as an emulator of the simple 6502 system like the virtual machine you can find [here](http://skilldrick.github.io/easy6502/). It can run the snake game you can find there, and the snake game I wrote six years ago that's here on Github.

Sound support is still a work-in-progress.

## Screenshots Taken During Development
![Screenshot running my snake game](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot1.png "Emulator Screenshot")
![Screenshot running Donky Kong](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot2.png "Emulator Screenshot of Donkey Kong")
![Screenshot running Super Mario Bros](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot3.png "Emulator Screenshot of Super Mario Bros")
![Screenshot running nestest.nes](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot4.png "Emulator Screenshot of nestest.nes")
![Screenshot running Super Mario Bros with color](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot5.png "Emulator Screenshot of Super Mario Bros. with color")
![Screenshot running Donky Kong](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot6.png "Emulator Screenshot of Donkey Kong with color")
![Screenshot of me playing Donky Kong](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot7.png "Emulator Screenshot of Donkey Kong with sprites")
![Screenshot running Super Mario Bros with proper title screen](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot8.png "Emulator Screenshot of Super Mario Bros. with proper title screen")
![Screenshot running Super Mario Bros with Mario](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot9.png "Emulator Screenshot of Super Mario Bros. with Mario")
![Screenshot running Super Mario Bros with graphical errors](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot10.png "Emulator Screenshot of Super Mario Bros. with graphical errors")
![Screenshot running Super Mario Bros with graphical errors](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot11.png "Emulator Screenshot of Super Mario Bros. with graphical errors")
![Screenshot running Super Mario Bros with graphical errors](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot12.png "Emulator Screenshot of Super Mario Bros. with graphical errors")
![Screenshot running Super Mario Bros with graphical errors](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot13.png "Emulator Screenshot of Super Mario Bros. with graphical errors")
![Screenshot running Super Mario Bros with graphical errors](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot14.png "Emulator Screenshot of Super Mario Bros. with graphical errors")
![Screenshot running Super Mario Bros with graphical errors](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot15.png "Emulator Screenshot of Super Mario Bros. with graphical errors")
![Screenshot running Super Mario Bros with graphical errors](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot16.png "Emulator Screenshot of Super Mario Bros. with graphical errors")
![Screenshot running Super Mario Bros with no graphical errors](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot17.png "Emulator Screenshot of Super Mario Bros. with no graphical errors")
![Screenshot running Super Mario Bros played to completion](https://github.com/coderTrevor/My_NES_Emulator/blob/master/Screenshots/Screenshot18.png "Emulator Screenshot of Super Mario Bros. played to completion")

## Goals
- [x] Should be capable of running a commercial NES game, not one of the really hard-to-emulate ones like Battletoads, but maybe one of the medium-difficulty ones like SMB. 
- [ ] Should be able to compile for and run on MyOS and Windows

## Acknowledgements
* Very much inspired by [javidx9's series](https://www.youtube.com/playlist?list=PLrOv9FMX8xJHqMvSGB_9G9nZZ_4IgteYf) of Youtube videos, but this is my own independent implementation.
* This project utilizes a modified version of [SDL_picofont](http://nurd.se/~noname/?section=sdl_picofont) by Fredrik Hultin with patches from Saul D "KrayZier" Beniquez, released under GPLv2.
* Uses [SDL 2](https://www.libsdl.org/)
* Would not have been possible without information from [NESDev.com](http://wiki.nesdev.com/)