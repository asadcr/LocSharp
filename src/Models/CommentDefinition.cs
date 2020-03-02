namespace LocSharp.Models
{
    using JetBrains.Annotations;
    using Utils;

    public class CommentDefinition
    {
        public CommentDefinition(
            [NotNull] string start,
            [CanBeNull] string end = null)
        {
            Start = Arg.NotNull(start, nameof(start));
            End = end ?? string.Empty;
        }

        [NotNull]
        public string Start { get; }

        [CanBeNull]
        public string End { get; }
    }
}
