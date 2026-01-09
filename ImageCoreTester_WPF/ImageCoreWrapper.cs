using System;
using System.Runtime.InteropServices;

namespace ImageCoreTester_WPF
{
    internal static class ImageCoreWrapper
    {
        private const string DllName = "ImageCore.dll";

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool CreateImageViewer(IntPtr hWnd, int width, int height);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool LoadImageFileW(
            [MarshalAs(UnmanagedType.LPWStr)] string filePath, // wchar_t* -> string
            int bitDepth); // 비트 심도 인자

        // 3. 뷰어 화면 갱신 요청 함수
        // C++: IMAGECORE_API void RequestViewerRedraw();
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void RequestViewerRedraw();

        // 4. (옵션) 뷰어 종료/해제 함수
        // C++: IMAGECORE_API void UninitializeViewer(); 
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void UninitializeViewer();

        // 5. (옵션) 뷰어 위치 및 크기 설정 함수 (WPF에서 HwndHost를 사용하면 필요 없을 수 있음)
        // C++: IMAGECORE_API void SetViewerPosition(int x, int y, int width, int height);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void SetViewerPosition(int x, int y, int width, int height);        
    }
}
