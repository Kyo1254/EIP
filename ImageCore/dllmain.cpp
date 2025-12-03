// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#include "pch.h"
#include "ImageCore.h"

// 전역, 정적 변수
HWND g_hWndViewer = NULL;
cv::Mat g_currentImage;
cv::Mat g_displayImage;

// Image Scale 및 렌더링 관련 변수
double g_scaleFactor = 1.0;
int g_offsetX = 0;
int g_offsetY = 0;

// Panning 관련 변수
bool g_isPanning = false;
POINT g_lastMousePos = { 0, 0 };

// 드롭된 파일 경로 목록
std::vector<std::wstring> g_fileList;
int g_currentIndex = -1;

struct BitmapInfo
{
    BITMAPINFOHEADER header;
    RGBQUAD palette[256];
};

// std::wstring을 UTF-8 인코딩의 std::string으로 변환하는 헬퍼 함수
std::string ConvertWStringToStringUTF8(const std::wstring& wstr)
{
    if(wstr.empty())
		return std::string();

    // 1. 필요한 버퍼 크기 계산
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);

	// 2. 버퍼에 변환된 문자열 저장
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), &strTo[0], size_needed, NULL, NULL);

    return strTo;
}


// 전역 뷰어 창 크기를 얻기 위한 헬퍼 함수
void GetViewerWindowSize(HWND hWnd, int& width, int& height)
{
    RECT rect;
	GetClientRect(hWnd, &rect);
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;
}

void ApplyZoomAndRedraw()
{
    if (g_hWndViewer == NULL || g_displayImage.empty())
        return;

	int width, height;
    GetViewerWindowSize(g_hWndViewer, width, height);
	
	int scaledW = (int)(g_displayImage.cols * g_scaleFactor);
	int scaledH = (int)(g_displayImage.rows * g_scaleFactor);

	g_offsetX = (width - scaledW) / 2;
    g_offsetY = (height - scaledH) / 2;

	RequestViewerRedraw();
}

void CalculateFitParams(int windowWidth, int windowHeight)
{
    if (g_displayImage.empty())
        return;

    int imgW = g_displayImage.cols;
    int imgH = g_displayImage.rows;

    // 1. 가로/세로 비율 계산
    double scaleX = (double)windowWidth / imgW;
    double scaleY = (double)windowHeight / imgH;

    // 2. Fit을 위해 더 작은 비율 선택
    g_scaleFactor = std::min(scaleX, scaleY);

    // 3. 중앙 정렬을 위한 오프셋 계산
    int scaledW = (int)(imgW * g_scaleFactor);
    int scaledH = (int)(imgH * g_scaleFactor);

    g_offsetX = (windowWidth - scaledW) / 2;
    g_offsetY = (windowHeight - scaledH) / 2;
}

// 리스트의 특정 인덱스에 있는 이미지를 로드하는 내부 헬퍼 함수
bool LoadImageByIndex(int index)
{
    if (index < 0 || index >= g_fileList.size())
        return false;

    g_currentIndex = index;

    const wchar_t* widePath = g_fileList[g_currentIndex].c_str();

    char multiBytePath[MAX_PATH * 2];
	WideCharToMultiByte(CP_ACP, 0, widePath, -1, multiBytePath, sizeof(multiBytePath), NULL, NULL);

	g_currentImage = cv::imread(multiBytePath, cv::IMREAD_COLOR);

    if (g_currentImage.empty())
        return false;

    g_displayImage = g_currentImage.clone();

    if (g_hWndViewer)
    {
        RECT rect;
		GetClientRect(g_hWndViewer, &rect);
		CalculateFitParams(rect.right - rect.left, rect.bottom - rect.top);
    }

	RequestViewerRedraw();
    return true;
}

IMAGECORE_API void ZoomIn(float zoomStep)
{
	g_scaleFactor *= (1.0 + zoomStep);
    
	ApplyZoomAndRedraw();
}

