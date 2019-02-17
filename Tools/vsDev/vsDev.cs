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
    public static async void Main( object arg )
    {
        ScriptEnginePackage sepkg = (ScriptEnginePackage)arg;
        DTE2 dte = await sepkg.GetServiceAsync(typeof(DTE)) as DTE2;
        IServiceProvider serviceProvider = sepkg as IServiceProvider;

        //sepkg.vsModule = new vsDev();
        //Debug.WriteLine("Started ok.");
        //VsShellUtilities.ShowMessageBox(this, "test 1", "title 1", OLEMSGICON.OLEMSGICON_INFO, OLEMSGBUTTON.OLEMSGBUTTON_OK, OLEMSGDEFBUTTON.OLEMSGDEFBUTTON_FIRST);

        //OutputWindowPanes panes = dte.ToolWindows.OutputWindow.OutputWindowPanes;
        //OutputWindowPane pane;
        //String cppScript = "C++ Script";
        //try
        //{
        //    pane = panes.Item(cppScript);
        //}
        //catch (ArgumentException)
        //{
        //    pane = panes.Add(cppScript);
        //}

        //pane.OutputString("Hello world !\n");
        //pane.Activate();
        //dte.ToolWindows.OutputWindow.Parent.Activate();

        //ErrorListProvider errorListProvider;

        //if (ScriptHost.userObj.Count == 0)
        //{
        //    errorListProvider = new ErrorListProvider(sepkg);
        //    ScriptHost.userObj.Add(errorListProvider);
        //}
        //else
        //{
        //    errorListProvider = ScriptHost.userObj[0] as ErrorListProvider;
        //}

        //errorListProvider.Tasks.Clear();
        //var task = new ErrorTask
        //{
        //    Document = @"C:\Prototyping\cppscriptcore\Tools\vsDev\vsDev.cs",
        //    Line = 59,
        //    Column = 22,
        //    ErrorCategory = TaskErrorCategory.Error,
        //    Category = TaskCategory.BuildCompile,
        //    Text = "Hello error panel 3"
        //};
        //task.Navigate += Task_Navigate;
        //errorListProvider.Tasks.Add(task);
        //errorListProvider.Show();
        //errorListProvider.BringToFront();

        Debug.WriteLine("New compilation: " + Assembly.GetExecutingAssembly().FullName);
        ScriptHost.exceptionHandler = new VsScriptExceptionHandler();
    }

    private static void Task_Navigate(object sender, EventArgs e)
    {
        Debug.WriteLine(Assembly.GetExecutingAssembly().FullName);
    }
}


class VsScriptExceptionHandler: ScriptExceptionHandler
{
    public override async void ReportScriptResult(String file, Exception ex)
    {
        Debug.WriteLine("Started from: " + Assembly.GetExecutingAssembly().FullName);
        ErrorListProvider errList = ScriptHost.GetUserObject<ErrorListProvider>(ScriptHost.mainArg);

        ScriptEnginePackage sepkg = (ScriptEnginePackage)ScriptHost.mainArg;
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
        }
        else {
            msg = file + "(1): info: compiled / executed successfully\r\n";
        }

        pane.OutputString(msg);
        pane.Activate();
    }
}

