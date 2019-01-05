using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;
using System.Xml.Linq;

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

    //C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\PublicAssemblies

    static String vsInstallPath = @"C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise";
    //static String vsInstallPath = @"C:\Program Files (x86)\Microsoft Visual Studio\2019\Preview";

    static String[] publicAsmPaths = {
        Path.Combine(vsInstallPath, @"Common7\IDE\PublicAssemblies"),
        // VCProjectShim is here.
        Path.Combine(vsInstallPath, @"Common7\IDE\CommonExtensions\Microsoft\VC\Project")
    };


    static Dictionary<String, XmlDocument> xmlDocs = new Dictionary<string, XmlDocument>();
    static Dictionary<String, CodeBuilder> modelFiles = new Dictionary<string, CodeBuilder>();
    static Dictionary<String, bool> savedTypes = new Dictionary<string, bool>();


    static String getPlannedNamespace(Type type)
    {
        String ns = type.Namespace.Replace("Microsoft.VisualStudio", "VSSync");
        return ns;
    }

    /// <summary>
    /// Gets CodeBuilder class where given type will be 'printed'
    /// </summary>
    /// <returns>null if type is going outside of public location</returns>
    static CodeBuilder getFileFromType(Type type, Action<CodeBuilder> onCreateCb = null)
    {
        String asmPath = type.Assembly.Location;
        bool bCreated = false;
        //if (!asmPath.StartsWith(publicAsmPath) && !type.Name.StartsWith("CSharpProjectConfigurationProperties"))
        //    return null;

        String asmName = Path.GetFileNameWithoutExtension(asmPath);
        String name = asmName;

        if (type.IsEnum)
            name += "_enums";

        CodeBuilder cb;

        if (!modelFiles.ContainsKey(name))
        {
            cb = new CodeBuilder();
            modelFiles.Add(name, cb);
            cb.AppendLine("using System;");
            bCreated = true;
        }
        else {
            cb = modelFiles[name];
        }

        // Specify namespace using what where we keep original type.
        String ns = getPlannedNamespace(type);

        // Already saved, no need to double save
        if (savedTypes.ContainsKey(ns + "." + type.Name))
            return null;
        savedTypes.Add(ns + "." + type.Name, true);

        if (cb.GetUserData<String>("namespace") != ns)
        {
            cb.SetUserData("namespace", ns);

            if (bCreated)
            {
                if (onCreateCb != null) onCreateCb(cb);
                cb.AppendLine();
            }

            if (cb.IndentValue() != 0)
            {
                cb.UnIndent();
                cb.AppendLine("}");
            }

            cb.AppendLine("namespace " + ns);
            cb.AppendLine("{");
            cb.AppendLine();
            cb.Indent();
        }

        return modelFiles[name];
    }


    /// <summary>
    /// Gets documentation xml file from type
    /// </summary>
    static XmlDocument getDocumentation(Type type)
    {
        String asmName = Path.GetFileNameWithoutExtension(type.Assembly.Location);

        if (!xmlDocs.ContainsKey(asmName))
        {
            String[] docFile = Directory.GetFiles(publicAsmPaths[0], asmName + ".xml", SearchOption.AllDirectories);

            xmlDocs.Add(asmName, new XmlDocument());

            if (docFile.Length != 0)
                xmlDocs[asmName].Load(docFile[0]);
        }

        return xmlDocs[asmName];
    }

    static String getComments(XmlDocument doc, String commentStart, String xmlPath)
    {
        XmlNode node = doc.SelectSingleNode("//member[starts-with(@name, '" + xmlPath + "')]");
        if (node == null)
            return "";
        XElement elements = XElement.Load(node.CreateNavigator().ReadSubtree());
        String comments = String.Join("\n", elements.Elements().Select( x => commentStart + x) );
        return comments + "\n";
    }



    static void DumpEnum(Type enumType)
    {
        String asmName = Path.GetFileNameWithoutExtension(enumType.Assembly.Location);
        CodeBuilder cb = getFileFromType(enumType);

        if (cb == null)
            return;

        cb.AppendLine("public enum " + enumType.Name);
        cb.AppendLine("{");
        String[] names = Enum.GetNames(enumType);
        Array values = Enum.GetValues(enumType);
        XmlDocument doc = getDocumentation(enumType);

        for (int i = 0; i < names.Length; i++)
        {
            cb.Append(getComments(doc, cb.IndentString + "/// ", "F:" + enumType.FullName + "." + names[i]));
            cb.AppendLine("    " + names[i] + " = " + (int)values.GetValue(i) + ",");
        }
        cb.AppendLine("}");
        cb.AppendLine();
    }


    static void Main(string[] args)
    {
        bool bTraceTypeFiltering = false;
        String[] allPaths =
        {
            publicAsmPaths[0],
            publicAsmPaths[1],
            Path.Combine(vsInstallPath, @"Enterprise\Common7\IDE\PrivateAssemblies"),
            // Microsoft.VisualStudio.DataDesign.Interfaces.dll...
            Path.Combine(vsInstallPath, @"Common7\IDE"),
            // Microsoft.VisualStudio.ProjectSystem.VS.dll
            Path.Combine(vsInstallPath, @"Common7\IDE\CommonExtensions\Microsoft\Project")
            
        };

        String[] asmPathes = Directory.GetFiles(publicAsmPaths[0], "*.dll").
            Concat(
                Directory.GetFiles(publicAsmPaths[1], "*.dll")
            ).ToArray();

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

                if (bTraceTypeFiltering) Console.WriteLine("    " + type.Name);

                if (
                    !(
                        (name.Contains("configuration") && !name.Contains("configurations")) &&
                        !name.Contains("manager") &&
                        !name.Contains("callback") ||
                        name.Contains("linker") ||
                        name.Contains("project")
                        //name.Contains("serviceprovider")
                        )
                    )
                    continue;

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

        Dictionary<String, List<String>> requiredNamespaces = new Dictionary<string, List<string>>();

        Console.WriteLine("Processing types:");
        foreach (String action in new String[] { "getinfo", "collect" })
        {
            foreach (String typeName in classNameToType.Keys)
            {
                if (!typeName.StartsWith("VC") && !typeName.StartsWith("CSharp"))
                    continue;

                Type type = classNameToType[typeName];

                // Collect namespace dependencies.
                if (action == "getinfo")
                {
                    String ns = getPlannedNamespace(type);

                    if (!requiredNamespaces.ContainsKey(ns))
                        requiredNamespaces.Add(ns, new List<string>());
                    List<String> namespaces= requiredNamespaces[ns];

                    foreach (PropertyInfo pi in type.GetProperties())
                    {
                        Type pitype = pi.PropertyType;

                        if (pitype != typeof(String) && pitype != typeof(Boolean) && !pitype.IsEnum)
                            continue;

                        String targetNs = getPlannedNamespace(pitype);
                        if (ns != targetNs && targetNs != "System" && !namespaces.Contains(targetNs))
                            namespaces.Add(targetNs);
                    }

                    continue;
                }

                Console.WriteLine("    " + typeName);

                CodeBuilder cb = getFileFromType(type, (cb2) =>
                {
                    // Dump all dependent namespace as "using ..."
                    foreach (String ns2 in requiredNamespaces[cb2.GetUserData<String>("namespace")])
                        cb2.AppendLine("using " + ns2 + ";");
                });

                if (cb == null)
                    continue;

                cb.AppendLine("public class " + type.Name);
                cb.AppendLine("{");

                XmlDocument doc = getDocumentation(type);


                foreach (PropertyInfo pi in type.GetProperties())
                {
                    Type pitype = pi.PropertyType;
                    if (pitype.IsEnum)
                        DumpEnum(pitype);

                    if (pitype != typeof(String) && pitype != typeof(Boolean) && !pitype.IsEnum)
                    {
                        cb.AppendLine("    // " + pitype.Name + " " + pi.Name + ";");
                        cb.AppendLine();
                        continue;
                    }
                    String propertyPath = type.Namespace  + "." + typeName + "." + pi.Name;

                    cb.Indent();
                    cb.Append(getComments(doc, cb.IndentString + "/// ", "P:" + propertyPath));
                    cb.AppendLine("public " + pitype.Name + " " + pi.Name + ";");
                    cb.UnIndent();
                    cb.AppendLine();
                }
                cb.AppendLine("};");
                cb.AppendLine();
            }
        }

        String vsModelDir = Path.Combine(Path.GetDirectoryName(Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location)), "vsmodel");

        if ( !Directory.Exists(vsModelDir) )
            Directory.CreateDirectory(vsModelDir);

        foreach (String asmName in modelFiles.Keys)
        {
            CodeBuilder cb = modelFiles[asmName];

            // Close all open braces.
            while (cb.IndentValue() != 0)
            {
                cb.UnIndent();
                cb.AppendLine("}");
            }

            File.WriteAllText(Path.Combine(vsModelDir, asmName + ".cs"), cb.ToString());
        }

    }





}
