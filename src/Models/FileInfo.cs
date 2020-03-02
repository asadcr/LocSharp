namespace LocSharp.Models
{
    using Utils;

    public class FileInfo
    {
        public FileInfo(string languageName, int blankLines, int commentLines, int codeLines)
        {
            Language = Arg.NotNull(languageName, nameof(languageName));
            Blank = blankLines;
            Comment = commentLines;
            Code = codeLines;
        }

        public string Language { get; }

        public int Blank { get; }

        public int Comment { get; }

        public int Code { get; }

        public override string ToString()
        {
            return $"{nameof(Language)}={Language}:{nameof(Blank)}={Blank}:{nameof(Comment)}={Comment}:{nameof(Code)}={Code}";
        }
    }
}
