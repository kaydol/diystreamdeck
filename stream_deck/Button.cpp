// ReSharper disable CppZeroConstantCanBeReplacedWithNullptr
// ReSharper disable CppCStyleCast
// ReSharper disable CppInconsistentNaming
// ReSharper disable CppClangTidyBugproneNarrowingConversions

#include <Arduino.h>
#include <SdFat.h>
#include <Keyboard.h>
#include <LCDWIKI_KBV.h> //Hardware-specific library

#include "AppDefinitions.h"
#include "Serialprint.h"
#include "Button.h"
#include "Memory.h"

uint8_t Button::InitBMP(String* path)
{
	SetBMPPath(path);

	File f;
	const bool success = f.open(GetBMPPath().c_str());
	if (!success)
		return DEF_ERR_CANT_OPEN_FILE;

	f.readBytes((char*)&_file_header, sizeof(BMPFileHeader));

	if (_file_header.file_type != 0x4D42)
		return DEF_ERR_UNRECOGNIZED_FILE_FORMAT;

	f.readBytes((char*)&_bmp_info_header, sizeof(BMPInfoHeader));

	// The BMPColorHeader is used only for transparent images
	if (_bmp_info_header.bit_count == 32)
	{
		// Check if the file has bit mask color information
		if (_bmp_info_header.size >= (sizeof(BMPInfoHeader) + sizeof(BMPColorHeader)))
		{
			f.read((char*)&_bmp_color_header, sizeof(BMPColorHeader));

			// Check if the pixel data is stored as BGRA and if the color space type is sRGB
			constexpr BMPColorHeader expected_color_header;
			if (expected_color_header.red_mask != _bmp_color_header.red_mask ||
				expected_color_header.blue_mask != _bmp_color_header.blue_mask ||
				expected_color_header.green_mask != _bmp_color_header.green_mask ||
				expected_color_header.alpha_mask != _bmp_color_header.alpha_mask)
				return DEF_ERR_UNEXPECTED_COLOR_MASK_FORMAT;
			
			if (expected_color_header.color_space_type != _bmp_color_header.color_space_type)
				return DEF_ERR_UNEXPECTED_COLOR_SPACE_TYPE;
		}
		else
		{
			return DEF_ERR_NO_BIT_MASK_INFO;
		}
	}

	// Jump to the pixel data location
	f.seek(_file_header.offset_data);

	// Adjust the header fields for output.
	// Some editors will put extra info in the image file, we only save the headers and the data.
	if (_bmp_info_header.bit_count == 32)
	{
		_bmp_info_header.size = sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
		_file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
	}
	else
	{
		_bmp_info_header.size = sizeof(BMPInfoHeader);
		_file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
	}
	_file_header.file_size = _file_header.offset_data;

	if (_bmp_info_header.planes == 1) // # planes -- must be '1'
	{
		if ((_bmp_info_header.bit_count == 24 || _bmp_info_header.bit_count == 16) && (_bmp_info_header.compression ==
			0)) // only 24/16 bit per pixel depth is supported; 0 = uncompressed
		{
			if (_bmp_info_header.height < 0)
			{
				_bmpIsFlipped = true;
				_bmp_info_header.height = -_bmp_info_header.height;
			}
			else
			{
				_bmpIsFlipped = false;
			}
			_isInitialized = true;
		}
	}

	f.close();
	return DEF_ERR_NONE;
}

