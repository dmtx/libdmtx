using System;
using System.Collections.Generic;
using System.Text;

namespace Libdmtx
{
    public class TestRunner
    {
        public static void Main(string[] args)
        {
            string assemblyFileName = typeof(TestRunner).Assembly.CodeBase;
            assemblyFileName = assemblyFileName.Substring("file:///".Length);
            string[] nunitArgs = new string[] { assemblyFileName };
            NUnit.ConsoleRunner.Runner.Main(nunitArgs);
            Console.WriteLine("Press any key to continue");
            Console.ReadKey();
        }
    }
}
