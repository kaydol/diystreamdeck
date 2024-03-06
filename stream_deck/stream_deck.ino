
// Read as "DISPLAY PIN TIRQ CORRESPONDS TO DUE PIN 44"
#define PIN_TIRQ 44 // Touch screen interrupt control pin (low level active)
#define PIN_MISO 50 // SPI bus input pin 
#define PIN_MOSI 51 // SPI bus output pin
#define PIN_EX_CLK 52 // SPI bus clock pin
#define PIN_TP_CS 53 // Touch screen chip select pin (low level active)

#define PIN_RS 38 // LCD register / data selection pin (high level: data, low level:register)
#define PIN_WR 39 // LCD write control pin  
#define PIN_CS 40 // LCD chip select control pin (low level active) 
#define PIN_RST 41 // LCD reset control pin (low level active) 
#define PIN_RD 43 // LCD read control pin 

// Read as "LCD SD PIN CONNECTED TO DUE PIN 48"
#define PIN_SD_CS 48 // Extended reference: SD card select pin (LCD SD)

                             /*  r     g    b */
#define BLACK        0x0000  /*   0,   0,   0 */
#define BLUE         0x001F  /*   0,   0, 255 */
#define RED          0xF800  /* 255,   0,   0 */
#define GREEN        0x07E0  /*   0, 255,   0 */
#define CYAN         0x07FF  /*   0, 255, 255 */
#define MAGENTA      0xF81F  /* 255,   0, 255 */
#define YELLOW       0xFFE0  /* 255, 255,   0 */
#define WHITE        0xFFFF  /* 255, 255, 255 */
#define NAVY         0x000F  /*   0,   0, 128 */
#define DARKGREEN    0x03E0  /*   0, 128,   0 */
#define DARKCYAN     0x03EF  /*   0, 128, 128 */
#define MAROON       0x7800  /* 128,   0,   0 */
#define PURPLE       0x780F  /* 128,   0, 128 */
#define OLIVE        0x7BE0  /* 128, 128,   0 */
#define LIGHTGREY    0xC618  /* 192, 192, 192 */
#define DARKGREY     0x7BEF  /* 128, 128, 128 */
#define ORANGE       0xFD20  /* 255, 165,   0 */
#define GREENYELLOW  0xAFE5  /* 173, 255,  47 */
#define PINK         0xF81F  /* 255,   0, 255 */

#define PIN_BUZZER 13
#define PIN_BUTTONS_POWER 11 // Supplies constant voltage to both buttons
#define PIN_BUTTON_1 9 // connected to this pin as well as to gnd via 10kOhm resistor
#define PIN_BUTTON_2 8 // connected to this pin as well as to gnd via 10kOhm resistor

//---------------------------------------------------------------------------------

#include <SdFat.h>
#include <Keyboard.h>
#include <LCDWIKI_GUI.h> //Core graphics library
#include <LCDWIKI_KBV.h> //Hardware-specific library
#include <LCDWIKI_TOUCH.h> //touch screen library
#include "Serialprint.h" // for better printing, courtesy of David Pankhurst

#if SPI_DRIVER_SELECT != 2  // Must be set in SdFat/SdFatConfig.h
  #error SPI_DRIVER_SELECT must be two in SdFat/SdFatConfig.h
#endif

SoftSpiDriver<PIN_MISO, PIN_MOSI, PIN_EX_CLK> softSpi;

// Speed argument is ignored for software SPI.
#if ENABLE_DEDICATED_SPI
  #define SD_CONFIG SdSpiConfig(PIN_SD_CS, DEDICATED_SPI, SD_SCK_MHZ(0), &softSpi)
#else  // ENABLE_DEDICATED_SPI
  #define SD_CONFIG SdSpiConfig(PIN_SD_CS, SHARED_SPI, SD_SCK_MHZ(0), &softSpi)
#endif  // ENABLE_DEDICATED_SPI


LCDWIKI_KBV my_lcd(NT35510, PIN_CS, PIN_RS, PIN_WR, PIN_RD, PIN_RST); //model,cs,cd,wr,rd,reset
LCDWIKI_TOUCH my_touch(PIN_TP_CS, PIN_EX_CLK, PIN_MISO, PIN_MOSI, PIN_TIRQ); //tcs,tclk,tdout,tdin,tirq