IMAGECORE_API void ZoomOut(float zoomStep)
{
    g_scaleFactor /= (1.0 + zoomStep);
    
    ApplyZoomAndRedraw();
}

IMAGECORE_API void ResetZoom()
{
	g_scaleFactor = 1.0;
    
    ApplyZoomAndRedraw();
}


// ---------------------------------------------------------------
// Win32 API 뷰어 창 프로시저 (WndProc)
// ---------------------------------------------------------------
LRESULT CALLBACK ViewerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DROPFILES:
    {
        HDROP hDrop = (HDROP)wParam;
        UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);

        if (fileCount > 0)
        {
            // 기존 리스트 초기화
			g_fileList.clear();

            // 드롭된 모든 파일 경로를 리스트에 저장
            for (UINT i = 0; i < fileCount; ++i)
            {
                wchar_t widePath[MAX_PATH];
				DragQueryFile(hDrop, i, widePath, MAX_PATH);
                g_fileList.push_back(widePath);
            }
            LoadImageByIndex(0);
        }
		DragFinish(hDrop);
    }
    return 0;

    case WM_MOUSEWHEEL:
    {
        short delta = HIWORD(wParam);

        // 1. 마우스 포인터의 클라이언트 좌표 획득
		POINT ptScreen = { LOWORD(lParam), HIWORD(lParam) };
		ScreenToClient(hWnd, &ptScreen);
		int mouseX = ptScreen.x;
		int mouseY = ptScreen.y;

		// 2. 현재 스케일과 오프셋을 사용하여 마우스 위치의 이미지 좌표 계산
        double imgX = (mouseX - g_offsetX) / g_scaleFactor;
		double imgY = (mouseY - g_offsetY) / g_scaleFactor;

        // 3. 새로운 스케일 계산
		double oldScale = g_scaleFactor;
        double zoomFactor = 0.1;

        if (delta > 0) // Zoom In
        {
            g_scaleFactor *= (1.0 + zoomFactor);
        }
        else if (delta < 0) // Zoom Out
        {
            g_scaleFactor /= (1.0 + zoomFactor);
		}

		g_scaleFactor = std::max(g_scaleFactor, 0.2); // 최소 스케일 제한
		g_scaleFactor = std::min(g_scaleFactor, 128.0); // 최대 스케일 제한


		//4 . 새로운 오프셋 계산 (마우스 위치 고정)
		g_offsetX = (int)(mouseX - (imgX * g_scaleFactor));
		g_offsetY = (int)(mouseY - (imgY * g_scaleFactor));

        RequestViewerRedraw();

        return 0;
    }
    
    case WM_LBUTTONDOWN:
    {
        if(g_displayImage.empty())
			break;

        SetCapture(hWnd);

		g_isPanning = true;

		g_lastMousePos.x = LOWORD(lParam);
        g_lastMousePos.y = HIWORD(lParam);
		return 0;
    }

    case WM_MOUSEMOVE:
    {
        if (!g_isPanning || g_displayImage.empty())
            break;

		int currentX = LOWORD(lParam);
		int currentY = HIWORD(lParam);

		int deltaX = currentX - g_lastMousePos.x;
		int deltaY = currentY - g_lastMousePos.y;

		g_offsetX += deltaX;
		g_offsetY += deltaY;

		g_lastMousePos.x = currentX;
        g_lastMousePos.y = currentY;
        
        RequestViewerRedraw();
        
		return 0;
    }

    case WM_LBUTTONUP:
    {
        if (g_isPanning)
        {
            ReleaseCapture();

			g_isPanning = false;
        }
        return 0;
    }

    case WM_CAPTURECHANGED:
    {
        if ((HWND)lParam != hWnd && g_isPanning)
        {
			g_isPanning = false;
        }
        break;
    }


    case WM_MBUTTONDOWN:
    {
        ResetZoom();
        
        return 0;
    }
    
    case WM_KEYDOWN:
    {
        if (g_fileList.empty())
            break;

		int newIndex = g_currentIndex;

        // page up/down 키 처리
        if (wParam == VK_PRIOR) // Page Up
        {
            newIndex--;
        }
        else if(wParam == VK_NEXT) // Page Down
        {
            newIndex++;
		}

        // 인덱스 범위 확인 및 조정
        if (newIndex >= 0 && newIndex < g_fileList.size())
        {
			LoadImageByIndex(newIndex);
        }
        else if (newIndex < 0)
        {
            LoadImageByIndex(g_fileList.size() - 1);
        }
        else if (newIndex >= g_fileList.size())
        {
			LoadImageByIndex(0);
        }
    }
    return 0;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // 이미지 뷰어 렌더링 (OpenCV Mat -> Win32 HDC)
        if (!g_displayImage.empty())
        {
            RECT clientRect;
			GetClientRect(hWnd, &clientRect);
			int winWidth = clientRect.right - clientRect.left;
			int winHeight = clientRect.bottom - clientRect.top;

            // 1. 메모리 DC 및 비트맵 생성 (이중 버퍼링)
			HDC hMemDC = CreateCompatibleDC(hdc);
			HBITMAP hBitmap = CreateCompatibleBitmap(hdc, winWidth, winHeight);
			HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

			SetStretchBltMode(hMemDC, HALFTONE);

            // 2. 메모리 DC의 배경 지우기
			HBRUSH hBrush = GetSysColorBrush(COLOR_WINDOW);
			FillRect(hMemDC, &clientRect, hBrush);
			DeleteObject(hBrush);

            // 3. 이미지 렌더링
            static BitmapInfo bmi_storage;
            BITMAPINFO* bmi = (BITMAPINFO*)&bmi_storage;         

            int channels = g_displayImage.channels();
            int bitCount = 0; // 픽셀당 비트 수

            ZeroMemory(&bmi_storage, sizeof(bmi_storage));
            bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi->bmiHeader.biWidth = g_displayImage.cols;
            bmi->bmiHeader.biHeight = -g_displayImage.rows; // 상단 좌표계
            bmi->bmiHeader.biPlanes = 1;
            bmi->bmiHeader.biCompression = BI_RGB;

            if (channels == 3)
            {
                bitCount = 24;
                bmi->bmiHeader.biClrUsed = 0;
            }
            else if (channels == 1)
            {
                bitCount = 8;
                bmi->bmiHeader.biClrUsed = 256;

                for (int i = 0; i < 256; i++)
                {
                    bmi->bmiColors[i].rgbRed = (BYTE)i;
                    bmi->bmiColors[i].rgbGreen = (BYTE)i;
                    bmi->bmiColors[i].rgbBlue = (BYTE)i;
                    bmi->bmiColors[i].rgbReserved = 0;
                }
            }
            else
            {
                MessageBox(hWnd, L"지원하지 않는 이미지 채널 수입니다.", L"Error", MB_ICONERROR);
                EndPaint(hWnd, &ps);
                return 0;
            }
            bmi->bmiHeader.biBitCount = bitCount;

            // StretchDIBits 호출 (화면에 그리기)
            StretchDIBits(
                hMemDC,
                // Destination Image
                g_offsetX,
                g_offsetY,
                (int)(g_displayImage.cols * g_scaleFactor),
                (int)(g_displayImage.rows * g_scaleFactor),

                // Source Image
                0, 0, g_displayImage.cols, g_displayImage.rows,

                g_displayImage.data,
                bmi,
                DIB_RGB_COLORS,
                SRCCOPY
            );

			// 4. 메모리 DC의 내용을 화면 HDC로 복사
			BitBlt(hdc, 0, 0, winWidth, winHeight, hMemDC, 0, 0, SRCCOPY);

			// 5. 자원 해제
			SelectObject(hMemDC, hOldBitmap);
			DeleteObject(hBitmap);
			DeleteDC(hMemDC);
        }
        EndPaint(hWnd, &ps);
        return 0;
    }
    

    case WM_SIZE:
        // WPF HwndHost의 크기가 변하면 메시지 전달
        // 뷰어의 렌더링 로직을 실행하고 갱신 요청
        if (wParam != SIZE_MINIMIZED)
        {
            int width = LOWORD(lParam);
			int height = HIWORD(lParam);    

			CalculateFitParams(width, height);

            InvalidateRect(hWnd, NULL, TRUE);
        }
        
        return 0;
    case WM_DESTROY:
        g_hWndViewer = NULL;
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

