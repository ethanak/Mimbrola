# Tools for importing Mbrola voices

## voice2c

Compilation:
```
gcc -o voice2c voice2c.c
```

Usage:
```
./voice2c [-O <data_directory>] [-a|-A|-h|-H] <mbrola_voice>
```

Example:
```
./voice2c -O ~/Arduino/libraries/Mimbrola/data -a /usr/share/mbrola/pl1/pl1
```

Parameters are:

- -O <folder_name> - output data folder.
   Folder with voice files will be created here. If not given, current
   directory is assumed.
- -a - create A-law compressed data
- -A - create resampled and A-law compressed data
- -h - create A-law compressed data and put into C array
- -H - create resampled and compressed data and put into C array
- Without modifier: no compression.

A-law compressed data uses half of voice size. You can safely use this
type of data files as voice quality is only little degraded (especially
if you use internal DAC and/or small speaker). Data fits in 8MB flash,
some voices fits even in 4 MB together with application (like Polish
pl1).

Resampled data uses quarter of voice size. Quality is very low and this
type of voice is not intended for end applications, but in some cases
may be usable during development (most of voices fits in 4 MB flash).

Compiled-in voices are not very usable, but may be interested if you
work on multi-language application and you must change voice very
often.

Created folders name are combined from:
- voice name. Actually got from file name, as Mbrola database does not contain machine-readable information.
- compression type. There are: full (for not-compressed), alaw (for compressed) and low (for resampled).
- "app" i folder contains array headers (no blob file)

For example:
- pl1_alaw_app contains data for pl1 voice, A-law compressed and
  provided to compile into application
- us1_full contains data for us1_voice, not compressed.
  There are header files and big blob file provided to upload into
  mbrola partition.

## mbrgenpart

Python script for both Python2 and Python3. You can use this script
to easy create csv partition description file, extra menu position
for Arduino IDE and parameters for blob file upload via esptool.

Usage:
```
python mbrgenpart.py [parameters] <blob_file>
```

Parameters are:
-  flash=<size> - flash size in MB. Acceptable are 4, 8 and 16. Default 4.
-  ota - create partition for OTA. Default no OTA.
-  spi - create partition for spiffs.
-  fat - create partition for fatfs.
-  spi=<size> - create partition for spiffs with given size
-  fat=<size> - create partition for fatfs woth given size
-  menu=<name> - name for position in Arduino IDE menu
-  board=<name> - board name from boards.txt
-  csv=<name> - name of csv file (without path, csv extension will be added)

If you provide csv name, file with this name will be created in current
directory. Otherwise partition data will be dumped to screen.

Sizes for spi and fat are in megabytes (default) or kilobytes. For example:
- 1M means "one megabyte"
- 2 means "two megabytes"
- 100k means "100 kilobytes"

For example:
```
python mbrgenpart.py flash=8 fat menu="8 MB, American us1" board=esp32wrover csv=mbrola_8m ~/Arduino/libraries/Mimbrola/data/us1_alaw/espola.blob
```

How to use this script:

Locate esp32 installation folder. On Linux will be probably:
```
~/.arduino15/packages/esp32/hardware/esp32/<version>/
```
There is a file "boards.txt" in this folder. Open it with your favorite
text editor and find your board. For example: for ESP32 WROVER Dev board
the board name will be ```esp32wrover```.

Now look at folder tools/partitions. There will be some files with csv
extension. Choose a name for new partition scheme (must not exists!) -
for example 'mbrola_8m'

Now run ```mbrgenpart.py``` script with correct parameters:
- csv=mbrola_8m
- board=esp32wrover
- and of course rest of parameters you need.

In current directory file ```mbrola_8m.csv``` will be created. Copy
this file into tools/partitions folder in your esp32 installation
folder.

Now back to editor, and add three lines displayed on screen after last
esp32wrover.menu.PartitionScheme line.

Restart Arduino IDE if running (to see new menu position). Now you can
select new partition scheme for your board.

You can now upload blob file into your board using command similar to
displayed on screen (see documentation of ```esptool.py``` for further
parameters).

That's all.