#define DEF_ROTATION_0 0 // (0,0) at bottom left corner relative to how I want it 
#define DEF_ROTATION_90 1 // (0,0) at top left corner relative to how I want it 
#define DEF_ROTATION_180 2
#define DEF_ROTATION_270 3


const String configName = "CONFIG.txt";
#define DEF_CONFIG_LINE_DELIMITER ','
#define DEF_CONFIG_PREV_STARTSWITH "PREV" // if line starts with PREV, expect path to PREV button icon
#define DEF_CONFIG_NEXT_STARTSWITH "NEXT" // if line starts with NEXT, expect path to NEXT button icon
#define DEF_BUTTON_CONFIG_TYPE_NEXT 1
#define DEF_BUTTON_CONFIG_TYPE_PREV -1
#define DEF_BUTTON_CONFIG_TYPE_UNDEFINED 0

#define DEF_BUTTON_KEY_SEND_DELAY 50
#define DEF_BUTTON_PRESS_DELAY 1500

const bool flipBMPs = true; // if true, flips the BMPs (stored upside down by default)
const bool flipXY = true; // if true, changes the button order from "up to down" to "left to right" 
const int16_t screenPadding = 6;
const uint8_t rectangleRound = 12;
const uint8_t rectangleSpacing = 10;
const uint8_t maxButtons = 200;
int16_t screenWidth;  
int16_t screenHeight;

struct ButtonConfig {
  String *path, *keys;
  int8_t type;
  uint16_t bmpHeight, bmpWidth, x1, x2, y1, y2;
  uint64_t filePos;
  bool initialized;
};

struct ConfigData {
  uint8_t rows, cols;
  ButtonConfig buttonNext, buttonPrev;
  ButtonConfig* buttons[maxButtons];
  size_t buttonsCount;
  String cfgName;
  uint8_t currentPage;
  bool drawPrevNext;
} config;

void swap(int16_t & a, int16_t & b) {
  int16_t c; c = a; a = b; b = c;
}

void swap(uint16_t & a, uint16_t & b) {
  uint16_t c; c = a; a = b; b = c;
}

// Due does not have builtin 'tone' function 
void tone(uint32_t buzzer, uint32_t tone_freq, uint32_t tone_duration) 
{
  digitalWrite(buzzer, HIGH);
  delay(tone_duration);
  digitalWrite(buzzer, LOW);
}

bool isButtonPressed(uint32_t pin) {
  return digitalRead(pin) == HIGH;
}

void showFatalError(uint8_t textSize, const char* error) {
  my_lcd.Set_Text_Back_colour(BLUE);
  my_lcd.Set_Text_colour(WHITE);    
  my_lcd.Set_Text_Size(textSize);
  my_lcd.Print_String(error,0,0);
  while(1);
}

