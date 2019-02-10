using System;
using System.Collections.Generic;
using System.IO.MemoryMappedFiles;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

public class IpcChannel
{
    EventWaitHandle ask;
    EventWaitHandle reply;
    MemoryMappedFile mmf;

    public IpcChannel(int processId)
    {
        ask = new EventWaitHandle(false, EventResetMode.ManualReset, "ask_Scripter_IpcChannel_" + processId);
        reply = new EventWaitHandle(false, EventResetMode.ManualReset, "reply_Scripter_IpcChannel_" + processId);
        mmf = MemoryMappedFile.CreateOrOpen("Scripter_IpcChannel_" + processId, 1000);
    }

    public bool Send(String msg)
    {
        using (MemoryMappedViewStream stream = mmf.CreateViewStream())
        {
            byte[] buf = Encoding.UTF8.GetBytes(msg);
            byte[] bufLen = BitConverter.GetBytes(buf.Length);
            stream.Write(bufLen, 0, bufLen.Length);
            stream.Write(buf, 0, buf.Length);
        }
        ask.Set();

        // Really long wait, process start can take time
        bool b = reply.WaitOne(10000);
        reply.Reset();

        return b;
    }

    public bool Receive(ref String s, int timeout = 1000)
    {
        if (!ask.WaitOne(timeout))
            return false;

        using (MemoryMappedViewStream stream = mmf.CreateViewStream())
        {
            byte[] buf = new byte[8];
            stream.Read(buf, 0, 4);
            int l = BitConverter.ToInt32(buf, 0);
            buf = new byte[l];
            stream.Read(buf, 0, l);
            s = Encoding.UTF8.GetString(buf);
        }

        reply.Set();
        ask.Reset();
        return true;
    }

}
