using System;
using System.Runtime.InteropServices;
using Thread = System.Threading.Thread;

/// <summary>
/// Utility functions.
/// </summary>
public class Utils
{
    /// <summary>
    /// Calls a function, retrying if necessary.
    /// </summary><remarks>
    /// DTE functions call into an instance of Visual Studio running in
    /// a separate process using COM interop. These calls can fail if 
    /// Visual Studio is busy, and in these cases we get a COM exception.
    /// 
    /// This function will retry calling the function if this happens, and
    /// will only fail if it has retried 20 times without success.
    /// 
    /// You pass in the function - or property - to call usually as a
    /// lambda. For example, to get the projects.Count property you would 
    /// call:
    /// 
    ///   int count = dteCall[int](() => (projects.Count));
    ///   
    /// (Note: replace the [] above with angle-brackets to specify the 
    ///        return type.)
    /// </remarks>
    public static T call<T>(Func<T> fn)
    {
        int numTries = 20;
        int intervalMS = 100;

        // We will try to call the function a number of times...
        for (int i = 0; i < numTries; ++i)
        {
            try
            {
                // We call the function passed in and return the result...
                return fn();
            }
            catch (COMException ex)
            {
                if ((uint)ex.ErrorCode == 0x8001010a) // RPC_E_SERVERCALL_RETRYLATER: The message filter indicated that the application is busy.
                {
                    // Server is busy - we sleep for a short while, and then try again...
                    Thread.Sleep(intervalMS);
                }
                else
                    throw ex;
            }
        }

        throw new Exception(String.Format("'call' failed to call function after {0} tries.", numTries));
    }

    /// <summary>
    /// Calls a function with no return value, retrying if necessary.
    /// </summary>
    public static void callVoidFunction(Action fn)
    {
        int numTries = 20;
        int intervalMS = 50;

        // We will try to call the function a number of times...
        for (int i = 0; i < numTries; ++i)
        {
            try
            {
                // We call the function passed in, and return
                // if it succeeds...
                fn();
                return;
            }
            catch (COMException ex)
            {
                if ((uint)ex.ErrorCode == 0x8001010a) // RPC_E_SERVERCALL_RETRYLATER: The message filter indicated that the application is busy.
                {
                    // Server is busy - we sleep for a short while, and then try again...
                    Thread.Sleep(intervalMS);
                }
                else
                    throw ex;
            }
        }

        throw new Exception(String.Format("'callVoidFunction' failed to call function after {0} tries.", numTries));
    }

}


