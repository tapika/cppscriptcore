//using csscript;
//using CSScriptLibrary;
using EnvDTE80;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;


public enum CodeType
{
    Managed = 0,
    Mixed,
    Native
};



public class ScriptHost
{
    // Must be here when using EnvDTE
    [STAThread]
    static void Main(string[] args)
    {
        Class1.dataList.Add("ScriptHost::Main was here");
        CsScript.CleanupScScriptTempDir();
        ScriptServer_ConnectDebugger(CodeType.Managed, null);
    }

    /// <summary>
    /// Client side: Either starts hostExePath or attaches debugger to that process. 
    /// </summary>
    /// <param name="codetype">Code type to debug</param>
    /// <param name="csScript">C# script to monitor on</param>
    /// <param name="hostExePath">Executable, which hosts C# scripting</param>
    /// <param name="additionCommandLineArguments">Additional command line arguments for host executable</param>
    public static void ConnectDebugger(
        CodeType codetype = CodeType.Managed,
        [System.Runtime.CompilerServices.CallerFilePath] String csScript = null, String hostExePath = null, String additionCommandLineArguments = "")
    {
        string[] args = Environment.GetCommandLineArgs();
        if (args.Contains("-Embedding"))
            return;

        ScriptServer_ConnectDebugger(codetype, csScript, hostExePath, additionCommandLineArguments);
    }

    /// <summary>
    /// Client side - see ConnectDebugger description.
    /// Server side - starts ipc channel / monitors for newly attaching ipc connections.
    /// </summary>
    /// <param name="csScript">C# script</param>
    public static void ScriptServer_ConnectDebugger(CodeType codetype = CodeType.Managed, String csScript = null, String hostExePath = null, String additionCommandLineArguments = "")
    {
        //---------------------------------------------------------------
        // Detect Visual studio, which is debugging us.
        //---------------------------------------------------------------
        Process currentProcess = Process.GetCurrentProcess();
        Process debuggerProcess = null;
        debuggerProcess = GetParentProcess(currentProcess);

        //
        // Visual studio is either debug process by itself, or it starts msvsmon.exe, when we locate
        // parent of parent approach.
        //
        if (debuggerProcess != null && debuggerProcess.ProcessName.ToLower() == "msvsmon")
            debuggerProcess = GetParentProcess(debuggerProcess);

        if (debuggerProcess != null && debuggerProcess.ProcessName.ToLower() != "devenv" )
            debuggerProcess = null;     // Not a visual studio, e.g. cmd

        DTE2 dte = null;
        if (debuggerProcess != null)
        {
            MessageFilter.Register();
            dte = GetDTE(debuggerProcess.Id);
        }

        string[] args = Environment.GetCommandLineArgs();
        if (args.Contains("-Embedding"))
        {
            // Breakpoint here will not work, Debugger is not yet attached.
            for (int i = 0; i < 100; i++)
            {
                if (Debugger.IsAttached)
                    break;

                Thread.Sleep(50);
            }

            // Breakpoint will start to work after this point.
            Task.Run(() => { IpcServerLoop(); });

            if (csScript == null)
            {
                Console.WriteLine("Started with command line arguments: '" + String.Join(" ", args) + "'");
                Console.WriteLine("[ Press enter to close host ]");

                //Console.ReadLine();
                while (true)
                {
                    if (Console.KeyAvailable)
                        break;

                    // Needed for AppDomain.Unload
                    Application.DoEvents();
                    Thread.Sleep(50);
                }
            }

            if (dte != null) MessageFilter.Revoke();
            return;
        }

        //---------------------------------------------------------------
        // Self hosting if not embedded in application
        //---------------------------------------------------------------
        if (hostExePath == null)
            hostExePath = Assembly.GetExecutingAssembly().Location;

        if (dte != null)
        {
            MessageFilter.Register();

            if (dte.Debugger.DebuggedProcesses.Count > 1)
                // If starting debug with two processes, then wait a little for second to start so 3-rd process won't start
                Thread.Sleep(500);

            bool bAttached = false;
            String debuggerTypes = "Managed (v4.6, v4.5, v4.0)";

            switch (codetype)
            {
                case CodeType.Managed:
                    debuggerTypes = "Managed (v4.6, v4.5, v4.0)";
                    break;
                case CodeType.Mixed:
                    debuggerTypes = "Managed (v4.6, v4.5, v4.0)|Native";       // Application will terminate when detached from debugger.
                    break;
                case CodeType.Native:
                    debuggerTypes = "Native";
                    break;
            }


            Debugger2 debugger2 = (Debugger2)dte.Debugger;
            Transport transport = debugger2.Transports.Item(1 /* Default transport */);

            String[] debTypes = debuggerTypes.Split('|').ToArray();
            Engine[] engines = new Engine[debTypes.Length];
            for (int i = 0; i < engines.Length; i++)
                engines[i] = transport.Engines.Item(debTypes[i]);

            for (int iTry = 0; iTry < 2; iTry++)
            {
                var processes = dte.Debugger.LocalProcesses.Cast<Process2>().ToArray();
                var exeNames = processes.Select(x => x.Name).ToArray();

                for( int i = 0; i < exeNames.Length; i++)
                {
                    var process = processes[i];
                    if (exeNames[i].ToLower() == hostExePath.ToLower())
                    {
                        // No need to attach if debugging multiple processes
                        if (dte != null && dte.Debugger.DebuggedProcesses.Count <= 1)
                            process.Attach2(engines);

                        new IpcChannel(process.ProcessID).Send(csScript);
                        bAttached = true;
                        break;
                    }
                }

                if (bAttached)
                    break;

                // Don't launch second process
                if (iTry == 1)
                    break;

                ProcessStartInfo procStartInfo = new ProcessStartInfo();
                procStartInfo.Arguments = "-Embedding " + additionCommandLineArguments;
                procStartInfo.CreateNoWindow = true;
                procStartInfo.FileName = hostExePath;
                procStartInfo.WorkingDirectory = Environment.CurrentDirectory;

                Process.Start(procStartInfo);
                Console.WriteLine("Starting process '" + hostExePath + "'");
                //System.Threading.Thread.Sleep(1000);
            } //for

        }

        if (dte != null) MessageFilter.Revoke();
        Environment.Exit(0);
    }