// Read row by row and immediately send to the screen
uint8_t Button::DrawBMP(LCDWIKI_KBV* my_lcd) const
{
	if (!IsInitialized())
		return DEF_ERR_ACCESS_TO_NOT_INITIALIZED_BMP;

	//const uint32_t time = millis();

	File f;
	const bool success = f.open(GetBMPPath().c_str());
	if (!success)
	{
		return DEF_ERR_CANT_OPEN_FILE;
	}

	f.seek(GetBMPOffset());

	const uint8_t bytesPerPixel = GetBMPDepth() / 8;
	const int16_t paddedWidth = (GetBMPWidth() * bytesPerPixel + 3) & ~3;

	auto data = (uint8_t*) mallocWrapper(paddedWidth);
	if (data == NULL)
		return DEF_ERR_NOT_ENOUGH_MEMORY;

	// TODO was only tested with width % 4 == 0
	//Serialprint("Width = %d, Padded width = %d\n", GetBMPWidth() * bytesPerPixel, paddedWidth);

	auto colors = (uint16_t*) mallocWrapper(GetBMPWidth() * sizeof(uint16_t));
	if (colors == NULL)
	{
		free(data);
		return DEF_ERR_NOT_ENOUGH_MEMORY;
	}

	for (size_t rowNum = 0; rowNum < GetBMPHeight(); ++rowNum)
	{
		size_t m = 0;
		f.readBytes(data, paddedWidth);

		switch (bytesPerPixel)
		{
		case 3:
			{
				for (size_t i = 0; i < GetBMPWidth(); ++i)
				{
					colors[i] = my_lcd->Color_To_565(data[m + 2], data[m + 1], data[m + 0]);
					m += bytesPerPixel;
				}
				break;
			}
		case 2:
			{
				for (size_t i = 0; i < GetBMPWidth(); ++i)
				{
					/*
					if (data[m] > 0) {
					  Serialprint("data[%d] = %d binary = ", m, data[m]);
					  PrintBinary(data[m]);
		  
					  Serialprint("data[%d] = %d binary = ", m+1, data[m+1]);
					  PrintBinary(data[m+1]);
		  
					  uint16_t combined = (data[m+1] << 8) | data[m];
					  Serialprint("Combined = %d binary = ", combined);
					  PrintBinary(combined);
		  
					  colors[i] = 0;
					  uint8_t* pointer = (uint8_t*) &colors[i];
					  Serialprint("Address of [0] = %p Address of [1] = %p", &(pointer[0]), &(pointer[1]));
					  pointer[0] = data[m];
					  pointer[1] = data[m+1];
					  Serial.println();
					  PrintBinary(colors[i]);
		  
					  delay(100000000);
					}
					*/

					const auto pointer = (uint8_t*)&colors[i];
					pointer[0] = data[m + 1];
					pointer[1] = data[m];
					m += bytesPerPixel;
				}
				break;
			}
		}

		if (_bmpIsFlipped)
			my_lcd->Set_Addr_Window(GetBMPX(), GetBMPY() + rowNum, GetBMPX() + GetBMPWidth() - 1, GetBMPY() + rowNum);
		else
			my_lcd->Set_Addr_Window(GetBMPX(), GetBMPY() + GetBMPHeight() - rowNum, GetBMPX() + GetBMPWidth() - 1,
			                        GetBMPY() + GetBMPHeight() - rowNum);

		my_lcd->Push_Any_Color(colors, GetBMPWidth(), true, DEF_LCDWIKI_KBV_FLAGS_NONE);
	}

	f.close();
	free(data);
	data = NULL;
	free(colors);
	colors = NULL;

	//Serialprint("Drawing took %d\n", millis() - time);
	return DEF_ERR_NONE;
}

void Button::DrawSelection(LCDWIKI_KBV* my_lcd) const
{
	if (IsSelected())
		my_lcd->Set_Draw_color(GREEN);
	else
		my_lcd->Set_Draw_color(BLACK);

	if (IsInitialized())
	{
		for (uint16_t i = 1; i < 8; ++i)
		{
			const uint16_t x0 = GetBMPX() - i;
			const uint16_t y0 = GetBMPY() - i;
			const uint16_t x1 = x0 + GetBMPWidth() + i * 2;
			const uint16_t y1 = y0 + GetBMPHeight() + i * 2;
			my_lcd->Draw_Rectangle(x0, y0, x1, y1);
		}
	}
	else
	{
		for (uint16_t i = 1; i < 8; ++i)
		{
			my_lcd->Draw_Round_Rectangle(GetX1() + i, GetY1() + i, GetX2() - i, GetY2() - i,
			                             DEF_BUTTON_RECTANGLE_ROUNDNESS);
		}
	}
}

void Button::DrawRectangle(LCDWIKI_KBV* my_lcd, const char* text) const
{
	my_lcd->Set_Text_Size(2);
	my_lcd->Set_Text_colour(GREEN);
	my_lcd->Set_Text_Back_colour(BLACK);

	if (text != NULL)
	{
		if (GetType() == DEF_BUTTON_TYPE_NEXT)
			my_lcd->Print_String(">", GetX1() + GetWidth() / 5, GetY1() + GetHeight() / 2);
		else if (GetType() == DEF_BUTTON_TYPE_PREV)
			my_lcd->Print_String("<", GetX1() + GetWidth() / 5, GetY1() + GetHeight() / 2);
		else if (GetType() == DEF_BUTTON_TYPE_SELECT_ALL)
			my_lcd->Print_String("A", GetX1() + GetWidth() / 5, GetY1() + GetHeight() / 2);
		else
			my_lcd->Print_String(text, GetX1() + GetWidth() / 5, GetY1() + GetHeight() / 2);
	}

	my_lcd->Set_Draw_color(WHITE);
	my_lcd->Draw_Round_Rectangle(GetX1(), GetY1(), GetX2(), GetY2(), DEF_BUTTON_RECTANGLE_ROUNDNESS);
}

void Button::SendKeys() const
{
	const String keys = GetKeys();
	int index = keys.indexOf('[');
	while (index >= 0)
	{
		String str = keys.substring(index + 1, keys.indexOf(']', index));

		uint16_t key = 0;
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
		else if (str.length() == 1)
		{
			key = str[0];
		}

		//Serialprint("str = '%s', key = %x\n", str.c_str(), key);
		if (key > 0)
		{
			Keyboard.press(key);
			delay(DEF_KEY_SEND_DELAY);
			Keyboard.release(key);
			delay(DEF_KEY_SEND_DELAY);
		}

		index = keys.indexOf('[', index + 1);
	}
}
