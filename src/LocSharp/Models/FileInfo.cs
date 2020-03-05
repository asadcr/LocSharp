namespace LocSharp.Models
{
    using Utils;

    public class FileInfo
    {
        public FileInfo(string filePath, string languageName, int blankLines, int commentLines, int codeLines)
        {
            FilePath = Arg.NotNullOrWhitespace(filePath, nameof(filePath));
            Language = Arg.NotNullOrWhitespace(languageName, nameof(languageName));
            Blank = blankLines;
            Comment = commentLines;
            Code = codeLines;
        }

        public string FilePath { get; }

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
