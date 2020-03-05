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
    using Newtonsoft.Json;
    using FileInfo = Models.FileInfo;

    public static class LocService
    {
        private const string Dash = "-";
        private const char SingleQuote = '\'';
        private const char DoubleQuote = '"';

        private static readonly IReadOnlyDictionary<string, LanguageDefinition> LanguageDefinitionsByExtension = FetchLanguageDefinitions();

        public static FileInfo GetFileInfo(string filePath)
        {
            Arg.NotNullOrWhitespace(filePath, nameof(filePath));

            var definition = GetLanguageDefinition(filePath);
            var lines = File.ReadLines(filePath);

            var classification = GetLineClassification(lines, definition)
                .GroupBy(g => g)
                .ToDictionary(a => a.Key, b => b.Count());

            return new FileInfo(filePath,
                definition.Name,
                classification.GetValueOrDefault(LineType.Blank),
                classification.GetValueOrDefault(LineType.Comment),
                classification.GetValueOrDefault(LineType.Code));
        }

        public static IEnumerable<LineType> GetLineClassification(string filePath)
        {
            var lines = File.ReadLines(filePath);
            var definition = GetLanguageDefinition(filePath);

            return GetLineClassification(lines, definition);
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
                    var currentLine = string.Concat(CleanLine(enumerator.Current));

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
                        var cleaned = string.Concat(CleanLine(enumerator.Current).ToArray());
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
        /// Replace Comments in string with empty string
        /// </summary>
        /// <param name="line">code line</param>
        /// <returns>list</returns>
        private static IEnumerable<char> CleanLine(string line)
        {
            IEnumerator<char> enumerator = line.GetEnumerator();

            while (enumerator.MoveNext())
            {
                var currentChar = enumerator.Current;

                switch (currentChar)
                {
                    case ' ':
                        continue;

                    case SingleQuote:
                    {
                        yield return SingleQuote;
                        enumerator = GetLastIndex(enumerator, SingleQuote);
                        yield return SingleQuote;
                        break;
                    }

                    case DoubleQuote:
                    {
                        yield return DoubleQuote;
                        enumerator = GetLastIndex(enumerator, DoubleQuote);
                        yield return DoubleQuote;
                        break;
                    }

                    default:
                        yield return currentChar;
                        break;
                }
            }

            enumerator.Dispose();
        }

        private static IEnumerator<char> GetLastIndex(IEnumerator<char> enumerator, char quote)
        {
            var list = new List<char>();

            while (enumerator.MoveNext())
            {
                if (enumerator.Current == quote)
                {
                    list.Clear();
                    continue;
                }

                list.Add(enumerator.Current);
            }

            return list.GetEnumerator();
        }

        private static IReadOnlyDictionary<string, LanguageDefinition> FetchLanguageDefinitions()
        {
            var definitions = File.ReadAllText(Path.Combine(CommonUtils.BasePath, "Languages", "Definitions.json"));

            var data = JsonConvert.DeserializeObject<IList<LanguageDefinition>>(definitions)
                .SelectMany(a => a.Extensions, (def, ext) => (Definition: def, Extension: ext));

            return data.ToDictionary(a => a.Extension, b => b.Definition, StringComparer.OrdinalIgnoreCase);
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