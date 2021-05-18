# NES_Emulator  
---
An amateur attempt at writing a simple emulator in C++ for the Nintendo Entertainment System (NES).  
So far, emulator presumably works for any cartridge using Mapper 0 (tested with Donkey Kong).
  
### *Building and Running*:
* Install CMake
* Download SDL2
* Run CMake and build
* Place ROM file inside of "ROM" folder in build directory
* Run "./NES_Emulator <ROM_name\>"
  
### *Controls*:

* Player 1:
    * UP: 'W'
    * LEFT: 'A'
    * DOWN: 'S'
    * RIGHT: 'D'
    * A: 'G'
    * B: 'H'
    * SELECT: 'T'
    * START: 'Y'

* Player 2:
    * UP: UP key
    * LEFT: LEFT key
    * DOWN: DOWN key
    * RIGHT: RIGHT key
    * A: numpad 2
    * B: numpad 3
    * SELECT: numpad 5
    * START: numpad 6  

* PAUSE: 'P'
  
### *TODO*:

* Implementing mappers 1-4  
* Implementing Audio  
* Testing compilation on Windows (already tested for Linux)  
  
### *Further Optimizations to Consider*:

* Multithreading (separate rendering into separate loop; although there are no problems with single-threaded performance)  
* Reimplementing CPU and PPU as JITs  
  
Shoutout to the Nesdev Wiki and r/Emudev for resources and documentation.
