using System;
using System.Collections.Generic;

public class Class1
{
    static public List<String> dataList = new List<string>();

    static public void HelloClass1()
    {
        Console.WriteLine("HelloClass1");
        Console.WriteLine("Data: " + String.Join(" ", dataList));
    }
}


