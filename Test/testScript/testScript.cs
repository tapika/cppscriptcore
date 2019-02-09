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
        Console.Clear();
        Console.WriteLine(DateTime.Now.ToString());
        Console.WriteLine("Hello world 1");

        for (int i = 0; i < 10; i++)
            Console.WriteLine("Hello " + i.ToString());


    }


}
