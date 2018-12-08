using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.Xml.Linq;
using System.Xml.Serialization;
using VSSync.VCProjectEngine;

class Program
{
    static void Main(string[] args)
    {
        //VCLinkerTool link = new VCLinkerTool();
        //link.SubSystem = subSystemOption.subSystemConsole;

        //XmlSerializer xsSubmit = new XmlSerializer(typeof(VCLinkerTool));
        //var xml = "";

        //using (var sww = new StringWriter())
        //{
        //    using (XmlWriter writer = XmlWriter.Create(sww))
        //    {
        //        xsSubmit.Serialize(writer, link);
        //        xml = sww.ToString();
        //    }
        //}

        String f = @"\PrototypingQuick\ConsoleApplication1\ConsoleApplication1.vcxproj";
        XDocument doc = XDocument.Load(f, LoadOptions.PreserveWhitespace);
        doc.Save(f + "2.xml", SaveOptions.DisableFormatting);
    }


}

