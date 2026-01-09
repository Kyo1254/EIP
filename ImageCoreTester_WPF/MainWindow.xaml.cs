using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Forms.Integration;

namespace ImageCoreTester_WPF
{
    /// <summary>
    /// MainWindow.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class MainWindow : Window
    {
        private IntPtr _viewerHwnd = IntPtr.Zero;
        private readonly System.Windows.Forms.Panel _hostPanel = new System.Windows.Forms.Panel();
        public MainWindow()
        {
            InitializeComponent();
            this.Loaded += MainWindow_Loaded;
        }

        private void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            // 1. WindowsFormsHost를 사용하여 WinForms Panel을 통합
            HostControl.Child = _hostPanel;

            // 2. Panel의 핸들을 가져옴 (이것이 DLL이 필요로 하는 HWND입니다)
            _viewerHwnd = _hostPanel.Handle;

            // 3. DLL 초기화 호출
            int width = (int)HostBorder.ActualWidth;
            int height = (int)HostBorder.ActualHeight;

            // C++ DLL의 InitializeViewer 함수를 호출하여 HWND 전달
            ImageCoreWrapper.CreateImageViewer(_viewerHwnd, width, height);
        }

        private void HostBorder_Drop(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);
                if (files.Length > 0)
                {
                    string filePath = files[0];

                    // ⭐ 파일 속성(Bit Depth) 확인 로직 (테스터 프로젝트에서 수행)
                    int bitDepth = FilePropertyReader.GetImageBitDepth(filePath);

                    // 4. 이미지 로드 호출
                    ImageCoreWrapper.LoadImageFileW(filePath, bitDepth);

                    // (LoadImageFileW 내부에서 RequestViewerRedraw를 호출하는 경우 필요 없음)
                    // ImageCoreWrapper.RequestViewerRedraw(); 
                }
            }
        }

        private void HostBorder_MouseDown(object sender, MouseButtonEventArgs e)
        {

        }
    }
}