    /// <summary>
    /// Queries for parent process, returns null if cannot be identified.
    /// </summary>
    /// <param name="process"></param>
    /// <returns>parent process or null if not found</returns>
    private static Process GetParentProcess(Process process)
    {
        Process parentProcess = null;
        var processName = process.ProcessName;
        var nbrOfProcessWithThisName = Process.GetProcessesByName(processName).Length;

        for (var index = 0; index < nbrOfProcessWithThisName; index++)
        {
            var processIndexdName = index == 0 ? processName : processName + "#" + index;
            var processId = new PerformanceCounter("Process", "ID Process", processIndexdName);
            if ((int)processId.NextValue() == process.Id)
            {
                var parentId = new PerformanceCounter("Process", "Creating Process ID", processIndexdName);
                try
                {
                    parentProcess = Process.GetProcessById((int)parentId.NextValue());
                }
                catch (ArgumentException)
                {
                    // Expected when starting with debugger to single process
                }
                break;
            }
        }

        return parentProcess;
    }


    /// <summary>
    /// C# scripts to monitor for changes.
    /// </summary>
    static List<String> scriptsToMonitor = new List<string>();
    static List<FileSystemWatcher> fswatchers = new List<FileSystemWatcher>();

    static void DebugPrint(String msg)
    {
        //Console.WriteLine(msg);
    }


    static void IpcServerLoop()
    {
        var ipc = new IpcChannel(Process.GetCurrentProcess().Id);
        Process currentProcess = Process.GetCurrentProcess();

        while (true)
        {
            String scriptToMonitor = "";

            for (int iRetry = 0; iRetry < 100; iRetry++)
            {
                bool b = ipc.Receive(ref scriptToMonitor, Timeout.Infinite);

                if (!b)
                    continue;

                break;
            }

            if (scriptToMonitor == "")
            {
                Console.WriteLine("Error: Too many failures, shutdowning...");
                break;
            }

            if (!File.Exists(scriptToMonitor))
            {
                Console.WriteLine("Error: File does not exists: " + scriptToMonitor);
                continue;
            }

            if (scriptsToMonitor.Contains(scriptToMonitor))
                continue;


            List<String> dirs = new List<string>();
            foreach (String script in scriptsToMonitor)
            {
                String dir = Path.GetDirectoryName(script);
                if (!dirs.Contains(dir))
                    dirs.Add(dir);
            }

            scriptsToMonitor.Add(scriptToMonitor);

            String newDir = Path.GetDirectoryName(scriptToMonitor);
            if (!dirs.Contains(newDir))
            {
                FileSystemWatcher watcher = new FileSystemWatcher();
                watcher.Path = newDir;
                watcher.NotifyFilter = NotifyFilters.LastAccess | NotifyFilters.LastWrite | NotifyFilters.FileName | NotifyFilters.DirectoryName;
                watcher.Filter = "*.cs";
                watcher.Changed += FileChanged;
                watcher.Created += FileChanged;
                watcher.Renamed += FileChanged;
                // Begin watching.
                watcher.EnableRaisingEvents = true;
                DebugPrint("- Monitoring for folder '" + newDir + "'");
                fswatchers.Add(watcher);
            }

            FileReload(scriptToMonitor);
        } //while

    }

