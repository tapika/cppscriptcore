using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Diagnostics;
using EnvDTE;                               //DTE
using EnvDTE80;
using Microsoft.VisualStudio.Project.VisualC.VCProjectEngine;
using Microsoft.VisualStudio.VCProjectEngine;
using System.Runtime.InteropServices;
using System.Text;
using System.Management;
using System.Runtime.InteropServices.ComTypes;
using System.Text.RegularExpressions;
using System.Threading;
using EnvDTE90;
using System.Collections.Generic;

class Program
{
    static String vsInstallPath = @"C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise";
    static String devEnvExe = Path.Combine(vsInstallPath, @"Common7\IDE\devenv.exe");
    static ProjectItemsEvents pievents;

    [STAThread]
    static void Main(string[] args)
    {
        String prjTemplDir = Path.Combine(vsInstallPath, @"Common7\IDE\ProjectTemplates");
        List<String> vsTemplates = Directory.GetFiles(prjTemplDir, "*.vstemplate", SearchOption.AllDirectories).Select(x => x.Substring(prjTemplDir.Length + 1)).ToList();
        Regex re = new Regex("^(.*?)" + Regex.Escape(@"\"));
        Func<String, String> untilBackSlash = (s) => { return re.Match(s).Groups[1].ToString(); };
        List<String> languages = vsTemplates.Select(x => untilBackSlash(x)).ToList();
        List<String> vsNames = new List<string>();

        for (int i = 0; i < vsTemplates.Count; i++)
        {
            bool keep = true;
            String lang = languages[i];
            if (lang != "CSharp" && lang != "VC" && lang != "Javascript")
                keep = false;

            if(
                vsTemplates[i].Contains("WebTemplate")  // has wizard
                ||
                vsTemplates[i].Contains("WapProj")      // hangs
                )
                keep = false;

            if (!keep)
            {
                vsTemplates.RemoveAt(i);
                languages.RemoveAt(i);
                i--;
                continue;
            }
            vsTemplates[i] = vsTemplates[i].Substring(languages[i].Length + 1);
            vsNames.Add(vsTemplates[i].Replace("\\", "_").Replace("1033_", "").Replace(".vstemplate", "").Replace(".", ""));
        }
        
        //var procs = System.Diagnostics.Process.GetProcesses().Where(x => x.ProcessName == "devenv").ToArray();
        //String devenvPath = @"C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\devenv.exe";
        //Assembly asm = Assembly.LoadFile(devenvPath);

        DTE2 dte = null;

        try
        {

            //String[] allPaths = {
            //    Path.Combine(vsInstallPath, @"Common7\IDE\PublicAssemblies"),
            //    // VCProjectShim is here.
            //    Path.Combine(vsInstallPath, @"Common7\IDE\CommonExtensions\Microsoft\VC\Project")
            //};

            //AppDomain.CurrentDomain.AssemblyResolve += (s, asmArgs) =>
            //{
            //    String dllName = new AssemblyName(asmArgs.Name).Name + ".dll";

            //    foreach (String dir in allPaths)
            //    {
            //        string path = Path.Combine(dir, dllName);

            //        if (!File.Exists(path))
            //            continue;

            //        return Assembly.LoadFrom(path);
            //    }

            //    Console.WriteLine("Warning: Required assembly not found: " + dllName);
            //    return null;
            //};

            //dte = (DTE)Activator.CreateInstance(Type.GetTypeFromProgID("VisualStudio.DTE.10.0"), true);
            //dte = (DTE)Activator.CreateInstance(Type.GetTypeFromProgID("VisualStudio.DTE.12.0"), true);
            //dte = (DTE)Activator.CreateInstance(Type.GetTypeFromProgID("VisualStudio.DTE.16.0"), true);

            bool bNormalStart = false;

            MessageFilter.Register();
            if (bNormalStart)
            {
                dte = (DTE2)Activator.CreateInstance(Type.GetTypeFromProgID("VisualStudio.DTE.15.0"), true);
            }
            else {
                //Microsoft.VisualStudio.Shell.ServiceProvider.GlobalProvider.GetService()
                bool bAttached = false;
                int processId = 0;
                Process2 processToAttachTo = null;

                for (int iTry = 0; iTry < 2; iTry++)
                {
                    dte = (DTE2)Marshal.GetActiveObject("VisualStudio.DTE.15.0");

                    var processes = dte.Debugger.LocalProcesses.Cast<EnvDTE.Process>().ToArray();
                    Debugger2 debugger2 = (Debugger2)dte.Debugger;

                    //
                    // https://varionet.wordpress.com/tag/debug/
                    // Windows 10: Attach2 triggers error: 8971001E, need to detach from all processes.
                    // Something to do debugging multiple processes?
                    //
                    debugger2.DetachAll();

                    int c = debugger2.Transports.Count;
                    Transport transport = debugger2.Transports.Item(1 /* Default transport */);
                    //foreach (var ix in transport.Engines)
                    //{
                    //    Engine e = ix as Engine;

                    //    String name = e.Name;
                    //    Console.WriteLine("'" + name + "'");
                    //}

                    // Otherwise will get timeout while trying to evaluate any com object in visual studio watch window
                    String debuggerType = "Managed (v3.5, v3.0, v2.0)";
                    //String debuggerType = "Managed (v4.6, v4.5, v4.0)";
                    //String debuggerType = "Managed";
                    Engine[] engines = new Engine[] { transport.Engines.Item(debuggerType) };

                    foreach (var process in processes)
                    {
                        String name = Path.GetFileNameWithoutExtension(Utils.call(() => (process.Name))).ToLower();
                        if (name != "devenv")
                            continue;

                        processId = Utils.call(() => (process.ProcessID));
                        String cmdArgs = GetProcessCommandLine(processId);

                        if (cmdArgs == null || !cmdArgs.Contains("-Embedding"))
                            continue;

                        processToAttachTo = process as Process2;
                        //Console.ReadKey();
                        Console.WriteLine("Attaching to: " + processId);
                        Utils.callVoidFunction(() => { processToAttachTo.Attach2(engines); });
                        // Apparently it takes some time for debugger to attach to process, otherwise missing breakpoints.
                        // Not sure if some sort of wait could be triggerred.
                        System.Threading.Thread.Sleep(2000);
                        bAttached = true;
                        break;
                    }

                    if (bAttached)
                        break;

                    {
                        if (iTry == 1)
                        {
                            Console.WriteLine("Error: Failed to launch vs2017.");
                            return;
                        }

                        // Analogue of
                        // Activator.CreateInstance(Type.GetTypeFromProgID("VisualStudio.DTE.15.0"), true);
                        // only with  experimental visual studio version.
                        Start_devenv_Embedded();
                    }
                }

                dte = (DTE2)GetDTE(processId, 120);
                //dte = null;
            }

            String edition = dte.Edition;
            Console.WriteLine("Edition: " + edition);

            // Make it visible.
            dte.MainWindow.Visible = true;
            dte.UserControl = true;

            //if (bAttached)
            //{
            //    dte = (DTE2)Marshal.GetActiveObject("VisualStudio.DTE.15.0");

            //    var processes = Utils.call(() => (dte.Debugger.LocalProcesses));
            //    foreach (var proc in processes)
            //    {
            //        EnvDTE.Process p = (EnvDTE.Process)proc;
            //        int pId = Utils.call(() => (p.ProcessID));
            //        if (pId != processId)
            //            continue;

            //        Console.WriteLine("Attaching to: " + processId);
            //        Utils.callVoidFunction(() => { p.Attach(); });
            //        bAttached = true;
            //        //Console.ReadKey();
            //        break;
            //    }
            //}

            //Microsoft.VisualStudio.OLE.Interop.IServiceProvider serv = (Microsoft.VisualStudio.OLE.Interop.IServiceProvider)dte;
            //dte.ExecuteCommand("File.InvokeOpenSyncProjectFile", "args");
            //dte.ExecuteCommand("File.InvokeOpenSyncProjectFile");
            //dte.ExecuteCommand("File.InvokeOpenSyncProjectFile", "thisIsArg1");
            //dte.ExecuteCommand("Tools.InvokeExecuteScript", @"D:\Prototyping\cppscriptcore\Tools\vsDev\bin\Debug\vsDev.dll");
            //int cmdCount = Utils.call(() => (dte.Commands.Count));


            //for( int i = 1; i <= cmdCount; i++)
            //{
            //    Command c = Utils.call(() => dte.Commands.Item(i));
            //    Console.WriteLine(Utils.call(() => c.Name));
            //    Console.WriteLine( "    " + Utils.call(() => c.ID));
            //    Console.WriteLine( "    " + Utils.call(() => c.Guid.ToString()) );
            //    Console.WriteLine();
            //}

            //Guid service = new Guid("89BE061A-1103-4D1D-8293-A51F8480E202");
            //Guid serviceApi = new Guid("89BE061A-1103-4D1D-8293-A51F8480E201");
            //IntPtr obj = IntPtr.Zero;
            //int r = serv.QueryService(ref service, ref serviceApi, out obj);

            //Console.WriteLine("[ Press any key to close ... ]");
            //Console.ReadKey();
            Solution2 sln2 = (Solution2)dte.Solution;

            for (int i = 0; i < vsTemplates.Count; i++)
            {
                String name = vsNames[i];
                String dir = Path.Combine(@"c:\Prototyping\testsln", name);
                bool bKeepProject = true;

                if (Directory.Exists(dir))
                    Directory.Delete(dir, true);

                Console.Write("Project '" + name + "... " );
                Directory.CreateDirectory(dir);
                sln2.Create(dir, name);
                Directory.CreateDirectory(dir + "\\prj");

                try
                {
                    string csTemplatePath = sln2.GetProjectTemplate(vsTemplates[i], languages[i]);
                    sln2.AddFromTemplate(csTemplatePath, dir + "\\prj", name, false);
                    Console.WriteLine("ok." );
                }
                catch (Exception ex)
                {
                    Console.WriteLine("Project '" + name + ": " + ex.Message);
                    bKeepProject = false;
                }
                sln2.Close(true);

                if (!bKeepProject)
                    Directory.Delete(dir, true);
            }

            /*
            Solution sln = dte.Solution;
            String slnPath = @"c:\Prototyping\testsln";
            if( !sln.IsOpen )
                sln.Create(slnPath, "test");
            Solution2 sln2 = (Solution2) sln;
            Solution3 sln3 = (Solution3) sln;

            int nCount = sln.Projects.Count;

            dte.Events.SolutionEvents.ProjectAdded += SolutionEvents_ProjectAdded;
            dte.Events.SolutionItemsEvents.ItemAdded += SolutionItemsEvents_ItemAdded;
            dynamic events = dte.Events.GetObject("CSharpProjectItemsEvents");
            //Events2 events2 = dte.Events as Events2;
            //events2.ProjectItemsEvents.ItemAdded += Pievents_ItemAdded;
            pievents = events as ProjectItemsEvents;
            pievents.ItemAdded += Pievents_ItemAdded;


            if (nCount <= 0)
            {
                string csTemplatePath = sln2.GetProjectTemplate(@"Windows\1033\ConsoleApplication\csConsoleApplication.vstemplate", "CSharp");
                sln.AddFromTemplate(csTemplatePath, slnPath + "\\prj", "Foo", false);
                dte.ExecuteCommand("File.SaveAll");
            }
            

            //sln.Open(@"D:\PrototypingQuick\ConsoleApplication2\ConsoleApplication2.sln");
            //Project p = sln.Projects.Item(1);
            //VCProject vcproj = (VCProject)p.Object;
            ////VCProject vcproj = (VCProject)p.Object;
            //Console.WriteLine(vcproj.ProjectFile);
            //Console.WriteLine(vcproj.ProjectGUID);
            Project p = sln.Projects.Item(1);
            //String kind = p.Kind;
            //String dir = sln2.ProjectItemsTemplatePath(kind);
            //String cppTemplDir = sln2.ProjectItemsTemplatePath("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}");
            
            ProjectItems pitems = p.ProjectItems;

            foreach (ProjectItem pi in pitems)
            {
                String name = pi.Name;

                if (pi.Name.ToLower() == "program.cs")
                    pi.Delete();
            }
            */

            MessageFilter.Revoke();
            Console.WriteLine();
            Console.ReadKey();
        }
        catch (Exception ex)
        {
            Console.WriteLine("Error: " + ex.Message);
            Console.WriteLine("Stack trace: " + ex.StackTrace);
            Console.ReadKey();
        }
        finally
        {
            // Need to close solution, so devenv.exe would not remain hanging in memory.
            //if (dte != null)
            //    dte.Solution.Close();
        }
    }

    private static void Pievents_ItemAdded(ProjectItem pi)
    {
        Console.WriteLine("File added: " + pi.Name);
    }

    private static void SolutionItemsEvents_ItemAdded(ProjectItem pi)
    {
        Console.WriteLine("File added: " + pi.Name);
    }

    private static void SolutionEvents_ProjectAdded(Project project)
    {
        Console.WriteLine("Project added: " + project.Name);
    }

    public static int Start_devenv_Embedded()
    {
        ProcessStartInfo procStartInfo = new ProcessStartInfo();
        procStartInfo.Arguments = "-Embedding" + " /RootSuffix Exp";
        //procStartInfo.Arguments = "-Embedding";
        procStartInfo.CreateNoWindow = true;
        procStartInfo.FileName = devEnvExe;
        procStartInfo.WindowStyle = ProcessWindowStyle.Hidden;
        procStartInfo.WorkingDirectory = Environment.CurrentDirectory;

        return System.Diagnostics.Process.Start(procStartInfo).Id;
    }

    /// <summary>
    /// Gets command line of specific process.
    /// </summary>
    /// <param name="processId">process id</param>
    /// <returns>null if not accessible (e.g. executed with admin priviledges)</returns>
    public static string GetProcessCommandLine(int processId)
    {
        using (ManagementObjectSearcher searcher = new ManagementObjectSearcher("SELECT CommandLine FROM Win32_Process WHERE ProcessId = " + processId))
        using (ManagementObjectCollection objects = searcher.Get())
            return objects.Cast<ManagementBaseObject>().SingleOrDefault()?["CommandLine"]?.ToString();
    }

    //
    // Idea borrowed from: https://www.helixoft.com/blog/creating-envdte-dte-for-vs-2017-from-outside-of-the-devenv-exe.html
    //


    /// <summary>
    /// Gets the DTE object from any devenv process.
    /// </summary>
    /// <remarks>
    /// After starting devenv.exe, the DTE object is not ready. We need to try repeatedly and fail after the
    /// timeout.
    /// </remarks>
    private static DTE GetDTE(int processId, int timeout)
    {
        DTE res = null;
        DateTime startTime = DateTime.Now;

        while (res == null && DateTime.Now.Subtract(startTime).Seconds < timeout)
        {
            System.Threading.Thread.Sleep(1000);
            res = GetDTE(processId);
        }

        return res;
    }


    [DllImport("ole32.dll")]
    private static extern int CreateBindCtx(uint reserved, out IBindCtx ppbc);


    /// <summary>
    /// Gets the DTE object from any devenv process.
    /// </summary>
    /// <param name="processId">
    /// <returns>
    /// Retrieved DTE object or <see langword="null"> if not found.
    /// </see></returns>
    private static DTE GetDTE(int processId)
    {
        object runningObject = null;

        IBindCtx bindCtx = null;
        IRunningObjectTable rot = null;
        IEnumMoniker enumMonikers = null;

        try
        {
            Marshal.ThrowExceptionForHR(CreateBindCtx(reserved: 0, ppbc: out bindCtx));
            bindCtx.GetRunningObjectTable(out rot);
            rot.EnumRunning(out enumMonikers);

            IMoniker[] moniker = new IMoniker[1];
            IntPtr numberFetched = IntPtr.Zero;
            while (enumMonikers.Next(1, moniker, numberFetched) == 0)
            {
                IMoniker runningObjectMoniker = moniker[0];

                string name = null;

                try
                {
                    if (runningObjectMoniker != null)
                    {
                        runningObjectMoniker.GetDisplayName(bindCtx, null, out name);
                    }
                }
                catch (UnauthorizedAccessException)
                {
                    // Do nothing, there is something in the ROT that we do not have access to.
                }

                Regex monikerRegex = new Regex(@"!VisualStudio.DTE\.\d+\.\d+\:" + processId, RegexOptions.IgnoreCase);
                if (!string.IsNullOrEmpty(name) && monikerRegex.IsMatch(name))
                {
                    Marshal.ThrowExceptionForHR(rot.GetObject(runningObjectMoniker, out runningObject));
                    break;
                }
            }
        }
        finally
        {
            if (enumMonikers != null)
            {
                Marshal.ReleaseComObject(enumMonikers);
            }

            if (rot != null)
            {
                Marshal.ReleaseComObject(rot);
            }

            if (bindCtx != null)
            {
                Marshal.ReleaseComObject(bindCtx);
            }
        }

        return runningObject as DTE;
    }

}


