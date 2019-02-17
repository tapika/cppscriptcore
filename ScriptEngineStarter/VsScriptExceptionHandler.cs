using EnvDTE;
using EnvDTE80;
using Microsoft.VisualStudio.Shell;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

/// <summary>
/// Handles errors coming from C# script compilation.
/// </summary>
public class VsScriptExceptionHandler : ScriptExceptionHandler
{
    public override async void ReportScriptResult(String file, Exception ex)
    {
        //Debug.WriteLine("Started from: " + Assembly.GetExecutingAssembly().FullName);
        ErrorListProvider errList = ScriptHost.GetUserObject<ErrorListProvider>(ScriptHost.mainArg);

        AsyncPackage sepkg = (AsyncPackage)ScriptHost.mainArg;
        DTE2 dte = await sepkg.GetServiceAsync(typeof(DTE)) as DTE2;

        OutputWindowPanes panes = dte.ToolWindows.OutputWindow.OutputWindowPanes;
        OutputWindowPane pane;
        String cppScript = "Script Engine";
        try
        {
            pane = panes.Item(cppScript);
        }
        catch (ArgumentException)
        {
            pane = panes.Add(cppScript);
        }

        pane.Clear();

        String msg;
        if (ex != null)
        {
            msg = ex.Message.ToString();

            // Just in case if developer is intrested to fix this.
            Debug.WriteLine(msg);

            // Bring into front in case of errors, otherwise don't activate window
            dte.ToolWindows.OutputWindow.Parent.AutoHides = false;
            dte.ToolWindows.OutputWindow.Parent.Activate();
        }
        else
        {
            msg = file + "(1): info: compiled & executed successfully\r\n";
        }

        pane.OutputString(msg);
        pane.Activate();
    }
}

