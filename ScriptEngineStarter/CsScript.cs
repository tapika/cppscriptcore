//#define NODEBUGTRACE
using System;
using System.Linq;
using System.Text;
using Microsoft.CSharp;
using System.CodeDom.Compiler;
using System.IO;
using System.Reflection;
using System.Diagnostics;
using System.Text.RegularExpressions;
using System.Collections.Generic;
using System.Threading;
using System.Runtime.InteropServices;

/// <summary>
/// class for executing c# script.
/// </summary>
public class CsScript
{
    static CSharpCodeProvider provider;
    static ICodeCompiler compiler;
    static int loadCounter = 1;

    /// <summary>
    /// 
    /// Compiles .cs script into dll/pdb, loads as assembly, and executes Main function.
    /// 
    /// Assembly gets executed within same appDomain.
    /// 
    /// If there is any compilation error - it will be thrown as exception.
    /// 
    /// Any //css_ref referred file will be loaded into appDomain once, and will stay there until application shuts down.
    /// 
    /// Unfortunately this function will collect .dll & .pdb compilations into %TEMP%\CSScriptHost.
    /// Use CleanupScScriptTempDir() on application startup to wipe out compilation folder.
    /// 
    /// </summary>
    static public void RunScript( String scriptPath, Object mainArg)
    {
        String tempDir = GetScriptTempDir();

        if (!Directory.Exists(tempDir))
            Directory.CreateDirectory(tempDir);

        String path = Path.GetFullPath(scriptPath);
        if( !File.Exists( path ) )
            throw new Exception("Error: Could not load file '" + Path.GetFileName(path) + "': File does not exists.");

        String dllBaseName = Path.GetFileNameWithoutExtension(path) + "_" + loadCounter.ToString();
        String basePath =  Path.Combine(tempDir, dllBaseName);

        String pdbPath = basePath + ".pdb";
        String dllPath = basePath + ".dll";

        try
        {
            List<String> filesToCompile = new List<string>();
            filesToCompile.Add(path);

            //---------------------------------------------------------------------------------------------------
            //  Get referenced .cs script file list, and from referenced files further other referenced files.
            //---------------------------------------------------------------------------------------------------
            CsScriptInfo csInfo = getCsFileInfo(filesToCompile[0], true);
            filesToCompile.AddRange(csInfo.csFiles);

            if (provider == null)
                provider = new CSharpCodeProvider();
#pragma warning disable 618
            if (compiler == null)
                compiler = provider.CreateCompiler();
#pragma warning restore 618
            CompilerParameters compilerparams = new CompilerParameters();
            compilerparams.GenerateExecutable = false;

#if NODEBUGTRACE
            // Currently it's not possible to generate in ram pdb debugging information.
            // Compiler option /debug:full should in theory allow that, but it does not work.
            compilerparams.GenerateInMemory = true;
#else
            compilerparams.GenerateInMemory = false;
            compilerparams.IncludeDebugInformation = true;          // Needed to get line / column numbers
            compilerparams.OutputAssembly = dllPath;
            compilerparams.CompilerOptions = "/d:DEBUG /d:TRACE";   // /debug+ /debug:full /optimize-
#endif

            // Add assemblies from my domain - all which are not dynamic.
            List<Assembly> appDomainAsms = AppDomain.CurrentDomain.GetAssemblies().Where(a => !a.IsDynamic).ToList();
            List<String> asms = new List<String>();

            foreach (String asmName in "System,System.Core,System.Xml".Split(','))
            {
                var appAsm = appDomainAsms.Where(x => x.GetName().Name.ToLower() == asmName.ToLower()).FirstOrDefault();
                if (appAsm == null)
                    continue;

                compilerparams.ReferencedAssemblies.Add(appAsm.Location);
            }

            foreach (var f in csInfo.refFiles)
                compilerparams.ReferencedAssemblies.Add(f);

            // ----------------------------------------------------------------
            //  If compile errors - report and exit.
            // ----------------------------------------------------------------
            CompilerResults results = compiler.CompileAssemblyFromFileBatch(compilerparams, filesToCompile.ToArray());
            if (results.Errors.HasErrors)
            {
                // Mimic visual studio error handling.
                StringBuilder sb = new StringBuilder();
                foreach (CompilerError error in results.Errors)
                {
                    // Missing reference file will not give any file or line information, we just use compilation
                    // script filename, and first line position. (Not exactly right, but something at least).

                    if (error.FileName == "")
                        sb.Append(Path.GetFileName(filesToCompile[0]));
                    else
                        sb.Append(Path.GetFileName(error.FileName));

                    if (error.Line == 0)
                        // error CS0006: Metadata file 'MystiqueDll.dll' could not be found
                        sb.Append("(1,1)");
                    else
                        sb.Append("(" + error.Line + "," + error.Column + ")");

                    sb.AppendFormat(": error {0}: {1}\r\n", error.ErrorNumber, error.ErrorText);
                }

                throw new Exception(sb.ToString());
            }
            loadCounter++;

            // ----------------------------------------------------------------
            //  Preload compiled .dll and it's debug information into ram.
            // ----------------------------------------------------------------
            MethodInfo entry = null;
            String funcName = "";
            Assembly asm = Assembly.LoadFrom(dllPath);

            // ----------------------------------------------------------------
            //  Locate entry point
            // ----------------------------------------------------------------
            BindingFlags flags = BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.IgnoreCase;

            foreach (Type type in asm.GetTypes())
            {
                funcName = "Main";
                entry = type.GetMethod(funcName, flags);

                if (entry != null)
                    break;
            }

            if (entry == null)
                throw new Exception(String.Format("{0}(1,1): error: Code does not have 'Main' function\r\n", Path.GetFileName(path)));

            if (entry.GetParameters().Length != 1)
                throw new Exception(String.Format("{0}(1,1): error: Function '{1}' is not expected to have one input parameter\r\n", Path.GetFileName(path), funcName));

            String oldDir = Environment.CurrentDirectory;
            //
            // We set current directory to where script is, just so script can use Directory.GetFiles without specifying directory.
            //
            Directory.SetCurrentDirectory(Path.GetDirectoryName(scriptPath));

            // ----------------------------------------------------------------
            //  Run script
            // ----------------------------------------------------------------
            try
            {
                entry.Invoke(null, new object[] { mainArg });
                Directory.SetCurrentDirectory(oldDir);
            }
            catch (Exception ex)
            {
                Directory.SetCurrentDirectory(oldDir);

                String errors = "";

                try
                {
                    StackFrame[] stack = new StackTrace(ex.InnerException, true).GetFrames();
                    StackFrame lastCall = stack[0];

                    errors = String.Format("{0}({1},{2}): error: {3}\r\n", path,
                        lastCall.GetFileLineNumber(), lastCall.GetFileColumnNumber(), ex.InnerException.Message);

                }
                catch (Exception ex3)
                {
                    errors = String.Format("{0}(1,1): error: Internal error - exception '{3}'\r\n", path, ex3.Message);
                }
                throw new Exception(errors);
            }
        }
        finally
        {
            // Currently all file deletes are disabled (otherwise throws exceptions)

            // Will work only if main was not possible to find.
            //try { File.Delete(dllPath); } catch { }

            // Works only when there is no debugger attached.
            //try { File.Delete(pdbPath); } catch { }
        }
    }

