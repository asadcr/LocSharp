namespace LocSharp
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using Services;

    public static class Program
    {
        private static readonly string[] IgnoredFilePaths =
        {
            ".git"
        };

        private static readonly string[] IgnoredFiles =
        {
            "gitignore", "exe", "config", "sln", "DotSettings", "ruleset",
            "csproj", "txt", "csv", "ico", "mod", "pfa", "map", "lock",
            "browserslist", "png", "jpg"
        };

        private const string RepoPath = "C:\\CAFlowRepos\\esw-ai\\CAFlow_70e2a52d145b84149e2c846971a38646ffd19bc7";
        const string FilePath = "C:\\Users\\Asad\\Documents\\Visual Studio 2017\\Projects\\CAFlow\\src\\CAFlow.Core\\Services\\CodeGraphService.cs";

        public static void Main(string[] args)
        {
            var sw = Stopwatch.StartNew();

            var loc = Directory.EnumerateFiles(RepoPath, "*.*", SearchOption.AllDirectories)
                //.AsParallel()
                //.AsUnordered()
                .Where(file => !IgnoredFilePaths.Any(file.Contains))
                .Where(file => !IgnoredFiles.Any(ig => file.EndsWith(ig, StringComparison.OrdinalIgnoreCase)))
                .Select(LocService.GetFileInfo)
                .Select(a => a.Blank + a.Code + a.Comment)
                .Sum();

            
            Console.WriteLine($"{loc} in {sw.Elapsed.TotalSeconds.ToString()} sec");

            Console.ReadLine();
        }
    }
}
