#include "AppDefinitions.h"
#include "Serialprint.h" // for better printing, courtesy of David Pankhurst
#include "Button.h"

//#include "FreeMemory.h"

#include <SdFat.h>
#include <Keyboard.h>
#include <Arduino.h>
#include <SdFat.h>
#include <LCDWIKI_GUI.h> //Core graphics library
#include <LCDWIKI_KBV.h> //Hardware-specific library
#include <LCDWIKI_TOUCH.h> //touch screen library

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

const String configName = "CONFIG.txt";

constexpr bool flipXY = true; // if true, changes the button order from "up to down" to "left to right" 
constexpr uint8_t maxButtons = 200;
int16_t screenWidth;
int16_t screenHeight;

struct ConfigData
{
	Button *buttons[maxButtons], *selectedButtons[maxButtons];
	String cfgName;
	uint8_t rows, cols, currentPageIndex, lastPageIndex;
	size_t buttonsCount, selectedButtonsCount;
	bool isInButtonSelection;
	Button ButtonNext, ButtonPrev, ButtonSelectAll;
} config;


void swap(int16_t& a, int16_t& b)
{
	int16_t c;
	c = a;
	a = b;
	b = c;
}

void swap(uint16_t& a, uint16_t& b)
{
	uint16_t c;
	c = a;
	a = b;
	b = c;
}

// Due does not have builtin 'tone' function 
void tone(uint32_t buzzer, uint32_t tone_freq, uint32_t tone_duration)
{
	digitalWrite(buzzer, HIGH);
	delay(tone_duration);
	digitalWrite(buzzer, LOW);
}

bool isPhysicalButtonPressed(uint32_t pin)
{
	return digitalRead(pin) == HIGH;
}

void showFatalError(uint8_t textSize, const char* error)
{
	Serialprint("%s\n", error);
	my_lcd.Set_Text_Back_colour(BLUE);
	my_lcd.Set_Text_colour(WHITE);
	my_lcd.Set_Text_Size(textSize);
	my_lcd.Print_String(error, 0, 0);
	while (1);
}

void handleError(uint8_t code, const char* arg = nullptr)
{
	switch (code)
	{
	case DEF_ERR_NONE:
		{
			return;
		}
	case DEF_ERR_NO_SD_CARD:
		{
			showFatalError(8, "No SD Card");
			break;
		}
	case DEF_ERR_CANT_OPEN_FILE:
		{
			char str[30];
			sprintf(str, "Can't open %s\0", arg);
			showFatalError(5, str);
			break;
		}
	case DEF_ERR_NO_ENTRIES_IN_CONFIG:
		{
			char str[30];
			sprintf(str, "No entries in  %s\0", arg);
			showFatalError(5, str);
			break;
		}
	case DEF_ERR_TOO_MANY_ENTRIES:
		{
			char str[30];
			sprintf(str, "Too many entries (>%s)\0", arg);
			showFatalError(5, str);
			break;
		}
	case DEF_ERR_NOT_ENOUGH_MEMORY:
		{
			showFatalError(5, "Out of memory");
			break;
		}
	case DEF_ERR_UNEXPECTED_COLOR_SPACE_TYPE:
		{
			showFatalError(2, "Unexpected color space type! The program expects sRGB values");
			break;
		}
	case DEF_ERR_UNEXPECTED_COLOR_MASK_FORMAT:
		{
			showFatalError(2, "Unexpected color mask format!The program expects the pixel data to be in the BGRA format");
			break;
		}
	case DEF_ERR_NO_BIT_MASK_INFO:
		{
			showFatalError(2, "The file does not seem to contain bit mask information");
			break;
		}
	default:
		{
			showFatalError(5, "Unspecified error");
		}
	}
}

