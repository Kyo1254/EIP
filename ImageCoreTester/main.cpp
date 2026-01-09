#include <windows.h>
#include <iostream>
#include <tchar.h>
#include <stdio.h>
#include <filesystem>

typedef HWND(*CREATE_VIEWER_FUNC)(HWND);
typedef HWND(*GET_HANDLE_FUNC)();
typedef void (*INIT_CORE_FUNC)();
typedef bool (*LOAD_IMAGE_FUNC)(const wchar_t*);
typedef void (*REQUEST_REDRAW_FUNC)();
typedef void (*SET_POS_FUNC)(int, int, int, int);

typedef HWND(*GET_VIEWER_HANDLE_FUNC)();

typedef void (*ZOOM_IN_FUNC)(float);
typedef void (*ZOOM_OUT_FUNC)(float);
typedef void (*RESET_ZOOM_FUNC)();

ZOOM_IN_FUNC g_pZoomIn = NULL;
ZOOM_OUT_FUNC g_pZoomOut = NULL;
RESET_ZOOM_FUNC g_pResetZoom = NULL;

GET_VIEWER_HANDLE_FUNC pGetViewerHandle = NULL;

LRESULT CALLBACK TestParentWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:
	{
		if (pGetViewerHandle)
		{
			HWND hViewer = pGetViewerHandle();
			if (hViewer)
			{
				PostMessage(hViewer, message, wParam, lParam);
			}
		}
		return 0;
	}
	case WM_SIZE:
	{
		int width = LOWORD(lParam);
		int height = HIWORD(lParam);

		HWND hViewer = pGetViewerHandle();
		if (hViewer)
		{
			MoveWindow(hViewer, 0, 0, width, height, TRUE);
		}
		return 0;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	std::filesystem::path currentPath = std::filesystem::current_path();

	
	// DLL 로드
	HMODULE hImageCoreDLL = LoadLibrary(_T("ImageCore.dll"));

	if (hImageCoreDLL == NULL)
	{
		DWORD dwError = GetLastError();
		TCHAR szError[256];
		_stprintf_s(szError, 256, _T("LoadLibrary 실패 (ImageCore.dll). 오류 코드: %d"), dwError);
		MessageBox(NULL, szError, _T("DLL 로드 오류"), MB_OK | MB_ICONERROR);

		return 1;
	}


	// 함수 포인터 획득
	INIT_CORE_FUNC pInitialize = (INIT_CORE_FUNC)GetProcAddress(hImageCoreDLL, "InitializeImageCore");
	CREATE_VIEWER_FUNC pCreateViewer = (CREATE_VIEWER_FUNC)GetProcAddress(hImageCoreDLL, "CreateImageViewer");
	LOAD_IMAGE_FUNC pLoadImageW = (LOAD_IMAGE_FUNC)GetProcAddress(hImageCoreDLL, "LoadImageFileW");

	ZOOM_IN_FUNC pZoomIn = (ZOOM_IN_FUNC)GetProcAddress(hImageCoreDLL, "ZoomIn");
	ZOOM_OUT_FUNC pZoomOut = (ZOOM_OUT_FUNC)GetProcAddress(hImageCoreDLL, "ZoomOut");
	RESET_ZOOM_FUNC pResetZoom = (RESET_ZOOM_FUNC)GetProcAddress(hImageCoreDLL, "ResetZoom");

	SET_POS_FUNC pSetVeiwerPosition = (SET_POS_FUNC)GetProcAddress(hImageCoreDLL, "SetViewerPosition");

	pGetViewerHandle = (GET_VIEWER_HANDLE_FUNC)GetProcAddress(hImageCoreDLL, "GetViewerHandle");

	if (!pCreateViewer)
	{
		MessageBox(NULL, _T("CreateImageViewer 함수를 찾을 수 없습니다."), _T("Error"), MB_OK | MB_ICONERROR);
		FreeLibrary(hImageCoreDLL);
		return 1;
	}

	// ---------------------------------------------------------------
	// 테스트 실행
	// ---------------------------------------------------------------

	// C++ Core 초기화
	if (pInitialize)
		pInitialize();

	// 테스트용 부모 창 생성

	const TCHAR* TEST_WINDOW_CLASS = _T("IMageCoreTesterParentClass");
	WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
	wc.lpfnWndProc = TestParentWndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = TEST_WINDOW_CLASS;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

	if(!RegisterClassEx(&wc))
	{
		MessageBox(NULL, _T("테스트 부모 창 클래스를 등록할 수 없습니다."), _T("Error"), MB_OK | MB_ICONERROR);
		FreeLibrary(hImageCoreDLL);
		return 1;
	}

	HWND hTestParent = CreateWindowEx(
		0, TEST_WINDOW_CLASS, _T("Test Parent Window"), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		100, 100, 800, 600, NULL, NULL, hInstance, NULL);

	// 뷰어 생성 함수 호출
	HWND hViewer = pCreateViewer(hTestParent);

	if (hViewer == NULL)
	{
		MessageBox(hTestParent, _T("Image Viewer 생성 실패"), _T("Error"), MB_OK | MB_ICONERROR);
	}
	else
	{
		// 뷰어 위치 설정 테스트
		if (pSetVeiwerPosition)
		{
			pSetVeiwerPosition(50, 50, 700, 500);
		}

		// 이미지 로드 테스트
		if (pLoadImageW)
		{
			pLoadImageW(_T("C:\\Dev\\Image\\osaka.jpg"));
		}

		// 메시지 루프 실행
		MSG msg = { 0 };
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	FreeLibrary(hImageCoreDLL);
	return 0;
}