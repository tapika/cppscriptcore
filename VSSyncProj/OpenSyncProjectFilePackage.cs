using System;
using System.Linq;
using System.Collections;
using System.ComponentModel.Design;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using EnvDTE;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.OLE.Interop;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.VCProjectEngine;
using Microsoft.Win32;
using Task = System.Threading.Tasks.Task;
using Microsoft.VisualStudio.Project.VisualC.VCProjectEngine;
using System.Reflection;
using EnvDTE80;
using System.IO;

namespace VSSyncProj
{
    /// <summary>
    /// This is the class that implements the package exposed by this assembly.
    /// </summary>
    /// <remarks>
    /// <para>
    /// The minimum requirement for a class to be considered a valid package for Visual Studio
    /// is to implement the IVsPackage interface and register itself with the shell.
    /// This package uses the helper classes defined inside the Managed Package Framework (MPF)
    /// to do it: it derives from the Package class that provides the implementation of the
    /// IVsPackage interface and uses the registration attributes defined in the framework to
    /// register itself and its components with the shell. These attributes tell the pkgdef creation
    /// utility what data to put into .pkgdef file.
    /// </para>
    /// <para>
    /// To get loaded into VS, the package must be referred by &lt;Asset Type="Microsoft.VisualStudio.VsPackage" ...&gt; in .vsixmanifest file.
    /// </para>
    /// </remarks>
    [PackageRegistration(UseManagedResourcesOnly = true, AllowsBackgroundLoading = true)]
    //[PackageRegistration(UseManagedResourcesOnly = true, AllowsBackgroundLoading = false)]
    [InstalledProductRegistration("#110", "#112", "1.0", IconResourceID = 400)] // Info on this package for Help/About
    [ProvideMenuResource("Menus.ctmenu", 1)]
    [Guid(OpenSyncProjectFilePackage.PackageGuidString)]
    [SuppressMessage("StyleCop.CSharp.DocumentationRules", "SA1650:ElementDocumentationMustBeSpelledCorrectly", Justification = "pkgdef, VS and vsixmanifest are valid VS terms")]