void setup()
{
	Serial.begin(115200);
	//SerialUSB.begin();
	while (!Serial)
	{
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
	my_lcd.Set_Rotation(DEF_ROTATION_90);

	screenWidth = my_lcd.Get_Display_Width();
	screenHeight = my_lcd.Get_Display_Height();

	if (flipXY)
		swap(screenWidth, screenHeight);

	my_lcd.Fill_Screen(BLACK);

	my_touch.TP_Init(my_lcd.Get_Rotation(), my_lcd.Get_Display_Width(), my_lcd.Get_Display_Height());
	// works for lcd rotation 90 and 270

	config.ButtonPrev.SetType(DEF_BUTTON_TYPE_PREV);
	config.ButtonNext.SetType(DEF_BUTTON_TYPE_NEXT);
	config.ButtonSelectAll.SetType(DEF_BUTTON_TYPE_SELECT_ALL);

	size_t entries = parseConfig(&config, configName);
	if (entries <= 0)
		handleError(DEF_ERR_NO_ENTRIES_IN_CONFIG);

	config.isInButtonSelection = true;
	config.currentPageIndex = 0;
	config.lastPageIndex = getLastPageIndex(config.cols * config.rows, entries + (config.isInButtonSelection ? 1 : 0),
	                                        canFitInTwoPages(entries + (config.isInButtonSelection ? 1 : 0),
	                                                         config.cols, config.rows));

	drawButtons(&config);

	Keyboard.begin();
	tone(PIN_BUZZER, 1000, 10);
}

bool canFitInTwoPages(size_t buttonsCount, uint8_t cols, uint8_t rows)
{
	return buttonsCount <= (2 * cols * rows - 2);
}

// Calculates number of pages required to show all the buttons for given grid, accounts for "prev" and "next" buttons
uint8_t getLastPageIndex(size_t buttonsPerScreen, size_t buttonsTotal, bool canFitInTwoPages)
{
	uint8_t index = 0;
	if (buttonsTotal <= buttonsPerScreen)
		return index;
	++index;
	size_t buttonsLeft = buttonsTotal - buttonsPerScreen - (canFitInTwoPages ? 1 : 2);
	while (buttonsLeft > 0)
	{
		if (buttonsLeft <= buttonsPerScreen - (canFitInTwoPages ? 1 : 2))
			return index;
		buttonsLeft -= (buttonsPerScreen - 2);
		++index;
	}
	return 0;
}

size_t getCountOfPreviouslyDrawnButtons(uint8_t currentPageIndex, uint8_t lastPageIndex, size_t buttonsPerScreen)
{
	return currentPageIndex * (buttonsPerScreen - (lastPageIndex == 1 ? 1 : 2));
}

// Returns amount of buttons, NEXT and PREV buttons are excluded
size_t parseConfig(ConfigData* cfgData, String cfgName)
{
	// SD card check 
	SdFat sd;
	if (!sd.begin(SD_CONFIG))
		handleError(DEF_ERR_NO_SD_CARD, nullptr);

	File cfgFile;
	const bool success = cfgFile.open(cfgName.c_str(), O_READ);
	if (!success)
		handleError(DEF_ERR_CANT_OPEN_FILE, cfgName.c_str());

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

		String left = line.substring(0, line.indexOf(DEF_CONFIG_LINE_DELIMITER));
		String right = line.substring(line.indexOf(DEF_CONFIG_LINE_DELIMITER) + 1, line.length());

		if (left == nullptr || right == nullptr)
			handleError(DEF_ERR_NOT_ENOUGH_MEMORY);

		if (left.equalsIgnoreCase(DEF_CONFIG_PREV_STARTSWITH))
		{
			handleError(cfgData->ButtonPrev.InitBMP(new String(right)));
			continue;
		}
		if (left.equalsIgnoreCase(DEF_CONFIG_NEXT_STARTSWITH))
		{
			handleError(cfgData->ButtonNext.InitBMP(new String(right)));
			continue;
		}
		if (left.equalsIgnoreCase(DEF_CONFIG_SELECT_ALL_STARTSWITH))
		{
			handleError(cfgData->ButtonSelectAll.InitBMP(new String(right)));
			continue;
		}

		Button* newButton = new Button();

		if (newButton == nullptr)
			handleError(DEF_ERR_NOT_ENOUGH_MEMORY);

		newButton->SetKeys(new String(right));

		const uint8_t errorCode = newButton->InitBMP(new String(left));
		if (errorCode != DEF_ERR_NONE)
			handleError(errorCode);

		cfgData->buttons[entries] = newButton;
		++entries;

		if (entries >= sizeof(cfgData->buttons) / sizeof(Button*))
			handleError(DEF_ERR_TOO_MANY_ENTRIES);
	}

	cfgFile.close();
	sd.end();

	cfgData->buttonsCount = entries;
	return entries;
}

template <typename T>
void PrintBinary(T val)
{
	for (size_t i = 0; i < sizeof(T) * 8; ++i)
	{
		if (i > 0 && i % 8 == 0)
			Serialprint(" ");
		Serialprint("%d", (val & (1 << i)) >> i);
	}
	Serial.println();
}