void setup() 
{
  Serial.begin(115200);
  //SerialUSB.begin();
  while (!Serial) {
  //  yield(); // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Initializing...");

  // Touch does not work on Due without these lines:
  pinMode(PIN_TIRQ, OUTPUT); 
  pinMode(PIN_EX_CLK, OUTPUT); 
  pinMode(PIN_MISO, INPUT); 
  pinMode(PIN_MOSI, OUTPUT);
  pinMode(PIN_TP_CS, OUTPUT);

  // Buzzer 
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  // Buttons
  pinMode(PIN_BUTTONS_POWER, OUTPUT);
  digitalWrite(PIN_BUTTONS_POWER, HIGH);
  pinMode(PIN_BUTTON_1, INPUT);
  pinMode(PIN_BUTTON_2, INPUT);

  my_lcd.Init_LCD();
  my_lcd.Set_Rotation(DEF_ROTATION_270);
  
  screenWidth = my_lcd.Get_Display_Width();  
  screenHeight = my_lcd.Get_Display_Height();
  
  if (flipXY)
    swap(screenWidth,screenHeight);
  
  my_lcd.Fill_Screen(BLACK); 

  my_touch.TP_Init(my_lcd.Get_Rotation(),my_lcd.Get_Display_Width(),my_lcd.Get_Display_Height()); // works for lcd rotation 90 and 270

  // SD card check 
  SdFat sd;
  if (!sd.begin(SD_CONFIG))
    showFatalError(8, "No SD Card");
  
  size_t entries = parseConfig(&config, configName);
  if (entries <= 0) {
    String msg = "No entries in " + configName;
    showFatalError(5, msg.c_str());
  }

  config.currentPage = 0;
  drawButtons(&config);
  
  Keyboard.begin();
  tone(PIN_BUZZER, 1000, 10);
}


size_t parseConfig(ConfigData* cfgData, String cfgName) 
{
  File cfgFile;
  bool success = cfgFile.open(cfgName.c_str(), O_READ);
  if (!success)
  {
    String msg = "No " + cfgName;
    showFatalError(6, msg.c_str());
  }
  
  cfgData->cfgName = cfgName;
  cfgFile.rewind();

  if (cfgFile.available())
  {
    cfgData->cols = cfgFile.parseInt();
    cfgData->rows = cfgFile.parseInt();
    cfgFile.readStringUntil('\n'); // skip until the end of the line 
    Serialprint("Cols = %d, Rows = %d\n", cfgData->cols, cfgData->rows);
  }

  size_t entries = 0;

  while (cfgFile.available())
  {
    String line = cfgFile.readStringUntil('\n');
    line.trim();

    if (line.length() <= 0) continue;
    if (line.startsWith("#")) continue;

    String left = line.substring(0, line.indexOf(DEF_CONFIG_LINE_DELIMITER)) + '\0';
    String right = line.substring(line.indexOf(DEF_CONFIG_LINE_DELIMITER)+1, line.length());

    if (left == NULL || right == NULL)
      showFatalError(5, "Out of memory");

    if (line.equalsIgnoreCase(DEF_CONFIG_PREV_STARTSWITH)) {
      cfgData->buttonPrev.type = DEF_BUTTON_CONFIG_TYPE_PREV;
      cfgData->buttonPrev.path = new String(right);
      continue;
    }
    if (line.equalsIgnoreCase(DEF_CONFIG_NEXT_STARTSWITH)) {
      cfgData->buttonNext.type = DEF_BUTTON_CONFIG_TYPE_NEXT;
      cfgData->buttonNext.path = new String(right);
      continue;
    }
    
    ButtonConfig* newButton = (ButtonConfig*) malloc (sizeof(ButtonConfig));
    //Serialprint("Saving pointer = %p\n", newButton);

    if (newButton == NULL)
      showFatalError(5, "Out of memory");
    
    newButton->path = new String(left);
    newButton->keys = new String(right);
    newButton->type = DEF_BUTTON_CONFIG_TYPE_UNDEFINED;
    newButton->initialized = false;
    initBMP(newButton);

    cfgData->buttons[entries] = newButton;
    ++entries;
  }

  cfgFile.close();

  cfgData->buttonsCount = entries;
  cfgData->drawPrevNext = cfgData->buttonsCount > (cfgData->rows * cfgData->cols);

  Serialprint("%s contains %d entries\n", cfgName.c_str(), entries);

  return entries;
}

uint16_t read16(char * buffer)
{
  uint8_t low;
  uint16_t high;
  low = buffer[0];
  high = buffer[1];

  //Serialprint("HEX = %x %x\n", low, high);

  return (high<<8)|low;
}

uint32_t read32(char* buffer)
{
  uint16_t low;
  uint32_t high;
  low = read16(buffer);
  high = read16(buffer+2);
  return (high<<8)|low;   
}

void initBMP(ButtonConfig* button) 
{
  button->bmpWidth = 0;
  button->bmpHeight = 0;

  File f; 
  bool success = f.open(button->path->c_str()); 
  if (!success) {
    Serialprint("Could not open file %s \n", button->path->c_str());
    return;
  }

  f.rewind();

  char buffer16[2];
  char buffer32[4];
  size_t read;
  
  read = f.readBytes(buffer16, sizeof(buffer16));

  if(read16(buffer16) != 0x4D42) // File is not BMP 
    return;
  
  //read = f.readBytes(buffer32, sizeof(buffer32));
  //uint32_t bmpFileSize = read32(buffer32);

  f.seek(18);

  read = f.readBytes(buffer32, sizeof(buffer32));
  int bmpWidth  = read32(buffer32);
  read = f.readBytes(buffer32, sizeof(buffer32));
  int bmpHeight  = read32(buffer32);
  
  read = f.readBytes(buffer16, sizeof(buffer16));

  if(read16(buffer16) == 1) // # planes -- must be '1'
  {
    read = f.readBytes(buffer16, sizeof(buffer16));
    uint8_t bmpDepth = read16(buffer16); // bits per pixel

    //Serialprint("Bit Depth: %d", bmpDepth);
    
    read = f.readBytes(buffer32, sizeof(buffer32));
    uint16_t compression = read32(buffer32);

    if((bmpDepth == 24) && (compression == 0)) // only 24 depth is supported; 0 = uncompressed
    {
      button->bmpWidth = bmpWidth;
      button->bmpHeight = bmpHeight;
      button->filePos = f.curPosition() + 2;
      button->initialized = true;
    }
  }

  f.close();
}

void drawBMP(ButtonConfig* button, uint16_t x0, uint16_t y0)
{
  if (!button->initialized)
    return;

  File f; 
  bool success = f.open(button->path->c_str()); 
  if (!success) {
    Serialprint("Could not open file %s \n", button->path->c_str());
    return;
  }

  f.seek(button->filePos);
  
  // Read whole picture in one go 
  size_t length = button->bmpHeight * button->bmpWidth; 
  // (button->bmpHeight * button->bmpWidth * 3 + 3) & ~3
  uint8_t* data = (uint8_t*) malloc(length * 3);
  if (data == NULL)
    return;

  f.readBytes(data, length * 3);
  f.close();

  for (uint16_t row = 0; row < button->bmpHeight; ++row)
  {
    size_t m = row * button->bmpWidth * 3;
    for (uint16_t col = 0; col < button->bmpWidth; ++col) 
    {
      uint16_t c = my_lcd.Color_To_565(data[m+2], data[m+1], data[m+0]);
      my_lcd.Set_Draw_color(c);
      if (flipBMPs)
        my_lcd.Draw_Pixel(x0 + col, y0 + button->bmpHeight - 1 - row);
      else
        my_lcd.Draw_Pixel(x0 + col, y0 + row);
      m+=3;
    }
  }
  free(data);
}

void drawButtons(ConfigData* cfgData)
{
  uint16_t cellHeight = (uint16_t) ((float)(screenHeight-screenPadding-((uint16_t)rectangleSpacing*(uint16_t)(cfgData->cols-1))) / (float)cfgData->cols);
  uint16_t cellWidth = (uint16_t) ((float)(screenWidth-screenPadding-((uint16_t)rectangleSpacing*(uint16_t)(cfgData->rows-1))) / (float)cfgData->rows);

  char dimensions[15];
  sprintf(dimensions, "%dx%d", cellHeight, cellWidth);

  //Serialprint("Calculated cell size = %s\n", dimensions);
  
  my_lcd.Set_Draw_color(WHITE);
  my_lcd.Set_Text_Size(2);
  my_lcd.Set_Text_colour(GREEN);
  my_lcd.Set_Text_Back_colour(BLACK);

  if (cfgData->drawPrevNext) {
    showFatalError(6, "Too many entries!");
  } else {
    size_t i = 0;
    for(uint8_t row = 0; row < cfgData->rows; ++row) 
    {
      for(uint8_t col = 0; col < cfgData->cols; ++col) 
      {
        if (i >= cfgData->buttonsCount) { 
          // do not draw more buttons than were provided in the cfg 
          break;
        }
        uint16_t x1, y1, x2, y2;
        
        if (flipXY) {
          // Swapped
          x1 = screenPadding / 2 + row * cellWidth + row * rectangleSpacing;
          y1 = screenPadding / 2 + col * cellHeight + col * rectangleSpacing;
          x2 = x1 + cellWidth;
          y2 = y1 + cellHeight;
          swap(x1, y1);
          swap(x2, y2);
        } else {
          // Original
          x1 = screenPadding / 2 + row * cellWidth + row * rectangleSpacing;
          y1 = screenPadding / 2 + col * cellHeight + col * rectangleSpacing;
          x2 = x1 + cellWidth;
          y2 = y1 + cellHeight;
        }

        cfgData->buttons[i]->x1 = x1;
        cfgData->buttons[i]->x2 = x2;
        cfgData->buttons[i]->y1 = y1;
        cfgData->buttons[i]->y2 = y2;

        //Serialprint("Square (%d, %d) (%d %d)\n", x1, y1, x2, y2);
        my_lcd.Print_String(dimensions, x1+cellWidth/5, y1+cellHeight/2);
        my_lcd.Set_Draw_color(WHITE);
        my_lcd.Draw_Round_Rectangle(x1, y1, x2, y2, rectangleRound);
        
        //Serialprint("Drawing icon: %s\n", (*(cfgData->buttons[i]->path)).c_str());

        if (cfgData->buttons[i]->initialized) {
          uint16_t x0 = x1 + (cellWidth-cfgData->buttons[i]->bmpWidth)/2;
          uint16_t y0 = y1 + (cellHeight-cfgData->buttons[i]->bmpHeight)/2;
          drawBMP(cfgData->buttons[i], x0, y0);
        }
        
        ++i;
      }
      if (i >= cfgData->buttonsCount) {
        // do not draw more buttons than were provided in the cfg 
        break;
      }
    }
  }
}

void loop() 
{
  my_touch.TP_Scan(0);
  
  if (isButtonPressed(PIN_BUTTON_1)) {
    tone(PIN_BUZZER, 1000, 100);
    delay(DEF_BUTTON_PRESS_DELAY);
  }
  if (isButtonPressed(PIN_BUTTON_2)) {
    tone(PIN_BUZZER, 1000, 100);
    delay(100);
    tone(PIN_BUZZER, 1000, 100);
    delay(DEF_BUTTON_PRESS_DELAY);
  }

  if (my_touch.TP_Get_State()&TP_PRES_DOWN) 
  {
    uint16_t x = my_lcd.Get_Display_Width() - my_touch.x; 
    uint16_t y = my_touch.y;
    
    // the Touch does not seem to be affected by flipXY 
    //if (flipXY)
    //  swap(x, y);
    
    for (size_t i = 0; i < config.buttonsCount; ++i) 
    {
      ButtonConfig* b = config.buttons[i];
      if (b->x1 < x && x < b->x2 && b->y1 < y && y < b->y2) {
        // button pressed
        Serialprint("%s\n", b->path->c_str());
        tone(PIN_BUZZER, 1000, 10);
        String keys = *(b->keys);
        int i = keys.indexOf('[');
        while(i >= 0) {
          String str = keys.substring(i+1, keys.indexOf(']', i));
          //Serialprint("%d Key pressed = %s\n", i, str.c_str());

          uint16_t key = -1;
          if (str.equalsIgnoreCase("up"))
            key = KEY_UP_ARROW;
          else if (str.equalsIgnoreCase("down"))
            key = KEY_DOWN_ARROW;
          else if (str.equalsIgnoreCase("right")) 
            key = KEY_RIGHT_ARROW;
          else if (str.equalsIgnoreCase("left")) 
            key = KEY_LEFT_ARROW;
          else if (str.equalsIgnoreCase("ctrl")) 
            key = KEY_LEFT_CTRL;
          else if (str.length() == 1) {
            key = str[0];
          }

          //Serialprint("str = '%s', key = %x\n", str.c_str(), key);
          if (key >= 0 /*&& key != KEY_LEFT_CTRL*/) {
            Keyboard.press(key);
            delay(DEF_BUTTON_KEY_SEND_DELAY); 
            Keyboard.release(key); 
            delay(DEF_BUTTON_KEY_SEND_DELAY);
          }
          
          i = keys.indexOf('[', i+1);
        }
        
        delay(DEF_BUTTON_PRESS_DELAY);
        break;
      }
    } 
  }
}
