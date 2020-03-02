namespace LocSharp.Models
{
    using JetBrains.Annotations;
    using Utils;

    public class CommentDefinition
    {
        public CommentDefinition(
            [NotNull] string startComment,
            [CanBeNull] string endComment = null)
        {
            StartComment = Arg.NotNull(startComment, nameof(startComment));
            EndComment = endComment ?? string.Empty;
        }

        [NotNull]
        public string StartComment { get; }

        [CanBeNull]
        public string EndComment { get; }
    }
}
