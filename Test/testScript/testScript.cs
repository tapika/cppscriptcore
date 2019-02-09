using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

class Program
{
    // Must be here
    [STAThread]
    static void Main(string[] args)
    {
        ScriptHost.ConnectDebugger();
    }

    public static void Reload(string[] args)
    {
        Console.WriteLine("Reload v7");
    }

}
