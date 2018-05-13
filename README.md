# arduino-puzzles
This repository contains documentation and schematics for puzzles in The Locked Room, a real-life escape room built into a shipping container.


## Room Wiring:
The following diagram shows how each of the puzzles are wired together in the room.
A [PDF version](https://cdn.rawgit.com/MarkHedleyJones/arduino-puzzles/be08f4e4/images/layout/full-layout.pdf) of the diagram is also available.
Lengths of CAT5 cable are used to link puzzles the puzzles to the main controller in a star configuration.
The communication bus between the main controller and each puzzle is I2C.

![Thing](https://cdn.rawgit.com/MarkHedleyJones/arduino-puzzles/master/images/layout/full-layout.svg)

## Puzzles

### ClosedCircuitPhones
This is the phone that is used to call the front desk to ask for clues etc.
There is no electronics, the phones are just wired as shown.
![ClosedCircuitPhones](https://cdn.rawgit.com/MarkHedleyJones/arduino-puzzles/master/images/schematics/ClosedCircuitPhones.png)

### Puzzle_CupboardSensor
This device may have been merged with another puzzle in the room.
![Puzzle_CupboardSensor](https://cdn.rawgit.com/MarkHedleyJones/arduino-puzzles/master/images/schematics/Puzzle_CupboardSensor.png)

### Puzzle_DoorController
This device controls the state of the LEDs around the door.
![Puzzle_DoorController](https://cdn.rawgit.com/MarkHedleyJones/arduino-puzzles/master/images/schematics/Puzzle_DoorController.png)
### Puzzle_LockerSensors
![Puzzle_LockerSensors](https://cdn.rawgit.com/MarkHedleyJones/arduino-puzzles/master/images/schematics/Puzzle_LockerSensors.png)
### Puzzle_MorseCodeInterpreter
![Puzzle_MorseCodeInterpreter](https://cdn.rawgit.com/MarkHedleyJones/arduino-puzzles/master/images/schematics/Puzzle_MorseCodeInterpreter.png)
### Puzzle_MorseCodePhone
![Puzzle_MorseCodePhone](https://cdn.rawgit.com/MarkHedleyJones/arduino-puzzles/master/images/schematics/Puzzle_MorseCodePhones.png)
### Puzzle_TelephoneExchange
![Puzzle_TelephoneExchange](https://cdn.rawgit.com/MarkHedleyJones/arduino-puzzles/master/images/schematics/Puzzle_TelephoneExchange.png)
### Puzzle_WallSwitches
![Puzzle_WallSwitches](https://cdn.rawgit.com/MarkHedleyJones/arduino-puzzles/master/images/schematics/Puzzle_WallSwitches.png)
