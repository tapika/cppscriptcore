using EnvDTE80;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

public class ScriptHost
{
    static void Main(string[] args)
    {
        ConnectDebugger();
    }

    /// <summary>
    /// Starts exe and attaches debugger to it.
    /// If process is hosted, waits for debugger
    /// </summary>
    /// <param name="exe">Executable to start</param>
    /// <param name="additionCommandLineArguments">Additional command line arguments to executable</param>
    public static void ConnectDebugger(String exe = null, String additionCommandLineArguments = "")
    {
        //---------------------------------------------------------------
        // Detect Visual studio, which is debugging us.
        //---------------------------------------------------------------
        Process currentProcess = Process.GetCurrentProcess();
        Process debuggerProcess = null;
        DTE2 dte = null;
        var processName = currentProcess.ProcessName;
        var nbrOfProcessWithThisName = Process.GetProcessesByName(processName).Length;

        for (var index = 0; index < nbrOfProcessWithThisName; index++)
        {
            var processIndexdName = index == 0 ? processName : processName + "#" + index;
            var processId = new PerformanceCounter("Process", "ID Process", processIndexdName);
            if ((int)processId.NextValue() == currentProcess.Id)
            {
                var parentId = new PerformanceCounter("Process", "Creating Process ID", processIndexdName);
                try
                {
                    debuggerProcess = Process.GetProcessById((int)parentId.NextValue());
                }
                catch (ArgumentException)
                {
                    // Expected when starting with debugger to single process
                }
                break;
            }
        }

        if (debuggerProcess != null && debuggerProcess.ProcessName.ToLower() != "devenv")
            debuggerProcess = null;

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

                System.Threading.Thread.Sleep(50);
            }

            // Breakpoint will start to work here
            FileSystemWatcher watcher = new FileSystemWatcher();
            watcher.Path = @"C:\Prototyping\cppscriptcore\bin";
            watcher.NotifyFilter = NotifyFilters.LastAccess | NotifyFilters.LastWrite | NotifyFilters.FileName | NotifyFilters.DirectoryName;
            watcher.Filter = "*.exe";
            watcher.Changed += FileChanged;
            watcher.Created += FileChanged;
            watcher.Renamed += FileChanged;
            // Begin watching.
            watcher.EnableRaisingEvents = true;

            if (dte != null)
            {
                List<int> processedBeingDebugged = dte.Debugger.DebuggedProcesses.Cast<EnvDTE.Process>().Select(x => x.ProcessID).ToList();
                processedBeingDebugged.Remove(currentProcess.Id);

                if (processedBeingDebugged.Count != 0)
                {
                    Process anotherProcess = Process.GetProcessById(processedBeingDebugged[0]);
                    anotherProcess.EnableRaisingEvents = true;
                    anotherProcess.Exited += (sender, e) => { Environment.Exit(0); };
                }
            }

            FileReload(Path.Combine(watcher.Path, "testScript.exe"));

            if (exe == null)
            {
                Console.WriteLine("Started with command line arguments: '" + String.Join(" ", args) + "'");
                Console.WriteLine("[ Press enter to close host ]");
                Console.ReadLine();
            }

            if ( dte != null ) MessageFilter.Revoke();
            return;
        }

        //---------------------------------------------------------------
        // Self hosting if not embedded in application
        //---------------------------------------------------------------
        if (exe == null)
            exe = System.Reflection.Assembly.GetExecutingAssembly().Location;

        if (dte != null)
        {
            MessageFilter.Register();
            bool bAttached = false;

            for (int iTry = 0; iTry < 2; iTry++)
            {
                var processes = dte.Debugger.LocalProcesses.Cast<EnvDTE.Process>().ToArray();
                var exeNames = processes.Select(x => x.Name).ToArray();

                for( int i = 0; i < exeNames.Length; i++)
                {
                    if (exeNames[i] == exe)
                    {
                        try
                        {
                            processes[i].Attach();
                        }
                        catch (Exception ex)
                        {
                            // When debugging multiple processes this is expected.
                            if (dte.Debugger.DebuggedProcesses.Count == 1)
                                throw ex;
                        }

                        bAttached = true;
                        break;
                    }
                }

                if (bAttached)
                    break;

                ProcessStartInfo procStartInfo = new ProcessStartInfo();
                procStartInfo.Arguments = "-Embedding " + additionCommandLineArguments;
                procStartInfo.CreateNoWindow = true;
                procStartInfo.FileName = exe;
                procStartInfo.WorkingDirectory = Environment.CurrentDirectory;

                Process.Start(procStartInfo);
                Console.WriteLine("Starting process '" + exe + "'");
                //System.Threading.Thread.Sleep(1000);
            } //for

        }

        if ( dte != null ) MessageFilter.Revoke();
    }

    static List<String> changedFiles = new List<string>();


    /// <summary>
    /// Triggered multiple times when file is changed. 
    /// </summary>
    private static void FileChanged(object sender, FileSystemEventArgs e)
    {
        lock (changedFiles)
        {
            if (changedFiles.Contains(e.FullPath))
            {
                return;
            }
            changedFiles.Add(e.FullPath);
        }

        System.Timers.Timer timer = new System.Timers.Timer(100) { AutoReset = false };
        timer.Elapsed += (timerElapsedSender, timerElapsedArgs) =>
        {
            lock (changedFiles)
            {
                changedFiles.Remove(e.FullPath);
                FileReload(e.FullPath);
            }
        };
        timer.Start();
    }

    static int loadCount = 0;

    static void FileReload( String file )
    {
        Console.WriteLine("File changed, reload: " + file);

        loadCount++;
        String exeName = Path.GetFileNameWithoutExtension(System.Diagnostics.Process.GetCurrentProcess().MainModule.FileName);
        String dirReload = Path.Combine(Environment.GetEnvironmentVariable("TEMP"), "scriptHost", exeName + "_" + Process.GetCurrentProcess().Id.ToString());

        if (!Directory.Exists(dirReload))
            Directory.CreateDirectory(dirReload);

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
        //asm.GetTypes().Select(x => x.GetMethod("Reload", BindingFlags.Static | BindingFlags.Public)).First().Invoke(null, null);
        String errors = "";
        if (!CsScript.RunScript(dirReload, loadCount, @"C:\Prototyping\cppscriptcore\Test\testScript\testScript.cs", false, false, out errors))
        {
            Console.WriteLine("Error: " + errors);
            Console.WriteLine("");
        }
    }

    /// <summary>
    /// Executes command and returns standard output & standard error to error string.
    /// </summary>
    /// <returns>Application exit code</returns>
    public static int ExecCmd(String cmd, ref String error)
    {
        Process p = new Process();
        p.StartInfo.UseShellExecute = false;
        p.StartInfo.RedirectStandardOutput = true;
        p.StartInfo.FileName = "cmd.exe";
        // Whole command should be quoted.
        // cmd.exe /C ""mytool.exe" "c:\mypath\myfile.txt""
        //            ^                                   ^                                   ^
        // https://social.msdn.microsoft.com/forums/vstudio/en-US/03ea84cf-19a6-450d-a3d6-8a139857e0cd/help-with-paths-containing-spaces
        //
        p.StartInfo.Arguments = "/C \"" + cmd + "\" 2>&1";
        // Console.WriteLine("Executing 'cmd.exe " + p.StartInfo.Arguments + "'");
        p.Start();
        error = p.StandardOutput.ReadToEnd();
        p.WaitForExit();

        return p.ExitCode;
    } //ExecCmd


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
