namespace LocSharp.Models
{
    using System;
    using JetBrains.Annotations;
    using System.Collections.Generic;
    using Utils;

    public class LanguageDefinition
    {
        public LanguageDefinition(
            string name,
            ICollection<string> extensions,
            ICollection<CommentDefinition> commentDefinitions)
        {
            Name = Arg.NotNullOrWhitespace(name, nameof(name));
            Extensions = Arg.NotNullOrEmpty(extensions, nameof(extensions));
            CommentDefinitions = commentDefinitions ?? Array.Empty<CommentDefinition>();
        }

        public string Name { get; }

        public ICollection<string> Extensions { get; }

        [NotNull]
        public ICollection<CommentDefinition> CommentDefinitions { get; }
    }
}
