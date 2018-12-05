using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;

class Program
{
    public static IEnumerable<Type> GetLoadableTypes(Assembly assembly)
    {
        try
        {
            return assembly.GetTypes();
        }
        catch (ReflectionTypeLoadException e)
        {
            return e.Types.Where(t => t != null);
        }
    }

    static void Main(string[] args)
    {
        String publicAsmPath = @"C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\PublicAssemblies";
        String[] allPaths =
        {
            publicAsmPath,
            @"C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\PrivateAssemblies",
            // Microsoft.VisualStudio.DataDesign.Interfaces.dll...
            @"C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE"
        };
        String[] asmPathes = Directory.GetFiles(publicAsmPath, "*.dll");

        AppDomain.CurrentDomain.AssemblyResolve += (s, asmArgs) =>
        {
            String dllName = new AssemblyName(asmArgs.Name).Name + ".dll";

            foreach (String dir in allPaths)
            {
                string path = Path.Combine(dir, dllName);

                if (!File.Exists(path))
                    continue;

                return Assembly.LoadFrom(path);
            }

            // Console.WriteLine("Warning: Required assembly not found: " + dllName);
            return null;
        };

        Dictionary<String, int> asmToVersion = new Dictionary<string, int>();
        Dictionary<String, Type> classNameToType = new Dictionary<string, Type>();



        foreach (String dllPath in asmPathes)
        {
            Assembly asm = null;
            try
            {
                asm = Assembly.LoadFrom(dllPath);
            }
            catch { }

            if (asm == null)
                continue;

            Console.WriteLine(Path.GetFileName(asm.Location));

            foreach (Type type in GetLoadableTypes(asm))
            {
                String name = type.Name.ToLower();
                if (
                    !(
                        (name.Contains("configuration") && !name.Contains("configurations")) &&
                        !name.Contains("manager") &&
                        !name.Contains("callback")
                        )
                    )
                    continue;

                Console.WriteLine("    " + type.Name);

                //if (!name.Contains("vcconfiguration"))
                //    continue;

                Match m = new Regex("^(.*)(\\d+)$").Match(type.Name);
                if (!m.Success)
                    continue;

                int version = Int32.Parse(m.Groups[2].Value);
                String baseName = m.Groups[1].Value;

                if (!asmToVersion.ContainsKey(baseName))
                {
                    asmToVersion.Add(baseName, version);
                    classNameToType.Add(baseName, type);
                }
                else
                {
                    if (version > asmToVersion[baseName])
                    {
                        asmToVersion[baseName] = version;
                        classNameToType[baseName] = type;
                    }
                }
            }
        } //foreach asmPathes

    }
}
