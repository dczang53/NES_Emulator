# NES_Emulator  
---
An amateur attempt at writing a simple emulator in C++ for the Nintendo Entertainment System (NES).  
  
### *Building and Running*:
* Install CMake
* Download SDL2
* Run CMake and build
* Run "./NES_Emulator <ROM_path\>"
  
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
  

### *Tested Games*:
* Castlevania
* Dragon Quest
* Donkey Kong
* Donkey Kong 3
* Duck Hunt
* DuckTales
* Ice Climber
* Kirby's Adventure
* Legend of Zelda
* Mega Man
* Pac-Man
* Super Mario Bros
* Super Mario Bros 3
* Tetris

### *TODO*:

* Adding savestates
* Testing compilation on Mac (already tested for Linux and Windows MinGW)  
  
### *Further Optimizations to Consider*:

* Reimplementing CPU and PPU as JITs  
  
Shoutout to the Nesdev Wiki and r/Emudev for resources and documentation.
