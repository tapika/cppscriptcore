//css_ref \Prototyping\cppscriptcore\ScriptEngine\packages\Microsoft.VisualStudio.Shell.15.0.15.0.26228\lib\Microsoft.VisualStudio.Shell.15.0.dll
//css_ref C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\PublicAssemblies\EnvDTE80.dll
//css_ref \Prototyping\cppscriptcore\ScriptEngine\bin\Debug\ScriptEngine.dll
//css_ref \Prototyping\cppscriptcore\ScriptEngine\packages\Microsoft.VisualStudio.Shell.Interop.7.10.6071\lib\Microsoft.VisualStudio.Shell.Interop.dll
//css_ref \Prototyping\cppscriptcore\ScriptEngine\packages\Microsoft.VisualStudio.OLE.Interop.7.10.6071\lib\Microsoft.VisualStudio.OLE.Interop.dll
//css_ref \Prototyping\cppscriptcore\ScriptEngine\packages\Microsoft.VisualStudio.Shell.Interop.8.0.8.0.50727\lib\Microsoft.VisualStudio.Shell.Interop.8.0.dll
//css_ref \Prototyping\cppscriptcore\ScriptEngine\packages\Microsoft.VisualStudio.Shell.Interop.9.0.9.0.30729\lib\Microsoft.VisualStudio.Shell.Interop.9.0.dll
//css_ref \Prototyping\cppscriptcore\ScriptEngine\packages\Microsoft.VisualStudio.Shell.Interop.10.0.10.0.30319\lib\Microsoft.VisualStudio.Shell.Interop.10.0.dll
//css_ref \Prototyping\cppscriptcore\ScriptEngine\packages\Microsoft.VisualStudio.Shell.Interop.14.0.DesignTime.14.3.25407\lib\Microsoft.VisualStudio.Shell.Interop.14.0.DesignTime.dll
//css_ref \Prototyping\cppscriptcore\ScriptEngine\packages\Microsoft.VisualStudio.Shell.Framework.15.0.26228\lib\net45\Microsoft.VisualStudio.Shell.Framework.dll
//css_ref C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\PublicAssemblies\EnvDTE.dll
//css_ref C:\Prototyping\cppscriptcore\bin\ScriptEngineStarter.exe
using EnvDTE;
using EnvDTE80;
using Microsoft.VisualStudio.Shell;
using ScriptEngine;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

public class vsDev
{
    public static bool Main( object arg )
    {
        Solution solution = ((ScriptEnginePackage)arg).dte.Solution;

        ScriptHost.console.Clear();
        ScriptHost.console.WriteLine("Build: " + Assembly.GetExecutingAssembly().Location);
        ScriptHost.console.WriteLine("---------------------------------------------------------------------------------");
        ScriptHost.console.WriteLine(solution.FileName);

        return false;
    }

}


