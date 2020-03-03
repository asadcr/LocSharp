namespace LocSharp.Services
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Text.RegularExpressions;
    using Extensions;
    using Models;
    using Utils;
    using MoreLinq;
    using Newtonsoft.Json;
    using FileInfo = Models.FileInfo;

    public static class LocService
    {
        private const string Dash = "-";

        private static readonly Regex DoubleQuoteRegex = new Regex("\".*?\"", RegexOptions.Compiled);
        private static readonly Regex SingleQuoteRegex = new Regex("'.*?'", RegexOptions.Compiled);
        private static readonly Regex CommonSingleLineComment = new CommentDefinition(@"\/\/").ToCompiledRegex();
        private static readonly Regex CommonMultiLineComment = new CommentDefinition("\\/\\*", "\\*\\/").ToCompiledRegex();
        private static readonly IReadOnlyDictionary<string, LanguageDefinition> LanguageDefinitionsByExtension = FetchLanguageDefinitions();

        public static FileInfo GetFileInfo(string filePath)
        {
            Arg.NotNullOrWhitespace(filePath, nameof(filePath));

            var definition = GetLanguageDefinition(filePath);
            var lines = File.ReadLines(filePath);

            var classification = GetLineClassification(lines, definition)
                .GroupBy(g => g)
                .ToDictionary(a => a.Key, b => b.Count());

            return new FileInfo(definition.Name,
                classification.GetValueOrDefault(LineType.Blank),
                classification.GetValueOrDefault(LineType.Comment),
                classification.GetValueOrDefault(LineType.Code));
        }

        private static IEnumerable<LineType> GetLineClassification(IEnumerable<string> lines, LanguageDefinition model)
        {
            using (var enumerator = lines.GetEnumerator())
            {
                var multiLineComment = model.CommentDefinitions.SingleOrDefault(a => !string.IsNullOrWhiteSpace(a.End));
                var multiLineStartRegex = multiLineComment == null ? null : new Regex($"({multiLineComment.Start}.*)", RegexOptions.Compiled);
                var multiLineRegex = multiLineComment?.ToCompiledRegex();

                var singleLineComments = model.CommentDefinitions.Where(a => string.IsNullOrWhiteSpace(a.End));
                var singleLineRegexes = singleLineComments.Select(pattern => pattern.ToCompiledRegex()).ToArray();

                while (enumerator.MoveNext())
                {
                    var currentLine = CleanLine(enumerator.Current);

                    if (string.IsNullOrWhiteSpace(currentLine))
                    {
                        yield return LineType.Blank;

                        continue;
                    }

                    if (singleLineRegexes.Length > 0)
                    {
                        currentLine = singleLineRegexes.Aggregate(currentLine, (current, regex) =>
                            regex.Replace(current, string.Empty));
                    }

                    // Comments are replaced and Only Spaces are left. Means it has No Code and is classified as Comment
                    if (string.IsNullOrWhiteSpace(currentLine))
                    {
                        yield return LineType.Comment;

                        continue;
                    }

                    if (multiLineStartRegex == null)
                    {
                        yield return LineType.Code;

                        continue;
                    }

                    var multiLineCommentStarted = multiLineStartRegex.IsMatch(currentLine);

                    // No-Comments. Pure Code
                    if (!multiLineCommentStarted)
                    {
                        yield return LineType.Code;

                        continue;
                    }

                    var mergedLines = string.Join(Environment.NewLine, currentLine);

                    while (!multiLineRegex.IsMatch(mergedLines) && enumerator.MoveNext())
                    {
                        // Always Clean the Line for accuracy.
                        var cleaned = CleanLine(enumerator.Current);
                        mergedLines = string.Join(Environment.NewLine, mergedLines, cleaned);
                    }

                    // Get original line count eg, 5. Count remaining lines eg, 2. means 3 comments were replaced. 
                    // Report all 5 accordingly
                    var replacedLines = multiLineRegex.Replace(mergedLines, (match) =>
                    {
                        // Replace with Dash So When Splitting the array back we get the same element count.
                        var replaced = match.Groups[1].Value.SplitToArray().Select(_ => Dash);

                        return string.Join(Environment.NewLine, replaced);
                    });

                    foreach (var replacedLine in replacedLines.SplitToArray())
                    {
                        // Comments are replaced and Only Spaces are left. Means it has No Code and is classified as Comment
                        var type = string.IsNullOrWhiteSpace(replacedLine) || replacedLine == Dash ? LineType.Comment : LineType.Code;

                        yield return type;
                    }
                }
            }
        }

        /// <summary>
        /// Replace Comments in string with empty string and Removes Matches in Strings
        /// </summary>
        /// <param name="line">code line</param>
        /// <returns>list</returns>
        private static string CleanLine(string line)
        {
            // Ported from CLOC https://github.com/AlDanial/cloc/blob/master/cloc#L5991
            var cleanedLine = line
                .Replace(SingleQuoteRegex)
                .Replace(DoubleQuoteRegex);

            //return string.IsNullOrEmpty(removeMatchesRegex) ? cleanedLine : Regex.Replace(cleanedLine, removeMatchesRegex, string.Empty);

            return cleanedLine;
        }

        private static IReadOnlyDictionary<string, LanguageDefinition> FetchLanguageDefinitions()
        {
            var definitions = File.ReadAllText(Path.Combine(CommonUtils.BasePath, "Languages", "Definitions.json"));

            return JsonConvert.DeserializeObject<IList<LanguageDefinition>>(definitions)
                .SelectMany(a => a.Extensions, (def, ext) => (Definition: def, Extension: ext))
                .ToDictionary(a => a.Extension, b => b.Definition, StringComparer.OrdinalIgnoreCase);
        }

        private static LanguageDefinition GetLanguageDefinition(string path)
        {
            var ext = Path.GetExtension(path).TrimStart('.');

            if (string.IsNullOrWhiteSpace(ext))
            {
                ext = CommonUtils.GetFileName(path);
            }

            return LanguageDefinitionsByExtension[ext];
        }
    }
}