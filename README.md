# NES Emulator
I grew up playing the NES so I really love this system. I learned most of what  
I know about the NES from the [nesdev wiki](https://www.nesdev.org/wiki/Nesdev_Wiki), the contributors who compiled that  
information are amazing. Special thanks to Disch [RIP] who wrote many answers  
in the nesdev forums to questions about the APU. They helped me immensely when  
trying to figure out audio, even so many years after he wrote them.  

## Dependencies
[SDL2](https://www.libsdl.org/download-2.0.php)  
Linux installation instructions [here](http://wiki.libsdl.org/Installation#linuxunix)  

# Building
## Linux
1. run `make`  
2. run `./nes <path_to_game_rom>`  

## Windows
1. Install mingw-w64  
  - You can find the specific version you want from [here](https://sourceforge.net/projects/mingw-w64/files/).  
  - The verision I used was version 8.1.0, [x86_64-posix-seh](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/8.1.0/threads-posix/seh/x86_64-8.1.0-release-posix-seh-rt_v6-rev0.7z)  
  - Add the mingw-w64 bin directory to your path environment variable, so that you can use gcc   
  - You can find more details about setting up SDL2 with mingw-w64 [here](https://www.matsson.com/prog/sdl2-mingw-w64-tutorial.php).  
2. Install SDL2  
  - Download the [mingw-w64 development libraries](https://www.libsdl.org/release/SDL2-devel-2.0.20-mingw.tar.gz)  
  - After extracting the contents, copy the directory `x86_64-w64-mingw32` to a  
    directory called `SDL2` in this project    
3. run `build`   
4. run `nes<path_to_game_rom>`   

#### References:
http://6502.org  
https://www.nesdev.org/wiki/Nesdev_Wiki  
https://masswerk.at/6502/6502_instruction_set.html  
http://www.6502.org/documents/books/mcs6500_family_hardware_manual.pdf  
http://www.6502.org/documents/books/mcs6500_family_programming_manual.pdf  
https://bisqwit.iki.fi/jutut/kuvat/programming_examples/nesemu1/  
https://www.copetti.org/writings/consoles/nes/  
http://6502.org/tutorials/interrupts.html  
https://www.youtube.com/watch?v=F8kx56OZQhg&list=PLrOv9FMX8xJHqMvSGB_9G9nZZ_4IgteYf&index=2  
https://nescartdb.com/  
**APU**  
https://www.nesdev.org/apu_ref.txt  
https://github.com/Grieverheart/sdl_tone_oscillator  
http://nicktasios.nl/posts/making-sounds-using-sdl-and-visualizing-them-on-a-simulated-oscilloscope.html  
https://forums.nesdev.org/viewtopic.php?f=3&t=13749  
https://forums.nesdev.org/viewtopic.php?f=3&t=13767  
https://www.reddit.com/r/EmuDev/comments/5gkwi5/gb_apu_sound_emulation/  

