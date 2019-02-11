using System;
using System.Collections.Generic;

public class Class2
{
    static public List<String> dataList = new List<string>();

    static public void HelloClass2()
    {
        Console.WriteLine("HelloClass2");
        Console.WriteLine("Data: " + String.Join(" ", dataList));
    }
}