    // Auto load when starting up
    [ProvideAutoLoad(UIContextGuids80.NoSolution)]
    // [ProvideService(typeof(VSSyncProjService))]
    public sealed class OpenSyncProjectFilePackage : AsyncPackage, IOleCommandTarget
    //public sealed class OpenSyncProjectFilePackage : Package
    {
        /// <summary>
        /// OpenSyncProjectFilePackage GUID string.
        /// </summary>
        public const string PackageGuidString = "913a89a0-d246-4e7a-b56c-fb182ea29f38";

        /// <summary>
        /// Initializes a new instance of the <see cref="OpenSyncProjectFilePackage"/> class.
        /// </summary>
        public OpenSyncProjectFilePackage()
        {
            // Inside this method you can place any initialization code that does not require
            // any Visual Studio service because at this point the package object is created but
            // not sited yet inside Visual Studio environment. The place to do all the other
            // initialization is the Initialize method.
        }

        DTE2 dte;

        /// <summary>
        /// Keep 'SolutionEvents' instance here for events to work.
        /// </summary>
        private SolutionEvents solutionEvents;
        private VCProjectEngineEvents projectEvents;

        /// <summary>
        /// Initialization of the package; this method is called right after the package is sited, so this is the place
        /// where you can put all the initialization code that rely on services provided by VisualStudio.
        /// </summary>
        /// <param name="cancellationToken">A cancellation token to monitor for initialization cancellation, which can occur when VS is shutting down.</param>
        /// <param name="progress">A provider for progress updates.</param>
        /// <returns>A task representing the async work of package initialization, or an already completed task if there is none. Do not return null from this method.</returns>
        protected override async Task InitializeAsync(CancellationToken cancellationToken, IProgress<ServiceProgressData> progress)
        {
            // When initialized asynchronously, the current thread may be a background thread at this point.
            // Do any initialization that requires the UI thread after switching to the UI thread.
            await this.JoinableTaskFactory.SwitchToMainThreadAsync(cancellationToken);
            await OpenSyncProjectFile.InitializeAsync(this);

            //VSSyncProjService serv = new VSSyncProjService();
            //((IServiceContainer)this).AddService(typeof(VSSyncProjService), serv);

            dte = this.GetService(typeof(SDTE)) as DTE2;
            solutionEvents = dte.Events.SolutionEvents;
            solutionEvents.AfterClosing += OnSolutionChanged;
            solutionEvents.Opened += OnSolutionChanged;
            await ExecuteScript.InitializeAsync(this);
        }

        private void OnSolutionChanged()
        {
            if (dte.Solution.Projects.Count == 0)
                return;

            Project p = dte.Solution.Projects.Item(1);
            var cfg = dte.Solution.SolutionBuild.ActiveConfiguration as EnvDTE80.SolutionConfiguration2;
            Debug.WriteLine(cfg.Name + "|" + cfg.PlatformName);

            VCProjectShim vcproject = (p.Object as VCProjectShim);

            //vcproject.ProjectGUID = "{612873E4-B1DE-4814-9477-864DDFDD40DA}";

            projectEvents = (VCProjectEngineEvents)dte.Events.GetObject("VCProjectEngineEventsObject");
            projectEvents.ItemPropertyChange2 += ProjectEvents_ItemPropertyChange2;

            var prjCfg = vcproject.ActiveConfiguration as VCConfiguration3;

            String platformName = ((VCPlatform)prjCfg.Platform).Name;
            Debug.WriteLine(prjCfg.ConfigurationName + "|" + platformName);
            IEnumerable projectTools = prjCfg.Tools as IEnumerable;

            foreach (var tobj in projectTools)
            {
                VCCLCompilerTool compilerTool = tobj as VCCLCompilerTool;
                if (compilerTool == null)
                    continue;

                Debug.WriteLine("Include directories:" + compilerTool.AdditionalIncludeDirectories);
            }
        }

        private void ProjectEvents_ItemPropertyChange2(object Item, string strPropertySheet, string strItemType, string PropertyName)
        {
            VCConfigurationShim conf = (VCConfigurationShim)Item;
            Debug.WriteLine("Configuration: " + conf.ConfigurationName + " Platform: " + conf.PlatformName + " in " + strPropertySheet + " type: " + strItemType + " name: " + PropertyName);
        }

        int IOleCommandTarget.QueryStatus(ref Guid commandGroup, uint commandsCount, OLECMD[] commands, IntPtr pCmdText)
        {
            if( ( commandGroup != OpenSyncProjectFile.CommandSet && commandGroup != ExecuteScript.CommandSet  )
                ||
                commandsCount != 1
            )
                return VSConstants.E_FAIL;

            // commands[0].cmdID - check if valid command
            commands[0].cmdf = (uint)(OLECMDF.OLECMDF_ENABLED | OLECMDF.OLECMDF_SUPPORTED);
            return VSConstants.S_OK;
        }

        /// <summary>
        /// Used to determine if the shell is querying for the parameter list of our command.
        /// </summary>
        private static bool IsQueryParameterList(IntPtr variantIn, IntPtr variantOut, uint nCmdexecopt)
        {
            ushort lo = (ushort)(nCmdexecopt & (uint)0xffff);
            ushort hi = (ushort)(nCmdexecopt >> 16);
            if (lo == (ushort)OLECMDEXECOPT.OLECMDEXECOPT_SHOWHELP)
            {
                if (hi == VsMenus.VSCmdOptQueryParameterList)
                {
                    if (variantOut != IntPtr.Zero)
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        int IOleCommandTarget.Exec(ref Guid commandGroup, uint commandId, uint commandExecOpt, IntPtr variantIn, IntPtr variantOut)
        {
            if (commandGroup != OpenSyncProjectFile.CommandSet && commandGroup != ExecuteScript.CommandSet)
                return VSConstants.E_FAIL;

            if (IsQueryParameterList(variantIn, variantOut, commandExecOpt))
            {
                Marshal.GetNativeVariantForObject("url", variantOut);
                return VSConstants.S_OK;
            }

            if(variantIn == IntPtr.Zero)
                return VSConstants.E_FAIL;

            // Commands that support parameters cannot be implemented via IMenuCommandService
            String file = Marshal.GetObjectForNativeVariant(variantIn) as String;

            //
            // Dll itself gets locked by executing process
            //
            //Assembly asm = Assembly.LoadFile(file);

            //
            // Asm gets loaded, but cannot be debugged
            //
            //byte[] asmbin = File.ReadAllBytes(file);
            //Assembly asm = Assembly.Load(asmbin);

            //
            // Can be debugged for first time, second, etc cannot debug / step in.
            //
            String pdbPath = Path.Combine(Path.GetDirectoryName(file), Path.GetFileNameWithoutExtension(file) + ".pdb");
            byte[] asmbin = File.ReadAllBytes(file);
            byte[] pdb = File.ReadAllBytes(pdbPath);
            Assembly asm = Assembly.Load(asmbin, pdb);

            foreach (Type type in asm.GetTypes())
            {
                MethodInfo mi = type.GetMethod("init", BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.IgnoreCase);
                if (mi == null)
                    continue;

                mi.Invoke(null, new object[] { dte });
                return VSConstants.S_OK;
            }

            return VSConstants.E_FAIL;
        }

    }
}
