#include <windows.h>
#include <tchar.h>
#include <string>
#include <filesystem>

// DLL 함수 포인터 정의
typedef HWND(*CREATE_VIEWER_FUNC)(HWND);
typedef bool (*LOAD_IMAGE_FUNC)(const std::string&);

LRESULT CALLBACK TestParentWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // 모든 처리는 DLL의 서브클래싱 함수에서 수행되므로 기본 처리만 남깁니다.
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // 1. DLL 로드
    HMODULE hDLL = LoadLibrary(_T("ImageCore.dll"));
    if (!hDLL) return 1;

    auto pCreateViewer = (CREATE_VIEWER_FUNC)GetProcAddress(hDLL, "CreateViewer");
    auto pLoadImage = (LOAD_IMAGE_FUNC)GetProcAddress(hDLL, "LoadImageFile");

    // 2. 부모 창 클래스 등록 (최소한의 설정)
    const TCHAR* CLASS_NAME = _T("EIP");
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = DefWindowProc; // 모든 처리는 DLL 서브클래싱이 담당
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // 3. 창 생성 (초기 위치와 크기만 지정)
    HWND hParent = CreateWindowEx(0, CLASS_NAME, _T("EIP Viewer"),
        WS_POPUP | WS_VISIBLE, // 스타일은 DLL 내부에서 WS_THICKFRAME 등으로 자동 재설정됨
        100, 100, 1024, 768, NULL, NULL, hInstance, NULL);

    // 4. DLL에 부모 핸들 전달 -> 여기서부터 DLL이 모든 UI를 제어함 ?
    if (pCreateViewer) {
        pCreateViewer(hParent);
    }

    // 5. 이미지 로드 테스트
    if (pLoadImage) {
        std::string sampleImagePath = std::filesystem::current_path().parent_path().string()
            + "\\Image\\osaka.jpg";
        pLoadImage(sampleImagePath);
    }

    // 메시지 루프
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    FreeLibrary(hDLL);
    return 0;
}