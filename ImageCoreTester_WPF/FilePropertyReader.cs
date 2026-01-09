using System;
using System.Runtime.InteropServices;
using System.IO;
using System.Runtime.Versioning; // .NET Framework 4.8에서 수동 정의가 필요합니다.

[SupportedOSPlatform("windows")] // COM 인터페이스는 Windows 전용입니다.
internal static class FilePropertyReader
{
    // =========================================================================
    // 1. P/Invoke를 위한 상수 및 구조체 정의
    //    -> 이제 선언만 하고 초기화는 정적 생성자에서 수행합니다.
    // =========================================================================

    // IID (Interface IDs)
    private static readonly Guid IID_IShellItem2;
    private static readonly Guid IID_IPropertyStore;

    // PKEY_Image_TotalBitDepth (속성 키)
    private static readonly PropertyKey PKEY_Image_TotalBitDepth;

    // PROPVARIANT의 vt (Vartype) 값 중, 우리가 필요한 UInt32 (DWORD) 타입
    private const ushort VT_UI4 = 19;

    // Win32 구조체 정의
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    private struct PropertyKey
    {
        public Guid fmtid;
        public uint pid;
    }
    // ... (PropVariant 구조체는 생략, 변경 없음) ...
    [StructLayout(LayoutKind.Sequential)]
    private struct PropVariant
    {
        public ushort vt; // Vartype
        public ushort wReserved1;
        public ushort wReserved2;
        public ushort wReserved3;
        // vt가 VT_UI4일 때 사용하는 유니온 영역 (UInt32)
        public uint ulVal;
        public int dummy; // 나머지 패딩 및 유니온 영역
    }


    // ⭐⭐ 정적 생성자: 모든 readonly 필드를 여기서 초기화합니다. ⭐⭐
    static FilePropertyReader()
    {
        // IID 초기화
        IID_IShellItem2 = new Guid("7E9FB0D3-919F-4307-AB2E-9B28D3613220");
        IID_IPropertyStore = new Guid("886D8EEB-CFEE-4C44-90C8-0CBDE9C5C644");

        // PropertyKey 초기화
        PKEY_Image_TotalBitDepth = new PropertyKey
        {
            fmtid = new Guid("14B81DA1-013F-4d4b-97A0-F2F6EFE736EA"),
            pid = 37
        };
    }

    // =========================================================================
    // 2. P/Invoke 함수 선언 (변경 없음)
    // =========================================================================

    // COM 객체 생성 함수
    [DllImport("shell32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern int SHCreateItemFromParsingName(
        string pszPath,
        IntPtr pbc, // 바인딩 컨텍스트는 사용하지 않으므로 IntPtr.Zero
        ref Guid riid,
        [MarshalAs(UnmanagedType.Interface)] out IShellItem2 ppv);

    // 메모리 해제 함수
    [DllImport("ole32.dll")]
    private static extern int PropVariantClear(ref PropVariant pvar);

    // IShellItem2 인터페이스 (COM 인터페이스 정의)
    [ComImport]
    [Guid("7E9FB0D3-919F-4307-4307-9B28D3613220")] // 이 부분도 IID_IShellItem2와 일치해야 함
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IShellItem2
    {
        // ... (메서드 선언 유지) ...
        void Dummy01(); void Dummy02(); void Dummy03(); void Dummy04(); void Dummy05(); void Dummy06(); void Dummy07();

        [PreserveSig]
        int GetPropertyStore(
            int flags, // GETPROPERTYSTOREFLAGS
            ref Guid riid,
            [MarshalAs(UnmanagedType.Interface)] out IPropertyStore ppv);

        // ... 나머지 IShellItem2 메서드 생략
    }

    // IPropertyStore 인터페이스 (COM 인터페이스 정의)
    [ComImport]
    [Guid("886D8EEB-CFEE-4C44-90C8-0CBDE9C5C644")] // 이 부분도 IID_IPropertyStore와 일치해야 함
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IPropertyStore
    {
        // ... (메서드 선언 유지) ...

        [PreserveSig]
        int GetValue(ref PropertyKey key, ref PropVariant pv);

        // ...
    }


    // =========================================================================
    // 3. 메인 기능 함수 (변경 없음)
    // =========================================================================
    public static int GetImageBitDepth(string filePath)
    {
        if (!File.Exists(filePath)) return 0;
        int bitDepth = 0;

        IShellItem2 shellItem = null;
        IPropertyStore propertyStore = null;

        // ⭐ 1. readonly 필드의 값을 쓰기 가능한 지역 변수에 복사 ⭐
        Guid iidShellItem2 = IID_IShellItem2;
        Guid iidPropertyStore = IID_IPropertyStore;
        PropertyKey pkeyBitDepth = PKEY_Image_TotalBitDepth;

        // IID_IShellItem2를 사용하여 COM 객체 생성
        // ⭐ ref IID_IShellItem2 대신 ref iidShellItem2 사용
        if (SHCreateItemFromParsingName(filePath, IntPtr.Zero, ref iidShellItem2, out shellItem) == 0)
        {
            try
            {
                // Property Store를 얻음
                // ⭐ ref IID_IPropertyStore 대신 ref iidPropertyStore 사용
                if (shellItem.GetPropertyStore(0, ref iidPropertyStore, out propertyStore) == 0)
                {
                    PropVariant pv = new PropVariant();

                    // PKEY_Image_TotalBitDepth 값 읽기
                    // ⭐ ref PKEY_Image_TotalBitDepth 대신 ref pkeyBitDepth 사용
                    if (propertyStore.GetValue(ref pkeyBitDepth, ref pv) == 0)
                    {
                        if (pv.vt == VT_UI4)
                        {
                            bitDepth = (int)pv.ulVal;
                        }
                    }
                    PropVariantClear(ref pv);
                }
            }
            finally
            {
                // COM 객체 해제
                if (propertyStore != null) Marshal.ReleaseComObject(propertyStore);
                if (shellItem != null) Marshal.ReleaseComObject(shellItem);
            }
        }

        // 비트 심도를 찾지 못한 경우 기본값 24로 가정
        return bitDepth > 0 ? bitDepth : 24;
    }
}