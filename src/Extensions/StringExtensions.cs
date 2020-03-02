namespace LocGuru.Extensions
{
    using System;
    using System.Collections.Generic;
    using System.Linq;

    public static class StringExtensions
    {
        public static IEnumerable<string> SplitToArray(this string str, string separator = null)
        {
            return string.IsNullOrEmpty(str) ?
                Enumerable.Empty<string>() :
                str.Split(separator ?? Environment.NewLine).Select(s => s.Trim());
        }
    }
}
