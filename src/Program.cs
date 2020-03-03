namespace LocSharp
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Text;
    using MoreLinq.Extensions;
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
                .AsParallel()
                .AsUnordered()
                .Where(file => !IgnoredFilePaths.Any(file.Contains))
                .Where(file => !IgnoredFiles.Any(ig => file.EndsWith(ig, StringComparison.OrdinalIgnoreCase)))
                .Select(LocService.GetFileInfo)
                .ToArray()
                .GroupBy(g => g.Language)
                .ToDictionary(d => d.Key, b =>
                {
                    var arr = b.ToArray();

                    return new
                    {
                        Blank = arr.Sum(a => a.Blank),
                        Comment = arr.Sum(a => a.Comment),
                        Code = arr.Sum(a => a.Code)
                    };
                });

            Console.WriteLine($"{loc.Count} in {sw.Elapsed.TotalSeconds.ToString()} sec");

            foreach (var (language, value) in loc.OrderByDescending(o => o.Value.Blank + o.Value.Code + o.Value.Comment))
            {
                var someString = new StringBuilder(new string(' ', 30), 30);
                language.ForEach((l, i) => someString[i] = l);

                Console.WriteLine($"{someString}\t{value.Blank}\t\t{value.Comment}\t\t{value.Code}");
            }

            Console.ReadLine();
        }
    }
}