void drawButtons(ConfigData* cfgData)
{
	const uint16_t cellHeight = (uint16_t)((float)(screenHeight - DEF_SCREEN_PADDING - (DEF_BUTTON_RECTANGLE_SPACING * (
		uint16_t)(cfgData->cols - 1))) / (float)cfgData->cols);
	const uint16_t cellWidth = (uint16_t)((float)(screenWidth - DEF_SCREEN_PADDING - (DEF_BUTTON_RECTANGLE_SPACING * (
		uint16_t)(cfgData->rows - 1))) / (float)cfgData->rows);

	size_t i = getCountOfPreviouslyDrawnButtons(cfgData->currentPageIndex, cfgData->lastPageIndex,
	                                            cfgData->rows * cfgData->cols);
	if (cfgData->isInButtonSelection && i > 0)
		i--; // account for the "SELECTALL" button which is drawn on 1st page

	my_lcd.Fill_Screen(BLACK);

	for (size_t i = 0; i < cfgData->buttonsCount; ++i)
		cfgData->buttons[i]->SetOnScreen(false);
	cfgData->ButtonSelectAll.SetOnScreen(false);
	cfgData->ButtonPrev.SetOnScreen(false);
	cfgData->ButtonNext.SetOnScreen(false);

	SdFat sd;
	if (!sd.begin(SD_CONFIG))
		handleError(DEF_ERR_NO_SD_CARD, nullptr);

	for (uint8_t row = 0; row < cfgData->rows; ++row)
	{
		for (uint8_t col = 0; col < cfgData->cols; ++col)
		{
			uint16_t x1, y1, x2, y2;

			if (flipXY)
			{
				// Swapped
				x1 = DEF_SCREEN_PADDING / 2 + row * cellWidth + row * DEF_BUTTON_RECTANGLE_SPACING;
				y1 = DEF_SCREEN_PADDING / 2 + col * cellHeight + col * DEF_BUTTON_RECTANGLE_SPACING;
				x2 = x1 + cellWidth;
				y2 = y1 + cellHeight;
				swap(x1, y1);
				swap(x2, y2);
			}
			else
			{
				// Original
				x1 = DEF_SCREEN_PADDING / 2 + row * cellWidth + row * DEF_BUTTON_RECTANGLE_SPACING;
				y1 = DEF_SCREEN_PADDING / 2 + col * cellHeight + col * DEF_BUTTON_RECTANGLE_SPACING;
				x2 = x1 + cellWidth;
				y2 = y1 + cellHeight;
			}

			Button* button = nullptr;

			if (cfgData->isInButtonSelection && row == 0 && col == 0 && cfgData->currentPageIndex == 0)
			{
				button = &(cfgData->ButtonSelectAll);
			}
			else if (cfgData->lastPageIndex > 1 && row == cfgData->rows - 1 && col == cfgData->cols - 1)
			{
				button = &(cfgData->ButtonNext);
			}
			else if (cfgData->lastPageIndex > 1 && row == cfgData->rows - 1 && col == cfgData->cols - 2)
			{
				button = &(cfgData->ButtonPrev);
			}
			else if (cfgData->currentPageIndex == 0 && cfgData->lastPageIndex != 0 && row == cfgData->rows - 1 && col ==
				cfgData->cols - 1)
			{
				// first page, only draw next, draw it in the last cell 
				button = &(cfgData->ButtonNext);
			}
			else if (cfgData->currentPageIndex == cfgData->lastPageIndex && cfgData->lastPageIndex != 0 && row ==
				cfgData->rows - 1 && col == cfgData->cols - 1)
			{
				// last page, only draw prev, draw it in the last cell 
				button = &(cfgData->ButtonPrev);
			}
			else
			{
				Button** array;
				size_t arraySize;

				if (cfgData->isInButtonSelection)
				{
					// draw all buttons when in selection menu 
					array = cfgData->buttons;
					arraySize = cfgData->buttonsCount;
				}
				else
				{
					// draw only selected buttons when outside selection menu
					array = cfgData->selectedButtons;
					arraySize = cfgData->selectedButtonsCount;
				}

				if (i < arraySize)
					button = array[i++];
			}

			if (button == nullptr)
				continue;

			button->SetOnScreen(true);
			button->SetXY(x1, y1, x2, y2);

			char dimensions[15];
			sprintf(dimensions, "%dx%d", cellHeight, cellWidth);
			button->SetWidth(cellWidth);
			button->SetHeight(cellHeight);
			button->DrawRectangle(&my_lcd, dimensions);

			if (button->IsInitialized())
			{
				const uint16_t x0 = x1 + (cellWidth - button->GetBMPWidth()) / 2;
				const uint16_t y0 = y1 + (cellHeight - button->GetBMPHeight()) / 2;
				button->SetBMPXY(x0, y0);

				const uint8_t errorCode = button->DrawBMP(&my_lcd);
				if (errorCode != DEF_ERR_NONE)
					handleError(errorCode);
			}

			if (button->IsSelected() && cfgData->isInButtonSelection)
				button->DrawSelection(&my_lcd);
		}
	}
	sd.end();
}

