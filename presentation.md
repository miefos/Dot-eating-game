<br />
<p align="center">
  <h3 align="center">Agario clone project</h3>
</p>


<details open="open">
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#dependencies">Dependencies</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#gameplay">Gameplay</a></li>
    <li><a href="#achievments">Our achievments</a></li>
    <li><a href="#problems">Problems and solutions</a></li>
    <li><a href="#division">Work division</a></li>
  </ol>
</details>


## Getting started with the project

In our project we decided to use RAYLIB for our client and server implementation.

Here's why:
* Raylib has a good documentation when comparing to alternatives
* Its easy to start using it as the library is well written and there are plenty of examples avialable


### Dependencies

For Raylib to work on your linux machine you have to have these packages installed
* build-essentials
* cmake
* libasound2-dev
* mesa-common-dev
* libx11-dev
* libxrandr-dev
* libxi-dev
* xorg-dev
* libgl1-mesa-dev
* libglu1-mesa-dev


## Installation

This is an example of how to list things you need to use the software and how to install them.
* build essentials
  ```sh
  sudo apt install build-essentials
  ```
* cmake
  ```sh
  sudo apt install cmake
  ```
* visual dependencies
  ```sh
  sudo apt install libasound2-dev mesa-common-dev libx11-dev libxrandr-dev libxi-dev xorg-dev libgl1-mesa-dev libglu1-mesa-dev
  ```
Installing Raylib
1. Clone the repo
   ```sh
   git clone https://github.com/raysan5/raylib.git raylib
   ```
2. Go to source directory
   ```sh
   cd raylib/src/
   ```
3. Make the library
   ```sh
   make PLATFORM=PLATFORM_DESKTOP
   ```
4. Install the library
   ```sh
   sudo make install
   ```



## Gameplay
<img src="visuals/eatingdotsgame.gif" alt="Gamemplay gif" title="Gameplay" width="500"/>


## Achievments

Beautiful gui 


## Problems
None


## Division
Work done by:
[Martins Ciekurs](LinkPlaceHolder),
[Roberts Rigacovs](https://github.com/Goodguyr)

