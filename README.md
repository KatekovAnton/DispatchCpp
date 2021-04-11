# DispatchCpp

## Overview

DispatchCpp is a small library (not a library but code can be easily extracted) that simplifies multithreading for c++ projects.

Tested on: windows, macOS

## Features

- concept of dedicated `main thread` for certan types of applications such as game engines (for dedicated render thread)
- submit a single task to a background thread as an operation
- submit a single task to a main thread as an operation
- submit a number of tasks to a background thread as a group operation
- completion handler for a group operation
- tracking states for both operation and group operation
- cancel operation, cancel group operation
- parallel loops - lock "current" thread and wait for number of tasks to finish
- nested parallel loops with a shared thread pool
- running not more than a reasonable amount of threads (based on `std::thread::hardware_concurrency`) (maybe a bit more sometimes)
- no external dependancies except std
- c++ 11

## Project structure

### Folders:

- **platform** - platform-specific implementations 
- **src** - main source code

### Key components:

- **Dispatch** - main interface
- **DispatchPrivate** - private interfaces, such as initialization
- **DispatchQueue** - task queues
- **Thread** - wrapper for `std::thread`

## More features to add

- Unit tests - tell me how to control the internals? inject a logic?
- Correct library setup with CMake
- Clean unused queues