// ---------------------------------------------------------------
// 초기화 및 뷰어 클래스 등록
// ---------------------------------------------------------------
BOOL RegisterViewerClass(HINSTANCE hInstance)
{
    WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ViewerWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = VIEWER_CLASS_NAME;

    return RegisterClassEx(&wc);
}

IMAGECORE_API void InitializeImageCore()
{
    // C++ Core 초기화
}

IMAGECORE_API void TerminateImageCore()
{
    // C++ Core 자원 해제
    if (g_hWndViewer)
    {
		DestroyWindow(g_hWndViewer);
    }
}

// ---------------------------------------------------------------
// 뷰어 창 생성 및 통합 함수
// ---------------------------------------------------------------
IMAGECORE_API HWND CreateImageViewer(HWND hParent)
{
    if (g_hWndViewer != NULL)
        return g_hWndViewer;

    HINSTANCE hInstance = GetModuleHandle(NULL);

    // 윈도우 클래스 등록
    if (!RegisterViewerClass(hInstance))
        return NULL;

	// 뷰어 창 생성
    g_hWndViewer = CreateWindowEx(
        0,                      // 확장 윈도우 스타일
        VIEWER_CLASS_NAME,      // 클래스 이름
        L"",                   // 윈도우 제목
        WS_CHILD | WS_VISIBLE,  // 윈도우 스타일: 자식 윈도우로 생성 및 보이기
        0, 0,                   // 위치
        800, 600,               // 크기 (초기값)
        hParent,                // 부모 윈도우 핸들
        NULL,                   // 메뉴 핸들
        hInstance,              // 인스턴스 핸들
        NULL                    // 추가 매개변수
    );

    if (g_hWndViewer)
    {
        DragAcceptFiles(g_hWndViewer, TRUE);
    }

    return g_hWndViewer;
}

