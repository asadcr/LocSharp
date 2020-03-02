namespace LocSharp
{
    using System;
    using Services;

    public static class Program
    {
        public static void Main(string[] args)
        {
            const string filePath = "C:\\Users\\Asad\\Documents\\Visual Studio 2017\\Projects\\CAFlow\\src\\CAFlow.Core\\Services\\CodeGraphService.cs";

            var info = LocService.GetFileInfo(filePath);

            Console.WriteLine(info.ToString());

            Console.ReadLine();
        }
    }
}
