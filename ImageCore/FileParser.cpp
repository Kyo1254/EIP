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
		bitDepth = 24;
	}

	file.close();
	return bitDepth;
}