    static String GetGlobalScriptTempDir()
    {
        return Path.Combine(Environment.GetEnvironmentVariable("TEMP"), "CSScriptHost");
    }

    static String GetScriptTempDir()
    {
        String exeName = Path.GetFileNameWithoutExtension(Process.GetCurrentProcess().MainModule.FileName);
        // Use process id, so would not conflict.
        return Path.Combine(GetGlobalScriptTempDir(), exeName + "_" + Process.GetCurrentProcess().Id.ToString());
    }


    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr OpenProcess(ProcessAccessFlags access, bool inheritHandle, int procId);
    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    static extern bool CloseHandle(IntPtr hObject);

    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    static extern bool GetExitCodeProcess(IntPtr hProcess, out uint lpExitCode);

    [Flags]
    public enum ProcessAccessFlags : uint
    {
        All = 0x001F0FFF,
        Terminate = 0x00000001,
        CreateThread = 0x00000002,
        VirtualMemoryOperation = 0x00000008,
        VirtualMemoryRead = 0x00000010,
        VirtualMemoryWrite = 0x00000020,
        DuplicateHandle = 0x00000040,
        CreateProcess = 0x000000080,
        SetQuota = 0x00000100,
        SetInformation = 0x00000200,
        QueryInformation = 0x00000400,
        QueryLimitedInformation = 0x00001000,
        Synchronize = 0x00100000
    }


