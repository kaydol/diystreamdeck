# DIY Stream Deck
__DIY Stream Deck alternative built with Arduino DUE and 4" MAR4018 NT35510 TFT Touch screen__

In this DIY project I had to write a somewhat complicated program in order for the device to act as a Stream Deck.

![GitHub Image](/README/result.jpg)

The main problem of this project was that the __lcdwiki__ library provided by the MAR4018 screen manufacturer was not intended to work with Arduino DUE, but rather with Arduino Mega. Hence why none of the examples worked with Due. Despite that, Mega and Due share the same pin configuration which allows the screen to be plugged directly into the Due board, exactly same way as it works with the Mega.

There were additional difficulties involving the Arduino Keyboard library which required a very specific usage to work in my case, poorly soldered ports on my Due, as well as my Due board being the R3 edition which lacks the neccesary hardware upgrade that allows it to boot correctly when connected to the Native USB port - the issue that has been plaguing Arduino Due boards since at least 2014.

## Changes to the MAR4018 lcdwiki libraries
The specific modifications done to the libraries provided by the screen manufacturer are not meant to be non-destructive and likely break the compatibility of the library with other boards that they were actually intended to work with (e.g. Arduino Mega). I only have Arduino Due, so I am not able to verify how the changes might have affected other boards. 

Most of the changes are derived from [the findings](https://forum.arduino.cc/t/arduino-due-and-nt35510-tft-screen/1131769/3) of __hml_2__ [and conclusions](https://forum.arduino.cc/t/mega-tft-shield-used-on-arduino-due/1134353/9) of __ZinggJM__. Many thanks to these brilliant minds!

The original lcdwiki libraries can be found on the [manufacturer's page](http://www.lcdwiki.com/4.0inch_Arduino_Display-Mega2560_NT35510). I've also included both the original and the modified versions of the libraries to this repository. If interested, you are welcome to do the diff on both versions to see what changes were made.

## Fixing the Arduino Due quirks
In order to emulate the keyboard, Arduino Due must be connected using the Native USB port (as opposed to the Programming USB port). When connected to the Native USB port, my board required a manual press of the RESET button to boot the loaded program. 

You shouldn't have this issue if your board is of R3-E or of later revisions. If you don't observe this behavior on your board, well, less headache for you. In short, there are 2 good ways to fix this issue. 

One way is to solder a 10kOhm (R99) resistor to the mosfet next to the 6-pin SPI connector (this solution seems to be derived from the R3-E revision of the board where they added a said resistor in there). Thanks to __dancombine__ from the Arduino forum for [this solution](https://forum.arduino.cc/t/due-wont-start-after-power-off-on-have-to-reset/247763/44). This is the way I did it and it works really well.

![GitHub Image](/README/before_soldering.jpeg)

Example 1: 

![GitHub Image](/README/after_soldering.jpeg)

Example 2:

![GitHub Image](/README/after_soldering_dip.jpeg)

The other way is to flash the 16U2 chip with a new BIOS that accounts for the lack of R99 resistor on R3. Refer to the beforementioned link to know how.

Instead of these 2 good solutions you could also get away with soldering a capacitor together with a resistor to one of the RESET pins, but it's not a reliable solution as the nominal of the capacitor can change from board to board and depends on whatever additional devices and peripherals are plugged in, so DON'T do that.

## Reading pictures from the SD card and the bottleneck
Despite being relatively powerful, the 32bit Arduino Due "only" has around 500 KB of memory (which is a lot, BUT not quite enough to store uncompressed BMP pictures in it).

So the idea was to store pictures on the SD card and read them using the MAR4018 built-in card reader. In practice, the size of the BMP pictures that I am going to use are approx 124 x 124 pixels, which take approx 50 KB each. This means we can read it from the card to the memory in one go and then work with the memory alone. 

If you are going to use bigger BMP pictures, you may need to rewrite the code to read pictures row by row (which is also a lot simpler than the complicated code I wrote to access the image pixels stored in one-dimensional array).

At first I read the pictures row by row, but the overall performance of the screen was so slow that I decided to rewrite it to read the whole picture in one go. It did not help much because the main bottleneck seems to be the way the pictures are drawn on the screen - pixel by pixel, which is very slow. I don't think MAR4018 supports drawing the entire rows of pixels in one go...

__With the current implementation it takes around 15 seconds to draw 15 124 x 124 pictures. The format of the pictures must be uncompressed 24bit BMP.__

Also worth mentioning that at first I tried using the <SD.h> library, but I didn't find how to easily customize the pins, so I used the <SDFat.h> library instead.

## Customization by config
One of the ideas on which Stream Deck is founded is being able to reprogram the device to easily customize the icons of the buttons and their functions. In this project, this is achieved by modifying the config file stored on the SD card. The config file has a very simple structure. 

* The first two integers in the first line define the amount of cols and rows to arrange the buttons in. The first line must not be empty.
* Further empty lines and lines starting with "#" are skipped
* Lines starting with `PREV` and `NEXT` must have a format of `PREV,path/to/picture.bmp` or `NEXT,path/to/picture.bmp` 
* All other lines must be in the format `path/to/picture.bmp,[keys][to][press]`
* Currently supported keys to press are `[ctrl]`, `[up]`, `[down]`, `[right]`, `[left]`
* Example config:
```
  5,3
  reinforcement.bmp,[ctrl][up][down][right][left][up]
  resupply.bmp,[ctrl][down][down][up][right]
  hellbomb.bmp,[ctrl][down][up][left][down][up][right][down][up]
  orbital_railgun.bmp,[ctrl][right][up][down][down][right]
  orbital_laser.bmp,[ctrl][right][down][up][right][down]
  # eagle_rearm.bmp,[ctrl][up][up][left][up][right]
  # eagle_cluster_bomb.bmp,[ctrl][up][right][down][down][right]
  # machinegun.bmp,[ctrl][down][left][down][up][right]
  stalwart.bmp,[ctrl][down][left][down][up][up][left]
  railgun.bmp,[ctrl][down][right][down][up][left][right]
  autocannon.bmp,[ctrl][down][left][down][up][up][right]
  supply_pack.bmp,[ctrl][down][left][down][up][up][down]
  shield_generator_pack.bmp,[ctrl][down][up][left][right][left][right]
  spear.bmp,[ctrl][down][down][up][down][down]
  shield_generator.bmp,[ctrl][down][up][left][right][left][right]
  hmg_emplacement.bmp,[ctrl][down][up][left][right][right][left]
  tesla_tower.bmp,[ctrl][down][up][right][up][left][right]
```

## Customization by code changes
By changing the code it is easily possible to 
* enable\disable flipping of the BMP pictures (needed because BMPs are stored upside down in memory)
* configure the padding between the edges of the screen and the buttons
* configue the spacing between the buttons themselves
* configure the roundness of the buttons (still treated as squares for the touch coordinate detection)
* flip the X and Y axis to effectively mirror the (0,0) coordinate to the opposite corner of the screen

## TODO 
* PREV and NEXT config lines were implemented to be used with PREV and NEXT buttons that would appear if the amount of buttons is bigger than what can be fit on one screen (`rows * cols`). However, after seeing how much time it takes to draw the pictures from the SD card pixel by pixel, this idea was scrapped.
* It is possible to add support for several config files and change between them using the PREV NEXT buttons. The selected config file can be stored in a special file created on the SD card.





