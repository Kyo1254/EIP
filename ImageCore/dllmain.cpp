// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#include "pch.h"
#include "ImageCore.h"
#include "FileParser.h"

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

POINT g_mousePoint = { 0, 0 };  // 마우스 위치 저장용 전역 변수

struct PyramidLevel {
    cv::Mat image;
    double ratio;   // 원본 대비 비율
};

std::vector<PyramidLevel> g_pyramids;

struct BitmapInfo
{
    BITMAPINFOHEADER header;
    RGBQUAD palette[256];
};



void CreateImagePyramids(cv::Mat source)
{
	g_pyramids.clear();

    g_pyramids.push_back({ source, 1.0 });

    cv::Mat current = source;
	double currentRatio = 1.0;

    // 이미지 크기가 일정 수준 이하로 작아질 때까지 생성
    while (current.cols > 1000) {
        currentRatio *= 0.5;
		cv::Mat nextLevel;

		cv::resize(current, nextLevel, cv::Size(), 0.5, 0.5, cv::INTER_AREA);

		g_pyramids.push_back({ nextLevel, currentRatio });
        current = nextLevel;
    }
}

PyramidLevel* GetBestPyramid(double scale)
{
    int bestIdx = 0;

    for (int i = 0; i < g_pyramids.size(); i++)
    {
        if (g_pyramids[i].ratio >= scale)
        {
            bestIdx = i;
        }
        else
        {
            break;
        }
    }
	return &g_pyramids[bestIdx];
}

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

// 이미지를 화면 크기에 맞추어 로드하는 함수
void FitImageToWindow(HWND hWnd)
{
    if(g_displayImage.empty())
		return;

    RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	int winWidth = clientRect.right - clientRect.left;
    int winHeight = clientRect.bottom - clientRect.top;
    
	double scaleX = (double)winWidth / g_displayImage.cols;
	double scaleY = (double)winHeight / g_displayImage.rows;

	g_scaleFactor = (scaleX < scaleY) ? scaleX : scaleY;

	g_offsetX = (int)((winWidth - (g_displayImage.cols * g_scaleFactor)) / 2);
	g_offsetY = (int)((winHeight - (g_displayImage.rows * g_scaleFactor)) / 2);

	RequestViewerRedraw();
}