    /// <summary>
    /// Checks if given process is still alive
    /// </summary>
    /// <param name="processId">process id</param>
    /// <returns>true if process is alive, false if not</returns>
    static public bool IsProcessAlive(int processId)
    {
        IntPtr h = OpenProcess(ProcessAccessFlags.QueryInformation, true, processId);

        if (h == IntPtr.Zero)
            return false;

        uint code = 0;
        bool b = GetExitCodeProcess(h, out code);
        CloseHandle(h);

        if (b)
            b = (code == 259) /* STILL_ACTIVE  */;

        return b;
    }


    /// <summary>
    /// Cleans up temporary folder from compiled files.
    /// </summary>
    static public void CleanupScScriptTempDir()
    {
        try
        {
            String scTempDir = GetGlobalScriptTempDir();
            if (!Directory.Exists(scTempDir))
                return;

            String[] dirs = Directory.GetDirectories(scTempDir);
            Regex r = new Regex("_(\\d+)$");
            foreach (String dir in dirs)
            {
                var rr = r.Match(dir);
                if (!rr.Success)
                    continue;

                if(IsProcessAlive(int.Parse(rr.Groups[1].ToString())))
                    continue;

                Directory.Delete(dir, true);
            }
        }
        catch
        {
        }
    }


    /// <summary>
    /// Scans through C# script and gets additional information about C# script itself, 
    /// like dependent .cs files, and so on.
    /// </summary>
    /// <param name="csPath">C# script to load and scan</param>
    /// <param name="bUseAbsolutePaths">true if to use absolute paths, false if not</param>
    /// <param name="exceptFiles">Don't include path'es specified in here</param>
    /// <returns>C# script info</returns>
    static public CsScriptInfo getCsFileInfo( String csPath, bool bUseAbsolutePaths, List<String> exceptFiles = null )
    {
        CsScriptInfo csInfo = new CsScriptInfo();
        if (exceptFiles == null)
            exceptFiles = new List<string>();

        if(!exceptFiles.Contains(csPath) )
            exceptFiles.Add(csPath);

        // ----------------------------------------------------------------
        //  Using C# kind of syntax - like this:
        //      //css_include <file.cs>;
        // ----------------------------------------------------------------
        var regexOpt = RegexOptions.Multiline | RegexOptions.IgnoreCase;
        Regex reIsCommentUsingEmptyLine = new Regex("^ *(//|using|$)", regexOpt);
        Regex reCssImport = new Regex("^ *//css_include +(.*?);?$", regexOpt);
        Regex reDebug = new Regex("^ *//css_debug", regexOpt);
        Regex reCssRef = new Regex("^ *//css_ref +(.*?);?$", regexOpt);

        int iLine = 1;

        csInfo.bCsDebug = false;

        using (StreamReader reader = new StreamReader(csPath))
        {
            for (; ; iLine++)
            {
                String line = reader.ReadLine();
                if (line == null)
                    break;

                // If we have any comments, or using namespace or empty line, we continue scanning, otherwise aborting (class, etc...)
                if (!reIsCommentUsingEmptyLine.Match(line).Success)
                    break;

                // Pick up .dll filename from //css_ref <dll filename> file line.
                var rasm = reCssRef.Match(line);
                if (rasm.Success)
                {
                    // Not interested in ourselves, we are added automatically as reference .dll
                    if (line.EndsWith("syncproj.exe", StringComparison.CurrentCultureIgnoreCase))
                        continue;

                    // Allow end user to use %SystemRoot%\....dll environment variables.
                    String file = Environment.ExpandEnvironmentVariables(rasm.Groups[1].Value);

                    // If assembly file exists near script, use it.
                    if (bUseAbsolutePaths)
                    { 
                        String fileFullPath = Path.Combine(Path.GetDirectoryName(csPath), file);
                        if (File.Exists(fileFullPath))
                            file = fileFullPath;
                    } //if

                    csInfo.refFiles.Add(file);
                    continue;
                }

                var rem = reCssImport.Match(line);
                if (rem.Success)
                {
                    String file = rem.Groups[1].Value;
                    String fileFullPath = file;

                    if (!Path.IsPathRooted(file))
                        fileFullPath = Path.Combine(Path.GetDirectoryName(csPath), file);

                    if (!File.Exists(fileFullPath))
                        throw new FileSpecificException("Include file specified in '" + Path.GetFileName(fileFullPath) + 
                            "' was not found (Included from '" + Path.GetFileName(csPath) + "')", csPath, iLine);

                    bool bContains = false;
                    String fPath;

                    if (bUseAbsolutePaths)
                        fPath = fileFullPath;
                    else
                        fPath = file;

                    // Prevent cyclic references.
                    bContains = csInfo.csFiles.Contains(fPath);
                    if (!bContains) bContains = exceptFiles.Contains(fileFullPath);

                    if (!bContains)
                    {
                        csInfo.csFiles.Add(fPath.Replace("/", "\\"));
                        exceptFiles.Add(fileFullPath);
                    }

                    if (!bContains)
                    {
                        CsScriptInfo subCsInfo = getCsFileInfo(fileFullPath, bUseAbsolutePaths, exceptFiles);
                        if (subCsInfo.bCsDebug)             // Flag turned on from any dependent .cs file will enable debug also for main file.
                            csInfo.bCsDebug = true;

                        foreach (String subFile in subCsInfo.csFiles)
                            if (!csInfo.csFiles.Contains(subFile))
                                csInfo.csFiles.Add(subFile);

                        foreach (String refFile in subCsInfo.refFiles)
                            if (!csInfo.refFiles.Contains(refFile))
                                csInfo.refFiles.Add(refFile);
                    } //if

                } //if

                if (reDebug.Match(line).Success)
                    csInfo.bCsDebug = true;
            } //for
        } //using

        return csInfo;
    } //getCsFileInfo
} //class CsScript


