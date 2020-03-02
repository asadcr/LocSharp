namespace LocSharp.Extensions
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text.RegularExpressions;
    using Models;
    using Utils;

    public static class CommonExtensions
    {
        public static IEnumerable<string> SplitToArray(this string str, string separator = null)
        {
            return string.IsNullOrEmpty(str) ?
                Enumerable.Empty<string>() :
                str.Split(separator ?? Environment.NewLine).Select(s => s.Trim());
        }

        public static string Replace(this string str, Regex regex, string replacement = "")
        {
            Arg.NotNull(str, nameof(str));
            Arg.NotNull(regex, nameof(regex));
            Arg.NotNull(replacement, nameof(replacement));

            return regex.Replace(str, replacement);
        }

        public static Regex ToRegex(this CommentDefinition definition)
        {
            Arg.NotNull(definition, nameof(definition));

            if (string.IsNullOrWhiteSpace(definition.End))
            {
                return new Regex($"({definition.Start}.*?$)", RegexOptions.Compiled);
            }

            return new Regex(
                $"({definition.Start}.*?{definition.End})",
                RegexOptions.Compiled | RegexOptions.Singleline);
        }
    }
}