    /// <summary>
    /// Triggered multiple times when file is changed. 
    /// </summary>
    private static void FileChanged(object sender, FileSystemEventArgs e)
    {
        FileReload(e.FullPath);
    }

    static void FileReload( String file )
    {
        DebugPrint("- File changed '" + file + "'");

        if (!scriptsToMonitor.Contains(file))
            return;

        //
        // Dynamically loadable .dll/assembly cannot reside next with application folder, as it gets loaded from there by default. 
        // Also need to change assembly name whenever new compilation comes up.
        //
        //String destFile = Path.Combine(dirReload, Path.GetFileName(file));
        //String srcPdb = Path.Combine(Path.GetDirectoryName(file), Path.GetFileNameWithoutExtension(file) + ".pdb");
        //String destPdb = Path.Combine(Path.GetDirectoryName(destFile), Path.GetFileNameWithoutExtension(destFile) + ".pdb");
        //File.Copy(file, destFile, true);
        //File.Copy(srcPdb, destPdb, true);

        //Process p = new Process();
        //p.StartInfo.UseShellExecute = false;
        //p.StartInfo.RedirectStandardOutput = true;
        //p.StartInfo.FileName = @"C:\Users\PikarTa1\Downloads\DebugInfo\Debug\DebugInfo.exe";
        //p.StartInfo.Arguments = "\"" + destFile + "\" clean-path";
        //p.Start();
        //String error = p.StandardOutput.ReadToEnd();
        //p.WaitForExit();

        //Assembly asm = Assembly.LoadFile(destFile);
        //asm.GetTypes().Select(x => x.GetMethod("Execute", BindingFlags.Static | BindingFlags.Public)).First().Invoke(null, null);

        //String exe = @"C:\Prototyping\cppscriptcore\bin\testScript.exe";

        // Not marked as serializable.
        // AppDomainSetup setup = new AppDomainSetup();
        // setup.ApplicationBase = AppDomain.CurrentDomain.BaseDirectory;
        // var appDomain = AppDomain.CreateDomain("myAppDomain", null, setup);

        // // Only loads assembly in one application domain.
        // appDomain.DoCallBack(() => {
        //     Assembly asm = Assembly.LoadFile(destFile);
        //     asm.GetTypes().Select(x => x.GetMethod("Execute", BindingFlags.Static | BindingFlags.Public)).First().Invoke(null, null);
        // }

        //);

        // AppDomain.Unload(appDomain);



        // Works
        //String errors = "";
        //if (!CsScript.RunScript(dirReload, loadCount, @"C:\Prototyping\cppscriptcore\Test\testScript\testScript.cs", false, false, out errors))
        //{
        //    Console.WriteLine("Error: " + errors);
        //    Console.WriteLine("");
        //}

        //String csScript = @"C:\Prototyping\cppscriptcore\Test\testScript\testScript.cs";

        //if (!File.Exists(scriptToMonitor))
        //    return;

        //Console.WriteLine("- reload");

        // Causes .pdb lock, assembly domain unload does not help.
        //using (AsmHelper helper = new AsmHelper(exe, "LoadDomain", true))

        Exception lastException = null;

        for( int iRetry = 0; iRetry < 10; iRetry++ )
        {
            try
            {
                // Works, but in isolated appDomain.
                //String[] asms = new CSharpParser(file, true).RefAssemblies;

                //using (AsmHelper helper = new AsmHelper(CSScript.CompileFile(file, null, true, asms), null, true))
                //{
                //    helper.Invoke("*.Main");
                //}

                CsScript.RunScript(file);
                break;
            }
            catch (IOException ex)
            {
                // File might be in a middle of save. Try to retry within 50 ms again.
                if (iRetry == 9)
                {
                    lastException = ex;
                    break;
                }
                Thread.Sleep(50);
                continue;
            }
            catch (Exception ex)
            {
                lastException = ex;
                break;
            }
        }

        if (lastException != null)
            Console.WriteLine(lastException.Message);

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
    private static DTE2 GetDTE(int processId)
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

        return runningObject as DTE2;
    }

}
