
#include <Arduino.h>
#include <SdFat.h>
#include <LCDWIKI_KBV.h> //Hardware-specific library

#ifndef BUTTON_H
#define BUTTON_H

#pragma pack(push, 1) // Disable padding for easier BMP file reading

struct BMPFileHeader {
  uint16_t file_type{0x4D42};          // File type always BM which is 0x4D42
  uint32_t file_size{0};               // Size of the file (in bytes)
  uint16_t reserved1{0};               // Reserved, always 0
  uint16_t reserved2{0};               // Reserved, always 0
  uint32_t offset_data{0};             // Start position of pixel data (bytes from the beginning of the file)
};

struct BMPInfoHeader {
  uint32_t size{ 0 };                      // Size of this header (in bytes)
  int32_t width{ 0 };                      // width of bitmap in pixels
  int32_t height{ 0 };                     // width of bitmap in pixels
                                           //       (if positive, bottom-up, with origin in lower left corner)
                                           //       (if negative, top-down, with origin in upper left corner)
  uint16_t planes{ 1 };                    // No. of planes for the target device, this is always 1
  uint16_t bit_count{ 0 };                 // No. of bits per pixel
  uint32_t compression{ 0 };               // 0 or 3 - uncompressed. THIS PROGRAM CONSIDERS ONLY UNCOMPRESSED BMP images
  uint32_t size_image{ 0 };                // 0 - for uncompressed images
  int32_t x_pixels_per_meter{ 0 };
  int32_t y_pixels_per_meter{ 0 };
  uint32_t colors_used{ 0 };               // No. color indexes in the color table. Use 0 for the max number of colors allowed by bit_count
  uint32_t colors_important{ 0 };          // No. of colors used for displaying the bitmap. If 0 all colors are required
};

struct BMPColorHeader {
  uint32_t red_mask{ 0x00ff0000 };         // Bit mask for the red channel
  uint32_t green_mask{ 0x0000ff00 };       // Bit mask for the green channel
  uint32_t blue_mask{ 0x000000ff };        // Bit mask for the blue channel
  uint32_t alpha_mask{ 0xff000000 };       // Bit mask for the alpha channel
  uint32_t color_space_type{ 0x73524742 }; // Default "sRGB" (0x73524742)
  uint32_t unused[16]{ 0 };                // Unused data for sRGB color space
};

#pragma pack(pop)

class Button {
  private:
    BMPFileHeader _file_header;
    BMPInfoHeader _bmp_info_header;
    BMPColorHeader _bmp_color_header;

    String *_path, *_keys;
    int8_t _type;
    bool _isInitialized, _selected, _onScreen, _bmpIsFlipped;
    int16_t _bmpX, _bmpY, _x1, _x2, _y1, _y2;
    
  public: 
    Button() {
      _path = NULL;
      _keys = NULL;
      _isInitialized = false;
      _selected = false;
      _onScreen = false;
      _bmpIsFlipped = false;
      _type = DEF_BUTTON_TYPE_UNDEFINED;
    }
    
    bool IsInitialized() { return _isInitialized; }
    bool IsSelected() { return _selected; }
    bool IsOnScreen() { return _onScreen; }
    void SetSelected(bool selected) { _selected = selected; }
    void SetOnScreen(bool onScreen) { _onScreen = onScreen; }
    void SetType(uint8_t type) { _type = type; }
    uint8_t GetType() { return _type; }
    void SetKeys(String* keys) { _keys = keys; }
    String GetKeys() {  if (_keys == NULL) return ""; return *_keys; }
    void SetBMPPath(String* path) { _path = path; }
    String GetBMPPath() { if (_path == NULL) return ""; return *_path; }
    uint8_t GetBMPDepth() { return _bmp_info_header.bit_count; } 
    int16_t GetBMPWidth() { return _bmp_info_header.width; }
    int16_t GetBMPHeight() { return _bmp_info_header.height; }
    int16_t GetBMPX() { return _bmpX; }
    int16_t GetBMPY() { return _bmpY; }
    int16_t SetBMPXY(int16_t bmpX, int16_t bmpY) { _bmpX = bmpX; _bmpY = bmpY; }
    int16_t GetX1() { return _x1; }
    int16_t GetX2() { return _x2; }
    int16_t GetY1() { return _y1; }
    int16_t GetY2() { return _y2; }
    int16_t SetXY(int16_t x1, int16_t y1, int16_t x2, int16_t y2) { _x1 = x1; _x2 = x2; _y1 = y1; _y2 = y2; }
    
    uint8_t InitBMP(String* path);
    uint8_t DrawBMP(LCDWIKI_KBV my_lcd);
    void DrawSelection(LCDWIKI_KBV my_lcd);
    bool IsHit(uint16_t x, uint16_t y) { return GetX1() < x && x < GetX2() && GetY1() < y && y < GetY2(); }
    void SendKeys();
};

#endif 