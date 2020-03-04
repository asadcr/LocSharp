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
        private static readonly IDictionary<CommentDefinition, Regex> CommentDefinitionsRegex = new Dictionary<CommentDefinition, Regex>(CommentDefinitionComparer.Instance);

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

        public static Regex ToCompiledRegex(this CommentDefinition definition)
        {
            Arg.NotNull(definition, nameof(definition));

            lock (CommentDefinitionsRegex)
            {
                return CommentDefinitionsRegex.GetOrAdd(definition, (def) =>
                {
                    if (string.IsNullOrWhiteSpace(def.End))
                    {
                        return new Regex($"({def.Start}.*?$)", RegexOptions.Compiled);
                    }

                    return new Regex(
                        $"({def.Start}.*?{def.End})",
                        RegexOptions.Compiled | RegexOptions.Singleline);
                });
            }
        }

        public static TValue GetOrAdd<TKey, TValue>(
            this IDictionary<TKey, TValue> dict,
            TKey key,
            Func<TKey, TValue> valueFactory)
        {
            Arg.NotNull(dict, nameof(dict));
            Arg.NotNull(valueFactory, nameof(valueFactory));

            return dict.TryGetValue(key, out var value)
                ? value
                : dict[key] = valueFactory(key);
        }
    }
}