void loop()
{
	my_touch.TP_Scan(0);

	if (isPhysicalButtonPressed(PIN_BUTTON_1))
	{
		tone(PIN_BUZZER, 1000, 100);
		delay(DEF_PHYSICAL_PRESS_DELAY);

		size_t j = 0;
		for (size_t i = 0; i < config.buttonsCount; ++i)
		{
			if (config.buttons[i]->IsSelected())
			{
				config.selectedButtons[j++] = config.buttons[i];
			}
		}
		config.selectedButtonsCount = j;

		config.isInButtonSelection = false;
		config.currentPageIndex = 0;
		config.lastPageIndex = getLastPageIndex(
			config.cols * config.rows,
			config.selectedButtonsCount + (config.isInButtonSelection ? 1 : 0),
			canFitInTwoPages(config.selectedButtonsCount + (config.isInButtonSelection ? 1 : 0),
			                 config.cols,
			                 config.rows)
		);

		drawButtons(&config);
	}
	if (isPhysicalButtonPressed(PIN_BUTTON_2))
	{
		tone(PIN_BUZZER, 1000, 100);
		delay(100);
		tone(PIN_BUZZER, 1000, 100);
		delay(DEF_PHYSICAL_PRESS_DELAY);

		config.isInButtonSelection = true;
		config.currentPageIndex = 0;
		config.lastPageIndex = getLastPageIndex(
			config.cols * config.rows,
			config.buttonsCount + (config.isInButtonSelection ? 1 : 0),
			canFitInTwoPages(config.buttonsCount + (config.isInButtonSelection ? 1 : 0),
			                 config.cols,
			                 config.rows)
		);

		drawButtons(&config);
	}

	if (my_touch.TP_Get_State() & TP_PRES_DOWN)
	{
		const uint16_t x = my_lcd.Get_Display_Width() - my_touch.x;
		const uint16_t y = my_touch.y;

		// the Touch does not seem to be affected by flipXY 
		//if (flipXY)
		//  swap(x, y);

		Button** array;
		size_t arraySize;

		if (config.isInButtonSelection)
		{
			array = config.buttons;
			arraySize = config.buttonsCount;
		}
		else
		{
			array = config.selectedButtons;
			arraySize = config.selectedButtonsCount;
		}

		if (config.ButtonPrev.IsOnScreen() && config.ButtonPrev.IsHit(x, y))
		{
			tone(PIN_BUZZER, 1000, 10);
			config.currentPageIndex = (config.currentPageIndex == 0
				                           ? config.lastPageIndex
				                           : config.currentPageIndex - 1);
			drawButtons(&config);
			return;
		}
		if (config.ButtonNext.IsOnScreen() && config.ButtonNext.IsHit(x, y))
		{
			tone(PIN_BUZZER, 1000, 10);
			config.currentPageIndex = (config.currentPageIndex == config.lastPageIndex
				                           ? 0
				                           : config.currentPageIndex + 1);
			drawButtons(&config);
			return;
		}
		if (config.ButtonSelectAll.IsOnScreen() && config.ButtonSelectAll.IsHit(x, y))
		{
			tone(PIN_BUZZER, 1000, 10);

			config.ButtonSelectAll.SetSelected(!config.ButtonSelectAll.IsSelected());
			config.ButtonSelectAll.DrawSelection(&my_lcd);

			for (size_t j = 0; j < arraySize; ++j)
			{
				array[j]->SetSelected(config.ButtonSelectAll.IsSelected());
				if (array[j]->IsOnScreen())
				{
					array[j]->DrawSelection(&my_lcd);
				}
			}

			delay(DEF_PHYSICAL_PRESS_DELAY);
			return;
		}

		size_t buttonsPerScreen = config.rows * config.cols;

		size_t i0 = getCountOfPreviouslyDrawnButtons(config.currentPageIndex, config.lastPageIndex, buttonsPerScreen);
		if (config.isInButtonSelection && config.currentPageIndex > 0)
			i0--; // account for the "SELECTALL" button which is drawn on 1st page

		size_t iN = i0 + buttonsPerScreen > arraySize ? arraySize : i0 + buttonsPerScreen;

		for (size_t i = i0; i < iN; ++i)
		{
			Button* b = array[i];

			if (b->IsHit(x, y))
			{
				// button pressed
				Serialprint("%s\n", b->GetBMPPath().c_str());
				tone(PIN_BUZZER, 1000, 10);

				if (config.isInButtonSelection)
				{
					b->SetSelected(!b->IsSelected());
					b->DrawSelection(&my_lcd);
				}
				else
				{
					b->SendKeys();
				}
				delay(DEF_PHYSICAL_PRESS_DELAY);
				break;
			}
		}
	}
}
