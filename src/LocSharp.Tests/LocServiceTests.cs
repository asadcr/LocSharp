namespace LocSharp.Tests
{
    using System.IO;
    using System.Linq;
    using Services;
    using Models;
    using Xunit;

    public class LocServiceTests
    {
        [Fact]
        public void GetLineClassification_GetsClassification_DetectsCommentProperly()
        {
            var lines = LocService.GetLineClassification(GetTestFilePath("PHPConfig.php")).ToArray();

            var expectedLines = new[] { LineType.Code, LineType.Comment, LineType.Blank }
                .Concat(Enumerable.Range(1, 123).SelectMany(_ => new[] { LineType.Comment, LineType.Code }))
                .Concat(new[] { LineType.Blank, LineType.Comment })
                .Concat(Enumerable.Range(1, 82).SelectMany(_ => new[] { LineType.Blank, LineType.Code }))
                .Concat(new[] { LineType.Blank, LineType.Comment, LineType.Code, LineType.Blank, LineType.Code, LineType.Blank })
                .ToArray();

            Assert.Equal(expectedLines.Length, lines.Length);
            Assert.Equal(expectedLines, lines);
        }

        [Theory]
        [InlineData(1, 411, 256, "Java.java")]
        [InlineData(6, 80, 32, "Java2.java")]
        [InlineData(6, 161, 151, "Java3.java")]
        [InlineData(1, 189, 134, "CSharp.cs")]
        [InlineData(1, 241, 22, "PHP.php")]
        [InlineData(169, 3621, 2458, "C.c")]
        [InlineData(1479, 2005, 314, "C.c")]
        [InlineData(2062, 3621, 1139, "C.c")]
        [InlineData(1, 168, 140, "C.c")]
        [InlineData(58, 645, 534, "C2.c")]
        [InlineData(1, 419, 370, "JavaScript.js")]
        [InlineData(1, 205, 146, "Ruby.rb")]
        [InlineData(1, 34, 24, "Sql.sql")]
        [InlineData(1, 569, 412, "Python.py")]
        [InlineData(1, 40, 30, "Groovy.groovy")]
        [InlineData(1, 529, 348, "Go.go")]
        [InlineData(1, 195, 159, "Scala.scala")]
        [InlineData(1, 53, 12, "Swift.swift")]
        [InlineData(1, 800, 681, "PlSql.sql")]
        [InlineData(1, 421, 246, "MatLab.m")]
        [InlineData(1, 69, 40, "PowerShell.ps1")]
        public void CountCodeLines_CountsLanguageCodeLines(int blank, int comment, int code, string fileName)
        {
            // Arrange
            var filePath = GetTestFilePath(fileName);

            // Act
            var info = LocService.GetFileInfo(filePath);

            // Assert
            Assert.Equal(blank, info.Blank);
            Assert.Equal(comment, info.Comment);
            Assert.Equal(code, info.Code);
        }

        private static string GetTestFilePath(string fileName)
        {
            var baseDir = Path.GetDirectoryName(typeof(LocServiceTests).Assembly.Location);

            return Path.Combine(baseDir, "TestData", fileName);
        }
    }
}
