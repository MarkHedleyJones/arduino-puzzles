# arduino-puzzles
This repository contains documentation and schematics for puzzles in The Locked Room, a real-life escape room built into a shipping container.


## Room Wiring:
The following diagram shows how each of the puzzles are wired together in the room.
A [PDF version](https://cdn.rawgit.com/MarkHedleyJones/arduino-puzzles/be08f4e4/images/layout/full-layout.pdf) of the diagram is also available.
Lengths of CAT5 cable are used to link puzzles the puzzles to the main controller in a star configuration.
The communication bus between the main controller and each puzzle is I2C.

![Thing](https://cdn.rawgit.com/MarkHedleyJones/arduino-puzzles/master/images/layout/full-layout.svg)
