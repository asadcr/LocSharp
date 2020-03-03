namespace LocSharp.Models
{
    using JetBrains.Annotations;
    using Utils;

    public class CommentDefinition
    {
        public CommentDefinition(
            string start,
            [CanBeNull] string end = null)
        {
            Start = Arg.NotNull(start, nameof(start));
            End = end ?? string.Empty;
        }

        public string Start { get; }

        [CanBeNull]
        public string End { get; }

        public override string ToString()
        {
            return $"{Start}.*?{End ?? "$"}";
        }
    }
}
