namespace LocSharp.Utils
{
    using System.IO;
    using System.Linq;
    using System.Reflection;

    public static class CommonUtils
    {
        private const char DirectorySeparatorChar = '/';
        private const char AltDirectorySeparatorChar = '\\';

        public static string BasePath => Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);

        public static string NormalizePath(string path)
        {
            Arg.NotNull(path, nameof(path));

            return path.Replace(AltDirectorySeparatorChar, DirectorySeparatorChar);
        }

        public static string GetFileName(string path)
        {
            Arg.NotNull(path, nameof(path));

            return NormalizePath(path).TrimEnd(DirectorySeparatorChar).Split(DirectorySeparatorChar).Last();
        }
    }
}
