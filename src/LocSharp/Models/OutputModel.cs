namespace LocSharp.Models
{
    using Utils;

    public class OutputModel
    {
        public OutputModel(string language, int files, int blank, int comment, int code)
        {
            Language = Arg.NotNull(language, nameof(language));
            Files = files;
            Blank = blank;
            Comment = comment;
            Code = code;
        }

        public string Language { get; }

        public int Files { get; set; }

        public int Blank { get; set; }

        public int Comment { get; set; }

        public int Code { get; set; }
    }
}
