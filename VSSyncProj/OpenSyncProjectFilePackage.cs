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
    public sealed class OpenSyncProjectFilePackage : AsyncPackage
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

        DTE dte;

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

            dte = this.GetService(typeof(SDTE)) as DTE;
            solutionEvents = dte.Events.SolutionEvents;
            solutionEvents.AfterClosing += OnSolutionChanged;
            solutionEvents.Opened += OnSolutionChanged;
        }

        private void OnSolutionChanged()
        {
            if (dte.Solution.Projects.Count == 0)
                return;

            Project p = dte.Solution.Projects.Item(1);
            var cfg = dte.Solution.SolutionBuild.ActiveConfiguration as EnvDTE80.SolutionConfiguration2;
            Debug.WriteLine(cfg.Name + "|" + cfg.PlatformName);

            VCProjectShim vcproject = (p.Object as VCProjectShim);
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

    }
}
