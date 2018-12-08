using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

public class CodeBuilder
{
    StringBuilder sb = new StringBuilder();
    String currentIndent = "";
    Dictionary<String, Object> userData;

    /// <summary>
    /// User defined data accociated with container.
    /// </summary>
    public Dictionary<String, Object> UserData
    {
        get {
            if (userData == null)
                userData = new Dictionary<string, object>();

            return userData;
        }
    }

    public T GetUserData<T>(String s) where T: class
    {
        var ud = UserData;
        Object o;
        if (!ud.ContainsKey(s))
            ud.Add(s, default(T));

        o = ud[s];
        return o as T;
    }

    public CodeBuilder SetUserData(String k, Object v)
    {
        var ud = UserData;
        if (ud.ContainsKey(k))
            ud[k] = v;
        else
            ud.Add(k, v);

        return this;
    }

    public int IndentValue()
    {
        return currentIndent.Length / 4;
    }

    public String IndentString
    {
        get {
            return currentIndent;
        }
    }

    /// <summary>
    /// Adds amount of indentation of each line.
    /// </summary>
    /// <param name="amount">Amount of indents to add (positive) or remove (negative)</param>
    public CodeBuilder Indent(int amount = 1)
    {
        int newIdent = (currentIndent.Length / 4) + amount;
        if (newIdent < 0)
            newIdent = 0;

        currentIndent = "".PadLeft(newIdent * 4, ' ');
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
        sb.Append(currentIndent);
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


