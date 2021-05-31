# Battery level indicator

The program simulates the charge indicator of a Li-Pol battery. After
entering the voltage value in the terminal, the charge level will be
spoken.

The program recognizes additional commands:

- `\c value` - set contrast (0 to 100)
- `\v value` - set Mbrola volume (0.5 to 2)
- `\s value` - set speed (0.5 to 2)
- `\p value` - set pitch (0.5 to 2)
- `\?` - display values

To create speech in your language, use `genpho.py` script. You must
edit the script, entering voice name (for Mbrola) and translation of
four phrases. Actually script contains translations for English, Polish
and Spanish.

Script dumps phonetic translations to file `lang_<language>.h`, where `<language>`
is two first letters of `lang` value.

You need espeak in your execution path to run this script!

As espeak is not perfect source for Mbrola phonemes, you can experiment
with native text to phoneme translator, f.ex. Milena for Polish or
txt2pho for German.

Sometimes it will be necessary to edit the file. For example: for English (both British and American)
word "empty" is translated incorrectly by current version of espeak, phoneme "p" after "m"
is too long (195 msec, corrected manually to 55 msec).

