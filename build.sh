#!/bin/bash

dpkg-query -S libsdl2-dev >> /dev/null 2>&1
if [ $? -eq 1 ]
then
    echo "no sdl dev :( go install it";
    exit
fi

dpkg-query -S libsdl2-ttf-dev >> /dev/null 2>&1
if [ $? -eq 1 ]
then
    echo "no sdlttf go install it";
    exit
fi

# libsdl2-dev
gcc mud.c -lSDL2 -lSDL2_ttf -o mud
