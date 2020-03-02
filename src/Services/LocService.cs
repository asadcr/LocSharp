namespace LocGuru
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Text.RegularExpressions;
    using Extensions;
    using Models;
    using MoreLinq;
    using Utils;
    using FileInfo = Models.FileInfo;

    public class LocService
    {
        private const string DoubleQuoteString = "\"";
        private const string SingleQuoteString = "'";
        private const string Dash = "-";
        private static readonly CommentDefinition CommonSingleLineComment = new CommentDefinition(@"\/\/");
        private static readonly CommentDefinition CommonMultiLineComment = new CommentDefinition(@"\/\*", @"\*\/");

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
                var multiLineStartRegex = new Regex($"({model.MultiLineComment.StartComment}.*)", RegexOptions.Compiled);
                var singleLineRegexes = model.SingleLineComments.Select(pattern => new Regex($"({pattern.StartComment}.*?$)", RegexOptions.Compiled)).ToArray();
                var multiLineRegex = new Regex(
                    $"({model.MultiLineComment.StartComment}.*?{model.MultiLineComment.EndComment})",
                    RegexOptions.Compiled | RegexOptions.Singleline);

                while (enumerator.MoveNext())
                {
                    var currentLine = CleanLine(enumerator.Current, model.StringMarker, model.RemoveMatchesRegex);

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
                        var cleaned = CleanLine(enumerator.Current, model.StringMarker, model.RemoveMatchesRegex);
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
        /// <param name="stringMarker">string quote which language uses</param>
        /// <param name="removeMatchesRegex">Regex to remove Matches</param>
        /// <returns>list</returns>
        private static string CleanLine(string line, string stringMarker, string removeMatchesRegex)
        {
            // Ported from CLOC https://github.com/AlDanial/cloc/blob/master/cloc#L5991
            var cleanedLine = string.IsNullOrEmpty(stringMarker) ? line : Regex.Replace(line, $"{stringMarker}.*{stringMarker}", "\"\"");

            return string.IsNullOrEmpty(removeMatchesRegex) ? cleanedLine : Regex.Replace(cleanedLine, removeMatchesRegex, string.Empty);
        }

        private static LanguageDefinition GetLanguageDefinition(string filePath)
        {
            var file = File.ReadLines(Path.Combine(CommonUtils.BasePath, "Languages", "Definitions.txt"))
                .Split(string.IsNullOrWhiteSpace)
                .Select(group =>
                {
                    var arr = group.ToArray();
                    var languageName = arr.First();
                    var data = arr.Skip(1)
                        .Select(item =>
                        {
                            var split = item.Trim().Split(" ", 2);

                            return KeyValuePair.Create(split[0], split[1]);
                        })
                        .GroupBy(g => g.Key)
                        .ToDictionary(g => g.Key, b => b.Select(bb => bb.Value).ToArray());

                    return new LanguageDefinition(languageName, data["extension"], null, null);
                })
                .ToArray();

            return null;
        }
    }
}