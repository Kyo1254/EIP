using System;
using System.IO;
using System.Windows;
using System.Windows.Forms;
using System.Windows.Forms.Integration;
using ImageCoreWrapper;

namespace ImageCoreTester_WPF
{
    /// <summary>
    /// MainWindow.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class MainWindow : Window
    {
        private ImageCoreWrapper.ImageCore _imageCore;

        public MainWindow()
        {
            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            string appDirectory = AppDomain.CurrentDomain.BaseDirectory;
            // C++ core Initialize
            ImageCore.Initialize();
            _imageCore = new ImageCore();
        }

        private void ViewerHost_Loaded(object sender, RoutedEventArgs e)
        {
            WindowsFormsHost host = (WindowsFormsHost)sender;

            // WinForms Panel을 호스트하여 부모 HWND 확보
            Panel winFormsPanel = new Panel();
            winFormsPanel.Width = 800;
            winFormsPanel.Height = 600;
            host.Child = winFormsPanel;

            IntPtr hParent = winFormsPanel.Handle;

            if (_imageCore != null)
            {
                IntPtr hViewer = _imageCore.CreateViewer(hParent);

                if (hViewer != IntPtr.Zero)
                {
                    _imageCore.SetViewerPos(0, 0, (int)winFormsPanel.Width, (int)winFormsPanel.Height);

                    string appDirectory = AppDomain.CurrentDomain.BaseDirectory;
                    string projectRoot = Path.GetFullPath(Path.Combine(appDirectory, @"..\..\..\..")); // 3단계 위로 가정
                    string sampleImagePath = Path.Combine(projectRoot, "Image", "osaka.jpg");

                    if (File.Exists(sampleImagePath))
                    {
                        bool success = _imageCore.LoadImgFile(sampleImagePath);
                        if (success)
                        {
                            _imageCore.RequestRedraw();
                        }   
                    }
                    else
                    {
                        System.Windows.MessageBox.Show("샘플 이미지 파일을 찾을 수 없습니다: " + sampleImagePath);
                    }
                }
            }

        }

        private void ZoomIn_Click(object sender, RoutedEventArgs e)
        {
            if (_imageCore != null)
            {
                _imageCore.ViewerZoomIn(0.1f);
                _imageCore.RequestRedraw();
            }
        }

        private void ZoomOut_Click(object sender, RoutedEventArgs e)
        {
            if (_imageCore != null)
            {
                _imageCore.ViewerZoomOut(0.1f);
                _imageCore.RequestRedraw();
            }
        }

        private void ResetZoom_Click(object sender, EventArgs e)
        {
            if (_imageCore != null)
            {
                _imageCore.ViewerResetZoom();
                _imageCore.RequestRedraw();
            }
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            ImageCore.Terminate();
        }
    }
}