bool ProcessLoadedImage(cv::Mat loadedImg)
{
    if (loadedImg.empty())
        return false;

    g_currentImage = loadedImg.clone();

    g_scaleFactor = 1.0;
    CreateImagePyramids(g_currentImage);

    g_displayImage = g_currentImage.clone();

    if (g_hWndViewer)
    {
        RECT rect;
        GetClientRect(g_hWndViewer, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        // CalculateFitParams 호출 후 FitImageToWindow 호출 가정
        CalculateFitParams(width, height);
        FitImageToWindow(g_hWndViewer);
    }
    RequestViewerRedraw();
    return true;
}

cv::Mat LoadImageByPath(const std::string& utf8Path)
{
    int bitDepth = GetBitDepthFromFile(utf8Path);
    cv::Mat loadedImg;
    if (bitDepth == 8)
    {
        loadedImg = cv::imread(utf8Path, cv::IMREAD_GRAYSCALE);
    }
    else if (bitDepth == 24)
    {
        loadedImg = cv::imread(utf8Path, cv::IMREAD_COLOR);
    }
    else
    {
        loadedImg = cv::imread(utf8Path, cv::IMREAD_UNCHANGED);
    }
    return loadedImg;
}

// 리스트의 특정 인덱스에 있는 이미지를 로드하는 내부 헬퍼 함수
bool LoadImageByIndex(int index)
{
    if (index < 0 || index >= g_fileList.size())
        return false;

    g_currentIndex = index;
    const wchar_t* widePath = g_fileList[g_currentIndex].c_str();
    std::string utf8Path = ConvertWStringToStringUTF8(widePath);

	cv::Mat loadedImg = LoadImageByPath(utf8Path);
	
    return ProcessLoadedImage(loadedImg);
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
	FitImageToWindow(g_hWndViewer);
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

		g_scaleFactor = std::max(g_scaleFactor, 0.02); // 최소 스케일 제한
		g_scaleFactor = std::min(g_scaleFactor, 128.0); // 최대 스케일 제한


		//4 . 새로운 오프셋 계산 (마우스 위치 고정)
		g_offsetX = (int)std::round(mouseX - (imgX * g_scaleFactor));
		g_offsetY = (int)std::round(mouseY - (imgY * g_scaleFactor));

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
		g_mousePoint.x = LOWORD(lParam);
		g_mousePoint.y = HIWORD(lParam);

        if (g_isPanning && !g_displayImage.empty())
        {
            int currentX = LOWORD(lParam);
            int currentY = HIWORD(lParam);

            int deltaX = currentX - g_lastMousePos.x;
            int deltaY = currentY - g_lastMousePos.y;

            g_offsetX += deltaX;
            g_offsetY += deltaY;

            g_lastMousePos.x = currentX;
            g_lastMousePos.y = currentY;
        }
        
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

    if (!g_displayImage.empty())
    {
        // 1. 창 크기 및 기본 정보 획득
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int winW = clientRect.right - clientRect.left;
        int winH = clientRect.bottom - clientRect.top;

        // 2. 이중 버퍼링(Double Buffering) 설정
        HDC hMemDC = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, winW, winH);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

        // 배경을 윈도우 기본 색으로 채우기
        HBRUSH hBgBrush = GetSysColorBrush(COLOR_WINDOW);
        FillRect(hMemDC, &clientRect, hBgBrush);
        DeleteObject(hBgBrush);

        // 현재 배율에 최적화된 피라미드 선택 ⭐
        PyramidLevel* pLevel = GetBestPyramid(g_scaleFactor);

        if(!pLevel || pLevel->image.empty())
        {
            EndPaint(hWnd, &ps);
            return 0;
		}
        
		cv::Mat& targetMat = pLevel->image;
        
        double invScale = 1.0 / g_scaleFactor;
		double currentPyramidRatio = pLevel->ratio;
        
        double relativeScale = g_scaleFactor / currentPyramidRatio; // 선택된 이미지 기준 배율
		double invRelativeScale = 1.0 / relativeScale;

        // 3. ROI(관심 영역) 계산 ⭐
        // 화면 좌표를 이미지 좌표로 역산 (화면 밖 영역 포함)
        
        // g_offsetX, g_offsetY는 화면 상의 픽셀 오프셋
        int roiX = (int)std::floor(-g_offsetX * invRelativeScale);
        int roiY = (int)std::floor(-g_offsetY * invRelativeScale);

        // 화면 크기에 해당하는 피라미드 이미지의 픽셀 수
        int roiW = (int)std::ceil(winW * invRelativeScale);
        int roiH = (int)std::ceil(winH * invRelativeScale);

        cv::Rect imgRect(0, 0, targetMat.cols, targetMat.rows);
        cv::Rect viewRect(roiX, roiY, roiW, roiH);
        cv::Rect intersect = viewRect & imgRect;

        // 4. ROI 영역 렌더링
        if (intersect.width > 0 && intersect.height > 0)
        {
            // 실제 데이터 복사 없이 해당 영역의 포인터만 참조
            cv::Mat roiMat = targetMat(intersect);

            // destX, destY: 원본 이미지의 0,0 위치를 기준으로 오프셋 적용
            int destX = (int)(intersect.x * relativeScale + g_offsetX);
            int destY = (int)(intersect.y * relativeScale + g_offsetY);
            // destW, destH: ROI 영역을 relativeScale만큼 키운 크기
            int destW = (int)(intersect.width * relativeScale);
            int destH = (int)(intersect.height * relativeScale);

            int bytesPerPixel = roiMat.channels();
            // BITMAPINFO 설정 (ROI 이미지 크기에 맞춤)
            static struct {
                BITMAPINFOHEADER bmiHeader;
                RGBQUAD bmiColors[256];
            } bmi_storage;

            ZeroMemory(&bmi_storage, sizeof(bmi_storage));
            bmi_storage.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi_storage.bmiHeader.biWidth = (int)(roiMat.step / bytesPerPixel);
            bmi_storage.bmiHeader.biHeight = -roiMat.rows; // Top-Down
            bmi_storage.bmiHeader.biPlanes = 1;
            bmi_storage.bmiHeader.biBitCount = (WORD)(bytesPerPixel * 8);
            bmi_storage.bmiHeader.biCompression = BI_RGB;

            if (roiMat.channels() == 1) { // Grayscale 팔레트 설정
                for (int i = 0; i < 256; i++) {
                    bmi_storage.bmiColors[i].rgbRed = bmi_storage.bmiColors[i].rgbGreen = bmi_storage.bmiColors[i].rgbBlue = (BYTE)i;
                }
            }

            // 고품질 보간 대신 속도를 위해 COLORONCOLOR 사용 가능 (대용량 대응)
            SetStretchBltMode(hMemDC, COLORONCOLOR);
            SetBrushOrgEx(hMemDC, 0, 0, NULL);

            StretchDIBits(hMemDC,
                destX, destY, destW, destH,
                0, 0, roiMat.cols, roiMat.rows,
                roiMat.data, (BITMAPINFO*)&bmi_storage, DIB_RGB_COLORS, SRCCOPY);
        }

        if (g_scaleFactor < 10.0) {
            SetStretchBltMode(hMemDC, COLORONCOLOR);
        }
        else {
			SetStretchBltMode(hMemDC, HALFTONE);
        }

        // 5. 고배율 그리드 및 픽셀 값 표시 (g_scaleFactor >= 8.0)

		const double PIXEL_DISPLAY_THRESHOLD = 8.0;

        if (g_scaleFactor >= PIXEL_DISPLAY_THRESHOLD && !g_currentImage.empty()) 
        {
            SetBkMode(hMemDC, TRANSPARENT);

			double invScale = 1.0 / g_scaleFactor;
            // 화면 좌상단(0,0)에 해당하는 원본 이미지 픽셀의 시작점
            int startX = (int)std::max(0.0, std::floor((-g_offsetX * invScale)));
            int startY = (int)std::max(0.0, std::floor((-g_offsetY * invScale)));

            // 화면 우하단에 해당하는 원본 이미지 픽셀의 끝점 (+1 하여 포함)
            int endX = (int)std::min((double)g_currentImage.cols, std::ceil((winW - g_offsetX) * invScale));
            int endY = (int)std::min((double)g_currentImage.rows, std::ceil((winH - g_offsetY) * invScale));

            int imgW = g_currentImage.cols;
            int imgH = g_currentImage.rows;
            int matType = g_currentImage.type();
            
            int requiredMinSize = 40;
           
            // Grid 선을 그리기 위한 Pen 설정
            HPEN hGridPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
            HPEN hOldPen = (HPEN)SelectObject(hMemDC, hGridPen);
            
            const TCHAR* text = _T("255\n255\n255");

            for (int y = startY; y < endY; y++)
            {
                for (int x = startX; x < endX; x++)
                {
                    int screenX = g_offsetX + (int)(x * g_scaleFactor);
                    int screenY = g_offsetY + (int)(y * g_scaleFactor);
                    int scaledSize = (int)g_scaleFactor;

                    if (screenX + scaledSize > 0 && screenY + scaledSize > 0 &&
                        screenX < winW && screenY < winH && scaledSize >= requiredMinSize)
                    {
                        // Pixel Grid 선 그리기
                        HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
                        Rectangle(hMemDC, screenX, screenY, screenX + scaledSize, screenY + scaledSize);
                        SelectObject(hMemDC, hOldBrush);

                        if (scaledSize >= requiredMinSize)
                        {
                            if (matType == CV_8UC3)
                            {
                                cv::Vec3b pixel = g_currentImage.at<cv::Vec3b>(y, x);
                                int b = pixel[0];
                                int g = pixel[1];
                                int r = pixel[2];

                                // R, G, B 채널 데이터 정의
                                struct ChannelData { int value; COLORREF color; const TCHAR* label; };
                                ChannelData channelsData[] = {
                                    { r, RGB(255, 0, 0), _T("R") }, // R: 빨간색
                                    { g, RGB(0, 255, 0), _T("G") }, // G: 초록색
                                    { b, RGB(0, 0, 255), _T("B") }  // B: 파란색
                                };

                                // 픽셀 블록 내에서 각 값을 표시할 공간 계산 (3등분)
                                int lineHeight = scaledSize / 3;

                                // R, G, B 세 줄을 순회하며 출력
                                for (int i = 0; i < 3; ++i)
                                {
                                    TCHAR text[16];
                                    // "R:255"와 같은 포맷으로 출력
                                    _stprintf_s(text, 16, _T("%d"), channelsData[i].value);

                                    int currentY = screenY + i * lineHeight;

                                    // ⭐ B. 채널별 고유 색상 설정 ⭐
                                    SetTextColor(hMemDC, channelsData[i].color);

                                    RECT textRect = { screenX, currentY, screenX + scaledSize, currentY + lineHeight };
                                    DrawText(hMemDC, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                                }
                            }
                            else if (matType == CV_8UC1) // GRAYSCALE (1채널) 이미지
                            {
                                uchar gray = g_currentImage.at<uchar>(y, x);

                                TCHAR grayText[16];
                                _stprintf_s(grayText, 16, _T("%d"), gray); // "G:255" 포맷으로 출력

                                // 1채널은 대비 색상 유지 (회색이므로 흰색/검은색이 가장 잘 보임)
                                COLORREF contrastColor = (gray > 128) ? RGB(0, 0, 0) : RGB(255, 255, 255);
                                SetTextColor(hMemDC, contrastColor);

                                // 픽셀 블록 중앙에 출력
                                RECT textRect = { screenX, screenY, screenX + scaledSize, screenY + scaledSize };
                                DrawText(hMemDC, grayText, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                            }
                        }
                    }
                }
            }
            SelectObject(hMemDC, hOldPen);
            DeleteObject(hGridPen);
        }

        // 6. 하단 상태 표시줄 (실시간 좌표, RGB, 파일명, 배율) ⭐
        {
            int barHeight = 30;
            RECT barRect = { 0, winH - barHeight, winW, winH };
            HBRUSH hStatusBrush = CreateSolidBrush(RGB(20, 20, 20));
            FillRect(hMemDC, &barRect, hStatusBrush);
            DeleteObject(hStatusBrush);

            SetBkMode(hMemDC, TRANSPARENT);
            SetTextColor(hMemDC, RGB(255, 255, 255));

            // 왼쪽 정보: 마우스 좌표 및 픽셀 값
            TCHAR leftText[256];

            int imgX = (int)std::floor((g_mousePoint.x - g_offsetX) * invScale);
            int imgY = (int)std::floor((g_mousePoint.y - g_offsetY) * invScale);
            if (imgX >= 0 && imgX < g_currentImage.cols && imgY >= 0 && imgY < g_currentImage.rows) {
				int matType = g_currentImage.type();

                if (matType == CV_8UC1)
                {
					uchar value = g_currentImage.at<uchar>(imgY, imgX);

                    _stprintf_s(leftText, _T("  X: %d, Y: %d | Value: %d"), imgX, imgY, value);
                }
                else if (matType == CV_8UC3)
                {
                    cv::Vec3b p = g_currentImage.at<cv::Vec3b>(imgY, imgX);
                    _stprintf_s(leftText, _T("  X: %d, Y: %d | R: %d G: %d B: %d"), imgX, imgY, p[2], p[1], p[0]);
                }
            }
            DrawText(hMemDC, leftText, -1, &barRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            // 오른쪽 정보: 배율 및 파일명
            std::wstring fName = !g_fileList.empty() ? g_fileList[g_currentIndex] : L"No File";
            size_t lastS = fName.find_last_of(L"\\/");
            if (lastS != std::wstring::npos) fName = fName.substr(lastS + 1);

            TCHAR rightText[512];
            _stprintf_s(rightText, _T("Scale: %.2f x | File: %s  "), g_scaleFactor, fName.c_str());
            DrawText(hMemDC, rightText, -1, &barRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        }

        // 7. 최종 결과 화면으로 전송 (BitBlt)
        BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top,
            ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
            hMemDC, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

        // 8. 리소스 정리
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

	cv::Mat loadedImg = LoadImageByPath(utf8Path);
	
    return ProcessLoadedImage(loadedImg);    
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

