namespace LocGuru.Utils
{
    using System;
    using System.Collections.Generic;
    using JetBrains.Annotations;

    public static class Arg
    {
        [NotNull]
        public static string NotNullOrWhitespace(string arg, [InvokerParameterName] string argName)
        {
            if (arg == null)
            {
                throw new ArgumentNullException(argName);
            }

            if (string.IsNullOrWhiteSpace(arg))
            {
                throw new ArgumentException($"{argName} can not be empty or whitespace.", argName);
            }

            return arg;
        }

        [NotNull]
        public static T NotNull<T>([NoEnumeration] T arg, [InvokerParameterName] string argName)
            where T : class
        {
            return arg ?? throw new ArgumentNullException(argName);
        }

        [NotNull]
        public static ICollection<T> NotNullOrEmpty<T>(ICollection<T> arg, [InvokerParameterName] string argName)
        {
            NotNull(arg, argName);

            if (arg.Count == 0)
            {
                throw new ArgumentException($"{argName} collection can not be empty.", argName);
            }

            return arg;
        }

        [NotNull]
        public static IReadOnlyList<T> NotNullOrEmpty<T>(IReadOnlyList<T> arg, [InvokerParameterName] string argName)
        {
            NotNull(arg, argName);

            if (arg.Count == 0)
            {
                throw new ArgumentException($"{argName} collection can not be empty.", argName);
            }

            return arg;
        }

        public static T InRange<T>(T arg, T minInclusive, T maxInclusive, [InvokerParameterName] string argName)
            where T : IComparable
        {
            if (arg.CompareTo(minInclusive) < 0 || arg.CompareTo(maxInclusive) > 0)
            {
                throw new ArgumentOutOfRangeException(
                    argName,
                    arg,
                    $"{argName} should be between '{minInclusive}' and '{maxInclusive}'.");
            }

            return arg;
        }

        public static T NotDefault<T>(T arg, [InvokerParameterName] string argName)
            where T : struct
        {
            if (EqualityComparer<T>.Default.Equals(arg, default))
            {
                throw new ArgumentException($"{argName} has default value.", argName);
            }

            return arg;
        }
    }
}