// ---------------------------------------------------------------
// 뷰어 핸들 획듳 및 영상 처리 함수 0
// ---------------------------------------------------------------
IMAGECORE_API HWND GetViewerHandle()
{
	return g_hWndViewer;
}

IMAGECORE_API void SetViewerPosition(int x, int y, int width, int height)
{
    if (g_hWndViewer == NULL)
		return;

	SetWindowPos(g_hWndViewer, NULL, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    
}

IMAGECORE_API bool LoadImageFileW(const wchar_t* filePath)
{
    std::wstring wPath(filePath);

	std::string utf8Path = ConvertWStringToStringUTF8(wPath);

    g_currentImage = cv::imread(utf8Path, cv::IMREAD_COLOR);

    if (g_currentImage.empty())
		return false;

    g_displayImage = g_currentImage.clone();

    if (g_hWndViewer)
    {
        RECT rect;
		GetClientRect(g_hWndViewer, &rect);
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;    

		CalculateFitParams(width, height);
    }

    RequestViewerRedraw();
    return true;    
}

// ---------------------------------------------------------------
// 뷰어 컨트롤 함수
// ---------------------------------------------------------------
IMAGECORE_API void RequestViewerRedraw()
{
    if (g_hWndViewer)
    {
        InvalidateRect(g_hWndViewer, NULL, TRUE);
    }
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

