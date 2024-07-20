# Mimbrola

Mbrola adaptation for microcontrollers

*Please be patient... work in progress!*

## Important information

This is not fork of [Mbrola](https://github.com/numediart/MBROLA),
but completely different program, containing big parts of Mbrola code.
It's designed especially for microcontollers with small amount of RAM
and 8 MB flash (4 MB for some voices).

I assume you are familiar with Mbrola, so I won't explain what is phoneme,
details of input text format etc.

## Destination

This program (library) is provided for ESP32 and Arduino IDE, but main
part of library is written in plain C. You probably will be able to
use C library (`esprola.c` file) without changes with Espressif SDK,
and porting to other microcontrollers should be limited to replace
only some lines (reading flash content).

This library needs board with 8 minimum MB flash (like some ESP32
WROVER boards), but some voices (for example Polish) may be installed
on 4 MB board.

This library is intended to use with ESP8266Audio.

## Installing voice

Before use of this library you must download original Mbrola voice
and convert it into binary or source format used by this library.
For debian-like distribution you can simply install voice using apt:

```sudo apt install mbrola-<voice_name>```

For example: to install us1 (American female voice) use:

```sudo apt install mbrola-us1```

For other distribution see documentation of your package manager,
or download voice from https://github.com/numediart/MBROLA-voices

You must convert voice using `voice2c` tool (from [tools](tools) folder).
After then, edit `config.h` file and put correct folder name
in `_data_header(x)` definition.

For Polish (full quality and compiled-in Alaw compressed) voices download prepared
data from [mimbrola_voices_pl](https://github.com/ethanak/mimbrola_voices_pl).

## Preparing ESP32 board

This library uses raw flash partition to store voice data. Before
compilation you must prepare partition scheme, adding mbrola partition.
It must be big enough to hold file espola.blob, must be named `mbrola`,
have type `data` and subtype `0x40`. Use `mbrgenpart.py` tool from
[tools](tools) folder to create partition description file,
extra lines for your boards.txt file and example command for uploading
blob into your ESP32 board.

_to be continued..._

