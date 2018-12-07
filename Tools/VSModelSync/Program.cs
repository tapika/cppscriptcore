﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

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

    static String publicAsmPath = @"C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\PublicAssemblies";
    static Dictionary<String, XmlDocument> xmlDocs = new Dictionary<string, XmlDocument>();
    static Dictionary<String, StringBuilder> modelFiles = new Dictionary<string, StringBuilder>();


    /// <summary>
    /// Gets StringBuilder class where given type will be 'printed'
    /// </summary>
    static StringBuilder getFileFromType(Type type)
    {
        String asmName = Path.GetFileNameWithoutExtension(type.Assembly.Location);
        String name = asmName;

        if (type.IsEnum)
            name += "_enums";

        if (!modelFiles.ContainsKey(name))
            modelFiles.Add(name, new StringBuilder());

        return modelFiles[name];
    }



    static void DumpEnum(Type enumType)
    {
        String asmName = Path.GetFileNameWithoutExtension(enumType.Assembly.Location);
        StringBuilder sb = getFileFromType(enumType);

        if (!xmlDocs.ContainsKey(asmName))
        {
            String[] docFile = Directory.GetFiles(publicAsmPath, asmName + ".xml", SearchOption.AllDirectories);

            if (docFile.Length != 0)
            {
                xmlDocs.Add(asmName, new XmlDocument());
                xmlDocs[asmName].Load(docFile[0]);
            }
        }

        sb.AppendLine("public enum " + enumType.Name);
        sb.AppendLine("{");
        String[] names = Enum.GetNames(enumType);
        Array values = Enum.GetValues(enumType);
        for (int i = 0; i < names.Length; i++)
        {
            if (xmlDocs.ContainsKey(asmName))
            {
                string xmlName = "F:" + enumType.FullName + "." + names[i];
                XmlNode node = xmlDocs[asmName].SelectSingleNode("//member[starts-with(@name, '" + xmlName + "')]");
                sb.AppendLine("    /// " + node.InnerXml);
            }
            sb.AppendLine("    " + names[i] + " = " + (int)values.GetValue(i) + ",");
        }
        sb.AppendLine("}");
        sb.AppendLine();
    }


    static void Main(string[] args)
    {
        bool bTraceTypeFiltering = false;
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

        List<String> ignoreAsms = new List<string>();

        if (File.Exists("ignoreAsms.txt"))
            ignoreAsms = File.ReadAllText("ignoreAsms.txt").Split('\n').ToList();

        foreach (String dllPath in asmPathes)
        {
            Assembly asm = null;

            if (ignoreAsms.Contains(dllPath))
                continue;

            try
            {
                asm = Assembly.LoadFrom(dllPath);
            }
            catch { }

            if (asm == null)
            {
                ignoreAsms.Add(dllPath);
                continue;
            }

            if(bTraceTypeFiltering) Console.WriteLine(Path.GetFileName(asm.Location));

            bool bAtLeastOneTypeFound = false;

            foreach (Type type in GetLoadableTypes(asm))
            {
                String name = type.Name.ToLower();
                if (
                    !(
                        (name.Contains("configuration") && !name.Contains("configurations")) &&
                        !name.Contains("manager") &&
                        !name.Contains("callback") ||
                        name.Contains("linker")
                        )
                    )
                    continue;

                if (bTraceTypeFiltering) Console.WriteLine("    " + type.Name);

                //if (!name.Contains("vcconfiguration"))
                //    continue;

                int version = 1;
                String baseName = type.Name;

                Match m = new Regex("^(.*)(\\d+)$").Match(type.Name);
                if (m.Success)
                {
                    version = Int32.Parse(m.Groups[2].Value);
                    baseName = m.Groups[1].Value;
                }

                if (!asmToVersion.ContainsKey(baseName))
                {
                    asmToVersion.Add(baseName, version);
                    classNameToType.Add(baseName, type);
                    bAtLeastOneTypeFound = true;
                }
                else
                {
                    if (version > asmToVersion[baseName])
                    {
                        asmToVersion[baseName] = version;
                        classNameToType[baseName] = type;
                        bAtLeastOneTypeFound = true;
                    }
                }
            } //foreach Type

            if (!bAtLeastOneTypeFound)
                ignoreAsms.Add(dllPath);

        } //foreach asmPathes

        File.WriteAllText("ignoreAsms.txt", String.Join("\n", ignoreAsms));

        if (bTraceTypeFiltering)
        {
            Console.WriteLine("Collected types:");
            foreach (String baseName in asmToVersion.Keys)
            {
                Console.Write("    " + baseName);
                if (asmToVersion[baseName] != 1)
                    Console.Write("(" + asmToVersion[baseName] + ")");
                Console.WriteLine();
            }
        }

        Console.WriteLine("Processing types:");
        foreach (String typeName in classNameToType.Keys)
        {
            if (!typeName.StartsWith("VC") && !typeName.StartsWith("CSharp"))
                continue;

            Console.WriteLine("    " + typeName);
            Type type = classNameToType[typeName];

            StringBuilder sb = getFileFromType(type);
            sb.AppendLine("public class " + type.Name);
            sb.AppendLine("{");

            foreach (PropertyInfo pi in type.GetProperties())
            {
                Type pitype = pi.PropertyType;
                if (pitype.IsEnum)
                    DumpEnum(pitype);

                sb.AppendLine("    " + pitype.Name + " " + pi.Name + ";");
            }
            sb.AppendLine("};");
            sb.AppendLine();
        }

        String vsModelDir = "vsmodel";

        if ( !Directory.Exists(vsModelDir) )
            Directory.CreateDirectory(vsModelDir);

        foreach (String asmName in modelFiles.Keys)
        {
            File.WriteAllText(Path.Combine(vsModelDir, asmName + ".cs"), modelFiles[asmName].ToString());
        }

    }





}
