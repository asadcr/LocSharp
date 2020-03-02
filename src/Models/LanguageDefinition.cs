namespace LocGuru.Models
{
    using JetBrains.Annotations;
    using System.Collections.Generic;
    using Utils;

    public class LanguageDefinition
    {
        public LanguageDefinition(
            string name,
            ICollection<string> extensions,
            CommentDefinition multiLineComment,
            ICollection<CommentDefinition> singleLineComments,
            [CanBeNull] string stringMarker = null,
            [CanBeNull] string removeMatchesRegex = null)
        {
            Name = Arg.NotNullOrWhitespace(name, nameof(name));
            Extensions = Arg.NotNullOrEmpty(extensions, nameof(extensions));

            MultiLineComment = Arg.NotNull(multiLineComment, nameof(multiLineComment));
            SingleLineComments = Arg.NotNull(singleLineComments, nameof(singleLineComments));
            StringMarker = stringMarker;
            RemoveMatchesRegex = removeMatchesRegex;
        }

        public string Name { get; }

        public ICollection<string> Extensions { get; }

        public CommentDefinition MultiLineComment { get; }

        [NotNull]
        public ICollection<CommentDefinition> SingleLineComments { get; }

        /// <summary>
        /// Gets the Comment Marker Character for Given Language
        /// </summary>
        [CanBeNull]
        public string StringMarker { get; }

        /// <summary>
        /// Gets the Regex For Language. Language can Require Pre-Processing
        /// So Comments Identification can process better.
        /// </summary>
        [CanBeNull]
        public string RemoveMatchesRegex { get; }
    }
}
