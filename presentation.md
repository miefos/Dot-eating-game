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
   make PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=SHARED # To make the dynamic shared version. - Recommended
   ```
4. Install the library to the standard directories, usr/local/lib and /usr/local/include
    ```sh
    sudo make install RAYLIB_LIBTYPE=SHARED # Dynamic shared version.
    ```
5. Install the library
   ```sh
   sudo make install
   ```
   
The simplest possible build command
    ```sh
    cc main.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    ```

For a more detailed installation you can check: https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux


## Gameplay
<img src="visuals/eatingdotsgame.gif" alt="Gamemplay gif" title="Gameplay" width="500"/>


## Division
The first and second part of the project we created seperately (basic networking, prototype).
For the final game we combined best of our previous work to achieve the best result. 

Work done by:
[Martins Ciekurs](https://github.com/miefos),
[Roberts Rigacovs](https://github.com/Goodguyr)


## Problems
1) It was hard to manage the workflow, combining our individually created servers and clients. 
2) unsigned char instead of unsigned int caused many problems and was hard to find the cause of problems.
3) Many more.


## Achievments
1) Beautiful gui; 
2) working game;
3) a lot learned during process.


