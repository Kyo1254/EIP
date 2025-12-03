#pragma once

//#include "pch.h"

#ifdef IMAGECORE_EXPORTS
#define IMAGECORE_API __declspec(dllexport)
#else
#define IMAGECORE_API __declspec(dllimport)
#endif

extern "C" {
	// 뷰어 창의 클래스 이름 정의
	const wchar_t* const VIEWER_CLASS_NAME = L"NativeImageViewerClass";

	// 초기화 및 종료 함수
	IMAGECORE_API void InitializeImageCore();
	IMAGECORE_API void TerminateImageCore();

	// 뷰어 창 생성 및 핸들 반환 함수
	IMAGECORE_API HWND CreateImageViewer(HWND hParent);

	// 뷰어 창 핸들 획득 함수
	IMAGECORE_API HWND GetViewerHandle();

	IMAGECORE_API void SetViewerPosition(int x, int y, int width, int height);

	// 이미지 로드 함수
	IMAGECORE_API bool LoadImageFileW(const wchar_t* filePath);

	// 뷰어 컨트롤 함수
	IMAGECORE_API void RequestViewerRedraw();

	// 이미지 Zoom 관련 함수
	IMAGECORE_API void ZoomIn(float zoomStep);
	IMAGECORE_API void ZoomOut(float zoomStep);
	IMAGECORE_API void ResetZoom();
}