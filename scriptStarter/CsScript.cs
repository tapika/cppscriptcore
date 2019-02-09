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

/// <summary>
/// class for executing c# script.
/// </summary>
public class CsScript
{
    static CSharpCodeProvider provider;
    static ICodeCompiler compiler;
    static String[] refAssemblies;

    /// <summary>
    /// Compiles .cs script into dll/pdb, loads as assembly, and executes Main function.
    /// Temporary dll/pdb gets deleted. If .cs throws exception - it will be converted to
    /// error information, including .cs filename and source code line information.
    /// </summary>
    /// <param name="_path">Path to script which to execute</param>
    /// <param name="bAllowThrow">true if allow to throw exceptions</param>
    /// <param name="errors">Errors if any</param>
    /// <param name="args">Main argument parameters</param>
    /// <param name="bCompileOnly">true if only to compile</param>
    /// <returns>true if execution was successful.</returns>
    static public bool RunScript( String cacheDir, int loadCount, String _path, bool bCompileOnly, bool bAllowThrow, out String errors, params String[] args )
    {
        errors = "";

        // ----------------------------------------------------------------
        //  Load script
        // ----------------------------------------------------------------
        String path = Path.GetFullPath( _path );
        if( !File.Exists( path ) )
        {
            errors = "Error: Could not load file '" + Path.GetFileName(path) + "': File does not exists.";
            if (bAllowThrow)
                throw new Exception(errors);
            return false;
        }

        String dllBaseName = Path.GetFileNameWithoutExtension(_path) + "_" + loadCount.ToString();
        String tempDll;
        String dllInfoFile = "";
        DateTime dllInfoRealDate = DateTime.MinValue;

        for (int i = 1; ; i++)
        {
            tempDll = Path.Combine(cacheDir, dllBaseName);
            if (i != 1)
                tempDll += i;

            dllInfoFile = tempDll + "_script.txt";       // We keep here C# script full path just not to get collisions.
            if (!File.Exists(dllInfoFile))
            {
                File.WriteAllText(dllInfoFile, path);
                break;
            }

            String pathFromFile = "";
            //
            // Another instance of syncProj might be accessing same file at the same time, we try to retry automatically after some delay.
            //
            for ( int iTry = 0 ; iTry < 20; iTry++)
            {
                try
                {
                    pathFromFile = File.ReadAllText(dllInfoFile);
                    break;
                }
                catch (Exception)
                {
                    Thread.Sleep(10 + iTry * 5);   // 5*20+10, overall (5*20+10)*20 / 2 = 1.1 sec
                }
            }

            if (pathFromFile == path)
            {
                dllInfoRealDate = File.GetLastWriteTime(dllInfoFile);
                break;
            }
        }

        String pdb = tempDll + ".pdb";
        tempDll += ".dll";

        List<String> filesToCompile = new List<string>();
        filesToCompile.Add(path);

        //---------------------------------------------------------------------------------------------------
        //  Get referenced .cs script file list, and from referenced files further other referenced files.
        //---------------------------------------------------------------------------------------------------
        CsScriptInfo csInfo = getCsFileInfo(filesToCompile[0], true);
        filesToCompile.AddRange(csInfo.csFiles);

        bool bCompileDll = false;

        //---------------------------------------------------------------------------------------------------
        // Compile .dll only if script.cs and it's dependent .cs are newer than compiled .dll.
        //---------------------------------------------------------------------------------------------------
        if (!File.Exists(tempDll))
            bCompileDll = true;

        DateTime dllInfoTargetDate = DateTime.MinValue;

        //---------------------------------------------------------------------------------------------------
        // Calculate target date anyway, so we can set it to file.
        // I have made such logic that scripts will be compiled if date / time of main script or any sub-script is changed.
        //---------------------------------------------------------------------------------------------------
        List<long> times = filesToCompile.Select(x => File.GetLastWriteTime(x).Ticks).ToList();

        //
        // If we are referencing any local .dll file, add it's time into calculation scheme.
        //
        foreach (String refDll in csInfo.refFiles)
        {
            if (File.Exists(refDll))
                times.Add(File.GetLastAccessTime(refDll).Ticks);
        }

        // If syncProj.exe also changed, requires recompiling all .dll's.
        // GetEntryAssembly() returns null during unit testing.
        String exeFile = Assembly.GetExecutingAssembly().Location;
        times.Add(File.GetLastWriteTime(exeFile).Ticks);

        times.Sort();

        //---------------------------------------------------------------------------------------------------
        //  Basically we have multiple files, each with it's own modification date, we need to detect if any of files
        //  has changed  - either updated forth (svn update) or back (svn revert with set file dates to last commit time)
        //  We try to calculate date / time from multiple date times 
        //---------------------------------------------------------------------------------------------------

        long time = times[0];                               // smallest date/time
        for (int i = 1; i < times.Count; i += 2)
        {
            if (i + 1 == times.Count)
                time = (time + times[i]) / 2;               // medium between current date/time and highest
            else
                time += (times[i + 1] - times[i]) / 2;      // just take different between dates / times and get medium from there.
        }

        dllInfoTargetDate = new DateTime(time);
        if (times.Count != 1)
            dllInfoTargetDate.AddSeconds(-times.Count);     // Just some checksum on how many files we actually have.

        if (!bCompileDll)
        {

            if (dllInfoRealDate != dllInfoTargetDate)
                bCompileDll = true;
        }

        if (csInfo.DebugEnabled())
        {
            if( !bCompileDll )
                Console.WriteLine(Path.GetFileName(path) + " dll is up-to-date.");
            else
                Console.WriteLine(Path.GetFileName(path) + " dll will be compiled.");
            //+ ": Date found: " + dllInfoRealDate.ToString("o") + " Date expected: " + dllInfoTargetDate.ToString("o") 
        }

        if (bCompileDll)
        {
            // ----------------------------------------------------------------
            //  Compile it into ram
            // ----------------------------------------------------------------
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
            compilerparams.OutputAssembly = tempDll;
            compilerparams.CompilerOptions = "/d:DEBUG /d:TRACE";   // /debug+ /debug:full /optimize-
#endif

            // Add assemblies from my domain - all which are not dynamic.
            if (refAssemblies == null)
            {
                var assemblies = AppDomain.CurrentDomain.GetAssemblies().Where(a => !a.IsDynamic).Select(a => a.Location).ToList();

                for (int i = 0; i < assemblies.Count; i++)
                {
                    if (assemblies[i].EndsWith(".exe") && !assemblies[i].EndsWith("\\scriptStarter.exe"))
                    {
                        assemblies.RemoveAt(i);
                        i--;
                    }
                }

                refAssemblies = assemblies.ToArray();
            }
            compilerparams.ReferencedAssemblies.AddRange(refAssemblies);
            foreach( var f in csInfo.refFiles)
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

                    sb.AppendFormat(": error {0}: {1}\r\n", error.ErrorNumber, error.ErrorText );
                }
                errors = sb.ToString();
                if (bAllowThrow)
                    throw new Exception(errors);

                return false;
            }
        } //if

        try
        {
            File.SetLastWriteTime(dllInfoFile, dllInfoTargetDate);
        }
        catch (Exception)
        { 
            // Visual studio can launch multiple instances of syncProj, and then each will try to compile it's own copy.
            // Add here just some guard, let's check if this needs to be improved later on.
        }

        if (bCompileOnly)
            return true;

        //------------------------------------------------------------------------------------------------------
        //
        // Let's check that script contains correct css_ref (Might be copied from another project).
        // We allow here also multiple copies of syncProj, as long as path to syncProj.exe is valid in .cs header
        // (Can be edited by C# script)
        //
        //------------------------------------------------------------------------------------------------------
        Regex reCssRef = new Regex("^ *//css_ref  *(.*syncproj\\.exe);?([\r\n]+|$)", RegexOptions.Multiline | RegexOptions.IgnoreCase);
        bool bUpdateScriptPath = false;
        String targetCsPath = "";

        using (StreamReader reader = new StreamReader(path))
        {
            for (int i = 0; i < 10; i++)
            { 
                String line = reader.ReadLine() ?? "";
                var re = reCssRef.Match(line);
                if (re.Success)
                {
                    // Current path, referred from C# script
                    String currentCsPath = re.Groups[1].Value;
                    String dir = Path.GetDirectoryName(path);
                    String referredExe = currentCsPath;
                    
                    if( !Path.IsPathRooted(referredExe) )       // Uses relative path, let's make it absolute.
                        referredExe = Path.Combine(dir, currentCsPath);
                } //if
            } //for
        } //using

        // ----------------------------------------------------------------
        //  Preload compiled .dll and it's debug information into ram.
        // ----------------------------------------------------------------
        MethodInfo entry = null;
        String funcName = "";
        Assembly asm = Assembly.LoadFrom(tempDll);
            
        //Assembly asm = results.CompiledAssembly;
        // ----------------------------------------------------------------
        //  Locate entry point
        // ----------------------------------------------------------------
        BindingFlags flags = BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.IgnoreCase;

        foreach (Type type in asm.GetTypes())
        {
            funcName = "Reload";
            entry = type.GetMethod(funcName, flags);

            if (entry != null)
                break;
        }

        if( entry == null )
        {
            errors = String.Format( "{0}(1,1): error: Code does not have 'Main' function\r\n", Path.GetFileName(path) );
            if (bAllowThrow)
                throw new Exception(errors);
            return false;
        }

        if ( entry.GetParameters().Length != 1 )
        {
            errors = String.Format("{0}(1,1): error: Function '{1}' is not expected to have {2} parameter(s)\r\n", Path.GetFileName(path), funcName,entry.GetParameters().Length);
            if (bAllowThrow)
                throw new Exception(errors);
            return false;
            
        }

        String oldDir = Environment.CurrentDirectory;
        //
        // We set current directory to where script is, just so script can use Directory.GetFiles without specifying directory.
        //
        Directory.SetCurrentDirectory( Path.GetDirectoryName(_path) );

        // ----------------------------------------------------------------
        //  Run script
        // ----------------------------------------------------------------
        try
        {
            entry.Invoke(null, new object[] { args });
            Directory.SetCurrentDirectory( oldDir );
        }
        catch ( Exception ex )
        {
            Directory.SetCurrentDirectory( oldDir );

            try
                {
                StackFrame[] stack = new StackTrace(ex.InnerException, true).GetFrames();
                StackFrame lastCall = stack[0];

                errors = String.Format("{0}({1},{2}): error: {3}\r\n", path,
                    lastCall.GetFileLineNumber(), lastCall.GetFileColumnNumber(), ex.InnerException.Message);
                
            } catch (Exception ex3 )
            {
                errors = String.Format("{0}(1,1): error: Internal error - exception '{3}'\r\n", path, ex3.Message);
            }
            if (bAllowThrow)
                throw new Exception(errors);
            return false;
        }

        return true;
    } //RunScript

    static String GetUniqueTempFilename( String path )
    {
        String baseName = Path.GetFileNameWithoutExtension(path);
        string ProcID = Process.GetCurrentProcess().Id.ToString();
        string tmpFolder = Path.GetTempPath();
        string outFile = tmpFolder + baseName + "_" + ProcID;
        return outFile;
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


