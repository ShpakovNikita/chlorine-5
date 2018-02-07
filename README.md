# Chlorine-5

This game was created by me in a study purposes, using openGl es 2.0.

![](http://i102.fastpic.ru/big/2018/0207/8d/bd83329c70b5f668dd8e44725ce32b8d.jpg)

## How to play

You have to dowload everything in bin/ , extract folders from the res/ to the bin/ and run the .exe.

### Source code

To build everything on eclipse c/c++, just follow the instructions in src/ folder.

## About the CHL engine

This light engine was created by Shpakov Nikita with the learning purpose. CHL engine is based on the SDL2 library, and also uses grew, truetype and openAL for different purposes. It can render instances by the layers (using z buffer), play animations, calculate and render lights, draw your own GUI based on the textures, render text with the given ttf font, size and color, play sounds dynamically using 4 distance gain models, check fast and slow (by points, need for collisions with rotated instances) collisions, can produce raycast, calculate some math tasks and rendrer scene inside the camera. But still, this engine is quite unoptimized and have some problems with the memory usage.

## Built With

* [SDL2](https://www.libsdl.org/) - Main core lib for the CHL engine
* [OpenAl](https://www.openal.org/downloads/) - Advanced 3d audio system
* [OpenAl](http://glew.sourceforge.net/) - Loading openGL functions
* [Truetype](https://www.freetype.org/) - Used to decrypt fonts.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details