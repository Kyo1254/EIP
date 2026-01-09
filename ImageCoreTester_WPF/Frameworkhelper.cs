using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace System.Runtime.Versioning
{
    [AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Module | AttributeTargets.Class | AttributeTargets.Method | AttributeTargets.Property | AttributeTargets.Struct | AttributeTargets.Interface, AllowMultiple = true, Inherited = false)]
    internal sealed class SupportedOSPlatformAttribute : Attribute
    {
        // 생성자는 플랫폼 이름 문자열을 받도록 정의되어야 합니다.
        public SupportedOSPlatformAttribute(string platformName)
        {
            // 실제 동작은 없습니다. 
        }
    }
}
