namespace LocGuru.Models
{
    using Utils;

    public class LineInfo
    {
        public LineInfo(int lineNumber, LineType type, string data)
        {
            LineNumber = Arg.InRange(lineNumber, 1, int.MaxValue, nameof(lineNumber));
            Type = type;
            Data = Arg.NotNull(data, nameof(data));
        }

        public int LineNumber { get; }

        public LineType Type { get; }

        public string Data { get; }
    }
}
