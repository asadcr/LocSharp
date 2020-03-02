namespace LocGuru.Utils
{
    using System.IO;
    using System.Reflection;

    public static class CommonUtils
    {
        public static string BasePath => Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
    }
}
