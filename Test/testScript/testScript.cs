//xss_ref ..\..\bin\ClassLibrary1.dll
//css_ref ..\..\bin\ClassLibrary2.dll
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

class Program
{
    // Must be here when using EnvDTE
    [STAThread]
    public static void Main()
    {
        ScriptHost.ConnectDebugger();
        //Class2.dataList.Add("testScript.Main was here");
        Console.Clear();
        Class2.HelloClass2();
        Class1.HelloClass1();
        //Console.WriteLine(DateTime.Now.ToString());
        Console.WriteLine(__FILE__() + ": Hello !");
        //for (int i = 0; i < 10; i++)
        //    Console.WriteLine("Hello " + i.ToString());
    }

    static string __FILE__([System.Runtime.CompilerServices.CallerFilePath] string fileName = "")
    {
        return fileName;
    }
}
