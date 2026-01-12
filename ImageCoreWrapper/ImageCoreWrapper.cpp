/*
* Image Core Wrapper
* C++/CLI를 사용하여 C++ 기반의 Image Core 라이브러리를 .NET 환경에서 사용할 수 있도록 래핑하는 코드입니다.
* -> ImageCoreWrapper.dll 생성
*/

#include "pch.h"
#include <windows.h>
#include <string>
#include "ImageCoreWrapper.h"

using namespace ImageCoreWrapper;
using namespace System;
using namespace System::Runtime::InteropServices;

std::string MarshalString(String^ s)
{
	IntPtr pString = Marshal::StringToHGlobalAnsi(s);

	std::string nativeString((const char*)pString.ToPointer());

	Marshal::FreeHGlobal(pString);
	
	return nativeString;
}

ImageCore::ImageCore()
{
	m_hViewer = System::IntPtr::Zero;
}

void ImageCore::Initialize()
{
	InitializeImageCore();
}

void ImageCore::Terminate()
{
	TerminateImageCore();
}

System::IntPtr ImageCore::CreateViewer(System::IntPtr hParent)
{
	if(m_hViewer != System::IntPtr::Zero)
		return m_hViewer;

	HWND nativeHParent = (HWND)hParent.ToPointer();

	HWND nativeViewer = CreateImageViewer(nativeHParent);

	m_hViewer = (nativeViewer != NULL) ? (System::IntPtr)nativeViewer : System::IntPtr::Zero;

	return m_hViewer;
}

void ImageCore::SetViewerPos(int x, int y, int width, int height)
{
	SetViewerPosition(x, y, width, height);
}

void ImageCore::RequestRedraw()
{
	RequestViewerRedraw();
}

bool ImageCore::LoadImgFile(String^ filePath)
{
	std::string nativePath = MarshalString(filePath);

	return LoadImageFile(nativePath);
}

void ImageCore::ViewerZoomIn(float zoomStep)
{
	ZoomIn(zoomStep);
}
void ImageCore::ViewerZoomOut(float zoomStep)
{
	ZoomOut(zoomStep);
}

void ImageCore::ViewerResetZoom()
{
	ResetZoom();
}