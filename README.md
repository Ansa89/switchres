# Switchres
Modeline generator engine and [MAME](https://github.com/mamedev/mame) resolution switcher.

See output of `switchres -h` for best information about command line settings.

Basic config file example is in [extra/switchres.conf](extra/switchres.conf).


### Simple modeline generation
`switchres --calc pacman`
`switchres 384 288 60.61 --monitor d9800`


### Running games in MAME
`switchres pacman --monitor cga`
`switchres frogger --ff --redraw --nodoublescan --monitor d9200`

##### ArcadeVGA in Windows 
`switchres tron --resfile extra/ArcadeVGA.txt --resfile ~/modes.txt`

##### Soft15Khz in Windows (reading registry modelines)
`switchres tron --soft15khz`


### Running games with other emulators
`switchres a2600 --emulator mess --rom /path/to/ROM\ File.bin --args -verbose -sdlvideofps`

The game rom becomes the game system and anything after `--args` is passed to the emulator (in this case "mess").

NOTE: even when using other emulators, MESS is still used to get the games correct resolution.
