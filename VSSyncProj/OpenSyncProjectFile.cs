using System;
using System.Linq;
using System.ComponentModel.Design;
using System.Globalization;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using Task = System.Threading.Tasks.Task;
using EnvDTE;
using EnvDTE80;

namespace VSSyncProj
{
    /// <summary>
    /// Command handler
    /// </summary>
    internal sealed class OpenSyncProjectFile
    {
        /// <summary>
        /// Command ID.
        /// </summary>
        public const int CommandId = 0x0100;

        /// <summary>
        /// Command menu group (command set GUID).
        /// </summary>
        public static readonly Guid CommandSet = new Guid("1f855adc-7f28-48c7-8ec3-035066af45d0");

        /// <summary>
        /// VS Package that provides this command, not null.
        /// </summary>
        private readonly AsyncPackage package;
        private DTE2 dte;
        private ErrorListProvider _errorListProvider;

        /// <summary>
        /// Initializes a new instance of the <see cref="OpenSyncProjectFile"/> class.
        /// Adds our command handlers for menu (commands must exist in the command table file)
        /// </summary>
        /// <param name="package">Owner package, not null.</param>
        /// <param name="commandService">Command service to add command to, not null.</param>
        private OpenSyncProjectFile(AsyncPackage package, OleMenuCommandService commandService)
        {
            this.package = package ?? throw new ArgumentNullException(nameof(package));
            commandService = commandService ?? throw new ArgumentNullException(nameof(commandService));

            var menuCommandID = new CommandID(CommandSet, CommandId);
            var menuItem = new MenuCommand(this.Execute, menuCommandID);
            commandService.AddCommand(menuItem);
        }

        /// <summary>
        /// Gets the instance of the command.
        /// </summary>
        public static OpenSyncProjectFile Instance
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the service provider from the owner package.
        /// </summary>
        private Microsoft.VisualStudio.Shell.IAsyncServiceProvider ServiceProvider
        {
            get
            {
                return this.package;
            }
        }

        /// <summary>
        /// Initializes the singleton instance of the command.
        /// </summary>
        /// <param name="package">Owner package, not null.</param>
        public static async Task InitializeAsync(AsyncPackage package)
        {
            // Switch to the main thread - the call to AddCommand in OpenSyncProjectFile's constructor requires
            // the UI thread.
            await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync(package.DisposalToken);

            OleMenuCommandService commandService = await package.GetServiceAsync((typeof(IMenuCommandService))) as OleMenuCommandService;
            Instance = new OpenSyncProjectFile(package, commandService);
            Instance.dte = await package.GetServiceAsync(typeof(EnvDTE.DTE)) as EnvDTE80.DTE2;
            Instance._errorListProvider = new ErrorListProvider(package);
        }


        void TraceMessage(string msg)
        {
            OutputWindowPanes panes = dte.ToolWindows.OutputWindow.OutputWindowPanes;
            OutputWindowPane pane;
            String cppScript = "C++ Script";
            try
            {
                pane = panes.Item(cppScript);
            }
            catch (ArgumentException)
            {
                pane = panes.Add(cppScript);
            }

            pane.OutputString(msg + "\n");
            pane.Activate();
            dte.ToolWindows.OutputWindow.Parent.Activate();
            return;

            //foreach (OutputWindowPane pane in panes)
            //{
            //    if (pane.Name.Contains("Build"))
            //    {
            //        pane.OutputString(msg + "\n");
            //        pane.Activate();
            //        //dte.ToolWindows.OutputWindow.Parent.Activate();
            //        return;
            //    }
            //}

            _errorListProvider.Tasks.Clear();
            var task = new ErrorTask
            {
                Document = @"d:\PrototypingQuick\VSSyncProj\OpenSyncProjectFile.cs",
                Line = 105,
                Column = 17,
                ErrorCategory = TaskErrorCategory.Error,
                Category = TaskCategory.BuildCompile,
                Text = "Hello error panel"
            };
            task.Navigate += Task_Navigate;
            _errorListProvider.Tasks.Add(task);
            _errorListProvider.Show();
            _errorListProvider.BringToFront();
        }

        private void Task_Navigate(object sender, EventArgs e)
        {
            ErrorTask task = (ErrorTask)sender;
            dte.ItemOperations.OpenFile(task.Document);
            (dte.ActiveDocument.Selection as TextSelection).MoveTo(task.Line, task.Column);
        }

        /// <summary>
        /// This function is the callback used to execute the command when the menu item is clicked.
        /// See the constructor to see how the menu item is associated with this function using
        /// OleMenuCommandService service and MenuCommand class.
        /// </summary>
        /// <param name="sender">Event sender.</param>
        /// <param name="e">Event args.</param>
        private void Execute(object sender, EventArgs e)
        {
            ThreadHelper.ThrowIfNotOnUIThread();
            string message = string.Format(CultureInfo.CurrentCulture, "Inside {0}.MenuItemCallback()", this.GetType().FullName);

            TraceMessage("Ok");
        //    string title = "OpenSyncProjectFile";

        //    // Show a message box to prove we were here
        //    VsShellUtilities.ShowMessageBox(
        //        this.package,
        //        message,
        //        title,
        //        OLEMSGICON.OLEMSGICON_INFO,
        //        OLEMSGBUTTON.OLEMSGBUTTON_OK,
        //        OLEMSGDEFBUTTON.OLEMSGDEFBUTTON_FIRST);
        }

    }
}
