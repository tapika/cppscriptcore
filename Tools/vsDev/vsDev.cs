//css_ref \Prototyping\cppscriptcore\ScriptEngine\packages\Microsoft.VisualStudio.Shell.15.0.15.0.26228\lib\Microsoft.VisualStudio.Shell.15.0.dll
//css_ref C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\PublicAssemblies\EnvDTE80.dll
//css_ref \Prototyping\cppscriptcore\bin\ScriptEngine.dll
//css_ref \Prototyping\cppscriptcore\ScriptEngine\packages\Microsoft.VisualStudio.Shell.Interop.7.10.6071\lib\Microsoft.VisualStudio.Shell.Interop.dll
//css_ref \Prototyping\cppscriptcore\ScriptEngine\packages\Microsoft.VisualStudio.OLE.Interop.7.10.6071\lib\Microsoft.VisualStudio.OLE.Interop.dll
//css_ref \Prototyping\cppscriptcore\ScriptEngine\packages\Microsoft.VisualStudio.Shell.Interop.8.0.8.0.50727\lib\Microsoft.VisualStudio.Shell.Interop.8.0.dll
//css_ref \Prototyping\cppscriptcore\ScriptEngine\packages\Microsoft.VisualStudio.Shell.Interop.9.0.9.0.30729\lib\Microsoft.VisualStudio.Shell.Interop.9.0.dll
//css_ref \Prototyping\cppscriptcore\ScriptEngine\packages\Microsoft.VisualStudio.Shell.Interop.10.0.10.0.30319\lib\Microsoft.VisualStudio.Shell.Interop.10.0.dll
//css_ref \Prototyping\cppscriptcore\ScriptEngine\packages\Microsoft.VisualStudio.Shell.Interop.14.0.DesignTime.14.3.25407\lib\Microsoft.VisualStudio.Shell.Interop.14.0.DesignTime.dll
//css_ref \Prototyping\cppscriptcore\ScriptEngine\packages\Microsoft.VisualStudio.Shell.Framework.15.0.26228\lib\net45\Microsoft.VisualStudio.Shell.Framework.dll
//css_ref C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\PublicAssemblies\EnvDTE.dll
//css_ref \Prototyping\cppscriptcore\bin\ScriptEngineStarter.exe
//css_ref C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\PublicAssemblies\Microsoft.VisualStudio.VCProjectEngine.dll
//css_ref C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\CommonExtensions\Microsoft\VC\Project\Microsoft.VisualStudio.Project.VisualC.VCProjectEngine.dll
//css_include ..\VSModelSync\CodeBuilder.cs
using EnvDTE;
using EnvDTE80;
using Microsoft.VisualStudio.Project.VisualC.VsShell.Interop;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.VCProjectEngine;
using ScriptEngine;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using Constants = EnvDTE.Constants;

public class vsDev
{
    public static bool Main( object arg )
    {
        ScriptEnginePackage sepkg = (ScriptEnginePackage)arg;
        Solution solution = sepkg.dte.Solution;

        ScriptHost.console.Clear();
        ScriptHost.console.WriteLine("Build: " + Assembly.GetExecutingAssembly().Location);
        ScriptHost.console.WriteLine("---------------------------------------------------------------------------------");
        ScriptHost.console.WriteLine(solution.FileName);
        //ScriptHost.console.WriteLine(solution.IsOpen.ToString());

        Project[] projects = solution.Projects.Cast<Project>().ToArray();
        foreach (Project project in projects)
        {
            String path = project.FileName;

            //
            // The "Miscellaneous Files" node is used to contain open files that are not associated 
            // with the current project contents within the solution
            //
            // https://stackoverflow.com/questions/7160425/what-is-miscellaneous-files-inside-dte-vs2010-solution
            //
            if (project.Kind == Constants.vsProjectKindMisc)
                continue;

            String dir = Path.GetDirectoryName(path);
            String scriptBase = Path.GetFileNameWithoutExtension(path);

            String script = Path.Combine(dir, scriptBase + "_proto.cs");

            CodeBuilder code = new CodeBuilder();

            code.AppendLine(
@"//css_ref syncproj.exe
using System;

class Builder: SolutionProjectBuilder
{
    static void Main(String[] args)
    {");
            code.Indent(2);

            String name = project.Name;
            code.AppendLine("project(" + quoted(name) + ");");

            Configuration[] configurations = project.ConfigurationManager.Cast<Configuration>().ToArray();
            code.AppendLine("configurations(" + String.Join(",", configurations.Select(x => quoted(x.ConfigurationName)).Distinct() ) + ");");

            IVsHierarchy hierarchy;
            sepkg.vsSolution.GetProjectOfUniqueName(project.UniqueName, out hierarchy);
            Guid projectGuid;
            hierarchy.GetGuidProperty(Microsoft.VisualStudio.VSConstants.VSITEMID_ROOT, (int)__VSHPROPID.VSHPROPID_ProjectIDGuid, out projectGuid);

            code.AppendLine("uuid(" + quoted(projectGuid.ToString()) + ");");


            String[] paths = null;

            // C++ Project
            VCProject vcProject = project.Object as VCProject;
            if (vcProject != null)
            {
                VCFile[] files = ((IVCCollection)vcProject.Files).Cast<VCFile>().ToArray();
                paths = files.Select(x => x.RelativePath).ToArray();
            }

            // C# Project
            VSProject2 sharpProject = project.Object as VSProject2;
            if(sharpProject != null)
            {
                ProjectItem[] files = project.ProjectItems.Cast<ProjectItem>().ToArray();

                paths = new string[files.Length];
                for (int i = 0; i < files.Length; i++)
                {
                    Property[] properties = files[i].Properties.Cast<Property>().ToArray();
                    String[] keys = properties.Select(x => x.Name).ToArray();
                    paths[i] = properties.Where(x => x.Name == "FileName").Select(x => x.Value.ToString()).FirstOrDefault();
                }
            }


            code.AppendLine("files(");
                code.Indent();
                for(int i = 0; i < paths.Length; i++)
                {
                    code.Append(code.IndentString + quoted(paths[i]));

                    if ( i != paths.Length - 1)
                        code.Append(",");

                    code.Append("\r\n");
                }
                code.UnIndent();
                code.AppendLine(");");

            code.UnIndent();
            code.AppendLine("}");
            code.UnIndent();
            code.AppendLine("}");

            ScriptHost.console.WriteLine("Saving " + script + "...");
            File.WriteAllText(script, code.ToString());
        }
        return false;
    }


    public static String quoted(String s)
    {
        return "\"" + s.Replace("\"", "\\\"") + "\"";
    }


}


