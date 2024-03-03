# DIY Stream Deck
DIY Stream Deck alternative built with Arduino DUE and 4" MAR4018 NT35510 TFT Touch screen. In this DIY project I had to write a somewhat complicated program in order for the device to act as a Stream Deck.

The main problem of this project was that the library provided by the MAR4018 screen manufacturer was not intended to work with Arduino DUE, but rather with Arduino Mega. Hence why none of the examples worked with Due. Despite that, Mega and Due share the same pin configuration which allows the screen to be plugged directly into the Due board, exactly same way as it works with the Mega.

There were additional difficulties involving the Arduino Keyboard library which required a very specific usage to work in my case, poorly soldered ports on my Due, as well as my Due board being the R3 edition which lacks the neccesary hardware upgrade that allows it to boot correctly when connected to the Native USB port - the issue that has been plaguing Arduino Due boards since at least 2014.

# Fixing the MAR4018 library provided by the screen manufacturer


# Fixing the Arduino Due quirks
In order to emulate the keyboard, Arduino Due must be connected using the Native USB port (as opposed to the Programming USB port). When connected to the Native USB port, my board required a manual press of the RESET button to boot the loaded program. 

You shouldn't have this issue if your board is of R3-E or of later revisions. If you don't observe this behavior on your board, well, less headache for you. In short, there are 2 good ways to fix this issue. 

One way is to solder a 10kOhm resistor to the mosfet next to the 6-pin SPI connector (this solution seems to be derived from the R3-E revision of the board where they added a said resistor in there). Thanks to *dancombine* from the Arduino forum for this solution ( https://forum.arduino.cc/t/due-wont-start-after-power-off-on-have-to-reset/247763/44 ). This is the way I did it and it works really well.

![GitHub Image](/README/before_soldering.jpeg)

![GitHub Image](/README/before_soldering.jpeg)

The other way is to flash the chip with a new BIOS that accounts for the lack of R99 resistor on R3. Refer to the beforementioned link to know how.

Instead of these 2 good solutions you could also get away with soldering a capacitor together with a resistor to one of the RESET pins, but it's not a reliable solution as the nominal of the capacitor can change from board to board and depends on whatever additional devices and peripherals are plugged in, so DON'T do that.

# Reading pictures from the SD card and the bottleneck
Despite being relatively powerful, the 32bit Arduino Due "only" has around 500 KB of memory (which is a lot, BUT not quite enough to store uncompressed BMP pictures in it).

So the idea was to store pictures on the SD card and read them using the MAR4018 built-in card reader. In practice, the size of the BMP pictures that I am going to use are approx 124 x 124 pixels, which take approx 50 KB each. This means we can read it from the card to the memory in one go and then work with the memory alone. 

If you are going to use bigger BMP pictures, you may need to rewrite the code to read pictures row by row (which is also a lot simpler than the complicated code I wrote to access the image pixels stored in one-dimensional array).

At first I read the pictures row by row, but the overall performance of the screen was so slow that I decided to rewrite it to read the whole picture in one go. It did not help much because the main bottleneck seems to be the way the pictures are drawn on the screen - pixel by pixel, which is very slow. 

I don't think MAR4018 supports drawing the entire rows of pixels in one go...

With the current implementation it takes around 15 seconds to draw 15 124 x 124 pictures. The format of the pictures must be uncompressed 24bit BMP. 

# Customization by config
One of the ideas on which Stream Deck is founded is being able to reprogram the device to easily customize the icons of the buttons and their functions.  
