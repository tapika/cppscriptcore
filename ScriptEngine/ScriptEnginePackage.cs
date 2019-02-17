using System;
using System.ComponentModel.Design;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;
using System.Threading;
using EnvDTE80;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using Task = System.Threading.Tasks.Task;

namespace ScriptEngine
{
    [PackageRegistration(UseManagedResourcesOnly = true, AllowsBackgroundLoading = true)]
    [InstalledProductRegistration("#110", "#112", "1.0", IconResourceID = 400)] // Info on this package for Help/About
    [Guid(ScriptEnginePackage.PackageGuid)]
    [SuppressMessage("StyleCop.CSharp.DocumentationRules", "SA1650:ElementDocumentationMustBeSpelledCorrectly", Justification = "pkgdef, VS and vsixmanifest are valid VS terms")]
    [ProvideMenuResource("Menus.ctmenu", 1)]

    // Auto load when starting up
    [ProvideAutoLoad(UIContextGuids80.NoSolution)]
    public sealed class ScriptEnginePackage : AsyncPackage
    {
        public const string PackageGuid = "dc06e809-f64c-49e4-b9d5-d6e6d60dee7c";
        public static readonly Guid CommandSet = new Guid("6fd609b8-64f9-4636-a95b-d3322b87b185");
        public const int CommandId = 0x0100;

        public dynamic vsModule;
        public DTE2 dte;

        /// <summary>
        /// Initialization of the package; this method is called right after the package is sited, so this is the place
        /// where you can put all the initialization code that rely on services provided by VisualStudio.
        /// </summary>
        /// <param name="cancellationToken">A cancellation token to monitor for initialization cancellation, which can occur when VS is shutting down.</param>
        /// <param name="progress">A provider for progress updates.</param>
        /// <returns>A task representing the async work of package initialization, or an already completed task if there is none. Do not return null from this method.</returns>
        protected override async Task InitializeAsync(CancellationToken cancellationToken, IProgress<ServiceProgressData> progress)
        {
            // Switch to main thread
            await this.JoinableTaskFactory.SwitchToMainThreadAsync(cancellationToken);
            dte = await this.GetServiceAsync(typeof(EnvDTE.DTE)) as DTE2;

            // Register menu handlers
            OleMenuCommandService commandService = await GetServiceAsync((typeof(IMenuCommandService))) as OleMenuCommandService;
            if (commandService == null)
                return;

            commandService.AddCommand(new MenuCommand(this.Execute, new CommandID(CommandSet, CommandId)));
            ScriptHost.ScriptServer_ConnectDebugger(this);
        }

        private void Execute(object sender, EventArgs e)
        {
            VsShellUtilities.ShowMessageBox(this, "test 1", "title 1", OLEMSGICON.OLEMSGICON_INFO, OLEMSGBUTTON.OLEMSGBUTTON_OK, OLEMSGDEFBUTTON.OLEMSGDEFBUTTON_FIRST);
        }
    }
}