/// <summary>
/// Exception which references specific file, specific line.
/// </summary>
public class FileSpecificException : Exception
{
    String file;
    int line;

    /// <summary>
    /// new exception which references specific file / line of source code position
    /// </summary>
    public FileSpecificException(String _msg, String _file, int _line) :
        base(_msg)
    {
        file = _file;
        line = _line;
    }
};


/// <summary>
/// Additional info about c# script.
/// </summary>
public class CsScriptInfo
{
    /// <summary>
    /// Referred .cs files to include into compilation
    /// </summary>
    public List<String> csFiles = new List<string>();

    /// <summary>
    /// Referred .dll's and assembly names, which must be included as reference assemblies when compiling.
    /// </summary>
    public List<String> refFiles = new List<string>();

    /// <summary>
    /// Just additional //css_debug for compile troubleshooting in this code
    /// </summary>
    public bool bCsDebug;

    /// <summary>
    /// checks if debug enabled.
    /// </summary>
    /// <returns>true - enabled</returns>
    public bool DebugEnabled()
    {
        if (bCsDebug)
            return true;

        if( g_bCsDebug )
            return true;

        return false;
    }

    /// <summary>
    /// Global flag to enable C# script compilation debugging
    /// </summary>
    public static bool g_bCsDebug = false;
}


