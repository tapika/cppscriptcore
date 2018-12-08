using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

public class CodeBuilder
{
    StringBuilder sb = new StringBuilder();
    String currentIdent = "";

    /// <summary>
    /// Adds amount of indentation of each line.
    /// </summary>
    /// <param name="amount">Amount of indents to add (positive) or remove (negative)</param>
    public CodeBuilder Indent(int amount = 1)
    {
        int newIdent = (currentIdent.Length / 4) + amount;
        if (newIdent < 0)
            newIdent = 0;

        currentIdent = "".PadLeft(newIdent * 4, ' ');
        return this;
    }

    public CodeBuilder UnIndent(int amount = 1)
    {
        return Indent(-amount);
    }


    public CodeBuilder Append<T>(T value)
    {
        sb.Append(value);
        return this;
    }

    public CodeBuilder AppendLine(string value)
    {
        sb.Append(currentIdent);
        sb.AppendLine(value);
        return this;
    }

    public CodeBuilder AppendLine()
    {
        sb.AppendLine();
        return this;
    }

    public override string ToString()
    {
        return sb.ToString();
    }

}


