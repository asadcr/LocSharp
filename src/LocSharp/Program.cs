namespace LocSharp
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices.ComTypes;
    using System.Text;
    using Extensions;
    using Models;
    using MoreLinq.Extensions;
    using Services;
    using Utils;
    using FileInfo = Models.FileInfo;

    public static class Program
    {
        private static readonly string[] IgnoredFilePaths =
        {
            ".git"
        };

        private static readonly string[] IgnoredFiles =
        {
            "gitignore", "exe", "config", "sln", "DotSettings", "ruleset",
            "txt", "csv", "ico", "mod", "pfa", "map", "lock",
            "browserslist", "png", "jpg"
        };

        private const string RepoPath = "C:\\Users\\Asad\\Documents\\Visual Studio 2017\\Projects\\CAFlow\\src\\CAFlow.Core.Tests\\TestData\\Fixtures\\JFlexLexer.java";

        public static void Main(string[] args)
        {
            var sw = Stopwatch.StartNew();
            var resultsPerFile = GetFileInfo(RepoPath);

            OutputByLanguage(resultsPerFile, sw);

            Console.ReadLine();
        }

        private static void OutputByFile(ParallelQuery<FileInfo> infos, Stopwatch sw)
        {
            infos.Select(r => $"{r.FilePath},{r.Language},{r.Blank},{r.Comment},{r.Code}").ForAll(Console.WriteLine);
        }

        private static void OutputByLanguage(ParallelQuery<FileInfo> infos, Stopwatch sw)
        {
            var resultsPerLanguage = infos
                .GroupBy(g => g.Language)
                .Select(b =>
                {
                    var arr = b.ToArray();

                    return new OutputModel(b.Key, arr.Length, arr.Sum(a => a.Blank), arr.Sum(a => a.Comment), arr.Sum(a => a.Code));
                })
                .ToArray();

            var totalLines = resultsPerLanguage.Sum(o => o.Blank + o.Code + o.Comment);
            var totalFiles = resultsPerLanguage.Sum(o => o.Files);

            var dashedLine = new string('-', 85);

            var title = GetTitle(sw.Elapsed.TotalSeconds, totalFiles, totalLines);

            Console.WriteLine(title);

            Console.WriteLine(dashedLine);
            Console.WriteLine(GetTitleDisplayLine());
            Console.WriteLine(dashedLine);

            foreach (var result in resultsPerLanguage.OrderByDescending(o => o.Code))
            {
                Console.WriteLine(GetDisplayLine(result));
            }

            Console.WriteLine(dashedLine);
            Console.WriteLine(GetDisplayLine(resultsPerLanguage.Aggregate(
                new OutputModel("Total", 0, 0, 0, 0),
                (model, item) => {
                    model.Comment += item.Comment;
                    model.Files += item.Files;
                    model.Blank += item.Blank;
                    model.Code += item.Code;

                    return model;
                })));
            Console.WriteLine(dashedLine);
        }

        private static ParallelQuery<FileInfo> GetFileInfo(string path)
        {
            Arg.NotNullOrWhitespace(path, nameof(path));

            var files = Directory.Exists(path) ?
                Directory.EnumerateFiles(path, "*.*", SearchOption.AllDirectories) :
                File.Exists(path)
                    ? new[] {path}
                    : throw new InvalidOperationException("Invalid Path");

            return files
                .AsParallel()
                .AsUnordered()
                .Where(file => !IgnoredFilePaths.Any(file.Contains))
                .Where(file => !IgnoredFiles.Any(ig => file.EndsWith(ig, StringComparison.OrdinalIgnoreCase)))
                .Select(LocService.GetFileInfo);
        }

        private static string GetTitle(double elapsedSeconds, int totalFiles, int totalLines)
        {
            var fps = Math.Round(totalFiles / elapsedSeconds, 2);
            var lps = Math.Round(totalLines / elapsedSeconds, 2);

            return $"github.com/asadcr/LocSharp v1.00  T={Math.Round(elapsedSeconds, 2)}s ({fps} files/s, {lps} lines/s)";
        }

        private static string GetDisplayLine(OutputModel model)
        {
            var sb = new StringBuilder();
            sb.Append(model.Language.ToFixedLength(25));
            sb.Append(model.Files.ToString().ToFixedLength(15, true));
            sb.Append(model.Blank.ToString().ToFixedLength(15, true));
            sb.Append(model.Comment.ToString().ToFixedLength(15, true));
            sb.Append(model.Code.ToString().ToFixedLength(15, true));

            return sb.ToString();
        }

        private static string GetTitleDisplayLine()
        {
            return nameof(OutputModel.Language).ToFixedLength(25) +
                   nameof(OutputModel.Files).ToFixedLength(15, true) +
                   nameof(OutputModel.Blank).ToFixedLength(15, true) +
                   nameof(OutputModel.Comment).ToFixedLength(15, true) +
                   nameof(OutputModel.Code).ToFixedLength(15, true);
        }
    }
}
