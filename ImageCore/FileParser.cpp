#include "pch.h"
#include "FileParser.h"

#include <algorithm>
#include <vector>

std::string GetFileExtension(const std::string& filePath) {
	size_t dotPos = filePath.find_last_of('.');
	if (dotPos != std::string::npos)
	{
		std::string ext = filePath.substr(dotPos + 1);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		return ext;
	}
	return "";
}

int ParseBmpHeader(std::ifstream& file) {
	file.seekg(14, std::ios::beg);
	BmpDibHeader header;
	if (!file.read((char*)&header, sizeof(BITMAPINFOHEADER))) return 0;
	return (int)header.biBitCount;
}

int ParseTiffHeader(std::ifstream& file)
{
	TiffHeader header;
	if (!file.read((char*)&header, sizeof(TiffHeader))) return 0;

	file.seekg(header.IFDOffset, std::ios::beg);
	WORD numEntries;
	if (!file.read((char*)&numEntries, sizeof(WORD))) return 0;

	TiffIFDEntry entry;
	int bitDepth = 0;

	for (int i = 0; i < numEntries; ++i)
	{
		if (!file.read((char*)&entry, sizeof(TiffIFDEntry))) break;
		if (entry.TagID == 0x0102)
		{
			if (entry.DataType == 3 && entry.DataCount == 1)
			{
				bitDepth = (int)(entry.ValueOffset & 0xFFFF);
			}
			else if (entry.DataType == 3 && entry.DataCount > 1)
			{
				bitDepth = 8 * (int)entry.DataCount;
			}
			return bitDepth;
		}
	}
	return 0;
}

int ParseJpegHeader(std::ifstream& file)
{
	file.seekg(2, std::ios::beg);

	unsigned short marker = 0;
	while (!file.eof())
	{
		if (!file.read((char*)&marker, sizeof(marker)))break;

		marker = SwapBytes(marker);

		if ((marker & 0xFF00) != 0xFF00)
		{
			file.seekg(-1, std::ios::cur);
			continue;
		}

		if (marker >= 0xFFC0 && marker <= 0xFFCF &&
			marker != 0xFFC4 && marker != 0xFFC8 && marker != 0xFFCC)
		{
			unsigned short length = 0;
			if (!file.read((char*)&length, sizeof(length))) return 24;

			length = SwapBytes(length);

			unsigned char precision = 0;
			unsigned short height = 0;
			unsigned short width = 0;
			unsigned char numComponents = 0;

			if (!file.read((char*)&precision, sizeof(precision))) return 24;

			if (!file.read((char*)&height, sizeof(height))) return 24;
			if (!file.read((char*)&width, sizeof(width))) return 24;
			if (!file.read((char*)&numComponents, sizeof(numComponents))) return 24;

			return (int)precision * (int)numComponents;
		}
		else
		{
			unsigned short length = 0;
			if (!file.read((char*)&length, sizeof(length))) break;

			length = SwapBytes(length);

			if (length > 2)
			{
				file.seekg(length - 2, std::ios::cur);
			}
			else
			{
				break;
			}
		}
	}

	return 24;
}

int GetBitDepthFromFile(const std::string& filePath)
{
	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open()) return 0;

	std::string ext = GetFileExtension(filePath);
	int bitDepth = 0;

	if (ext == "bmp")
	{
		bitDepth = ParseBmpHeader(file);
	}
	else if (ext == "tif" || ext == "tiff")
	{
		bitDepth = ParseTiffHeader(file);
	}
	else if (ext == "jpg" || ext == "jpeg")
	{
		bitDepth = ParseJpegHeader(file);
	}

	file.close();
	return bitDepth;
}