
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


#define DEF_ERR_NONE 0
#define DEF_ERR_NOT_ENOUGH_MEMORY 1
#define DEF_ERR_NOT_INITIALIZED 2
#define DEF_ERR_CANT_OPEN_FILE 3
#define DEF_ERR_UNRECOGNIZED_FILE_FORMAT 4
#define DEF_ERR_UNEXPECTED_COLOR_MASK_FORMAT 5
#define DEF_ERR_UNEXPECTED_COLOR_SPACE_TYPE 6
#define DEF_ERR_NO_BIT_MASK_INFO 7

#define DEF_LCDWIKI_KBV_FLAGS_NONE 0
#define DEF_LCDWIKI_KBV_FLAGS_CONST 1
#define DEF_LCDWIKI_KBV_FLAGS_IS_BIG_ENDIAN 2

#define DEF_ROTATION_0 0 
#define DEF_ROTATION_90 1 
#define DEF_ROTATION_180 2
#define DEF_ROTATION_270 3

#define DEF_CONFIG_LINE_DELIMITER ','
#define DEF_CONFIG_PREV_STARTSWITH "PREV" // if line starts with PREV, expect path to PREV button icon
#define DEF_CONFIG_NEXT_STARTSWITH "NEXT" // if line starts with NEXT, expect path to NEXT button icon
#define DEF_CONFIG_SELECT_ALL_STARTSWITH "SELECTALL" // if line starts with SELECTALL, expect path to SELECTALL button icon

#define DEF_BUTTON_TYPE_UNDEFINED 0
#define DEF_BUTTON_TYPE_PREV 1
#define DEF_BUTTON_TYPE_NEXT 2
#define DEF_BUTTON_TYPE_SELECT_ALL 3

#define DEF_KEY_SEND_DELAY 50
#define DEF_PHYSICAL_PRESS_DELAY 500

#define DEF_BUTTON_RECTANGLE_ROUNDNESS 12
