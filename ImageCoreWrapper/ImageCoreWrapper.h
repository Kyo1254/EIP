#pragma once

#include "ImageCore.h"
#include <vcclr.h>

using namespace System;

namespace ImageCoreWrapper {

	// C#에서 객체처럼 사용할 관리 클래스
	public ref class ImageCore sealed
	{
	private:
		System::IntPtr m_hViewer;		
	public:
		ImageCore();
		// 초기화/종료
		static void Initialize();
		static void Terminate();

		// 뷰어 핸들 get
		property System::IntPtr ViewerHandle
		{
			System::IntPtr get() { return m_hViewer; }
		}

		// 뷰어 창 생성: C# IntPtr (hParent)를 받아 HWND를 생성하고 IntPtr로 반환
		System::IntPtr CreateViewer(System::IntPtr hParent);

		// 뷰어 위치/크기 설정
		void SetViewerPos(int x, int y, int width, int height);

		// 뷰어 강제 갱신 요청
		void RequestRedraw();

		// 이미지 로딩
		bool LoadImgFile(String^ filePath);

		// 뷰어 컨트롤 함수
		void ViewerZoomIn(float zoomStep);
		void ViewerZoomOut(float zoomStep);
		void ViewerResetZoom();
	};
}
