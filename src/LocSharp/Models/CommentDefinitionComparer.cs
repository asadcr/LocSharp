using System;
using System.Collections.Generic;

namespace LocSharp.Models
{
    public class CommentDefinitionComparer : IEqualityComparer<CommentDefinition>
    {
        public static readonly CommentDefinitionComparer Instance = new CommentDefinitionComparer();

        private const StringComparison Comparison = StringComparison.InvariantCultureIgnoreCase;

        public bool Equals(CommentDefinition x, CommentDefinition y)
        {
            if (ReferenceEquals(x, y))
            {
                return true;
            }

            if (ReferenceEquals(x, null))
            {
                return false;
            }

            if (ReferenceEquals(y, null))
            {
                return false;
            }

            if (x.GetType() != y.GetType())
            {
                return false;
            }

            return string.Equals(x.Start, y.Start, Comparison) &&
                   string.Equals(x.End, y.End, Comparison);
        }

        public int GetHashCode(CommentDefinition obj)
        {
            unchecked
            {
                var hashCode = (obj.Start != null ? obj.Start.GetHashCode(Comparison) : 0);
                hashCode = (hashCode * 397) ^ (!string.IsNullOrWhiteSpace(obj.End) ? obj.End.GetHashCode(Comparison) : 0);

                return hashCode;
            }
        }
    }
}
