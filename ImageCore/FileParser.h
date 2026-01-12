#pragma once

#include <string>
#include <fstream>
#include <Windows.h>

#pragma pack(push, 1)
// 파일 확장자 추출 함수
std::string GetFileExtension(const std::string& filePath);

struct BmpDibHeader {
    DWORD biSize;         // 4 bytes
    LONG biWidth;         // 4 bytes
    LONG biHeight;        // 4 bytes
    WORD biPlanes;        // 2 bytes
    WORD biBitCount;      // 2 bytes
    DWORD biCompression;  // 4 bytes
    DWORD biSizeImage;    // 4 bytes
    LONG biXPelsPerMeter; // 4 bytes
    LONG biYPelsPerMeter; // 4 bytes
    DWORD biClrUsed;      // 4 bytes
    DWORD biClrImportant; // 4 bytes
};

struct TiffHeader {
	WORD ByteOrder;
	WORD Version;
	DWORD IFDOffset;
};

struct TiffIFDEntry {
	WORD TagID;
	WORD DataType;
	DWORD DataCount;
	DWORD ValueOffset;
};

#pragma pack(pop)

// 포맷별 파싱 함수
int ParseBmpHeader(std::ifstream& file);
int ParseTiffHeader(std::ifstream& file);
int ParseJpegHeader(std::ifstream& file);

int GetBitDepthFromFile(const std::string& filePath);

inline unsigned short SwapBytes(unsigned short val)
{
	return (val >> 8) | (val << 8);
}