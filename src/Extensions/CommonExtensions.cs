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

        public static Regex ToRegex(this CommentDefinition definition)
        {
            Arg.NotNull(definition, nameof(definition));

            if (string.IsNullOrWhiteSpace(definition.EndComment))
            {
                return new Regex($"({definition.StartComment}.*?$)", RegexOptions.Compiled);
            }

            return new Regex(
                $"({definition.StartComment}.*?{definition.EndComment})",
                RegexOptions.Compiled | RegexOptions.Singleline);
        }
    }
}
