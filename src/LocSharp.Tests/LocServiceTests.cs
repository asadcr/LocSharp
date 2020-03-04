namespace LocSharp.Tests
{
    using System.IO;
    using Services;
    using Xunit;

    public class LocServiceTests
    {
        [Theory]
        [InlineData("C.c", 677, 449, 2494)]
        [InlineData("C2.c", 77, 68, 500)]
        [InlineData("CSharp.cs", 30, 25, 134)]
        //[InlineData("Go.txt", "go", 1, 1, 1)]
        //[InlineData("Groovy.txt", "groovy", 1, 1, 1)]
        [InlineData("Java.java",  46, 108, 256)]
        [InlineData("Java2.java", 9, 38, 33)]
        [InlineData("Java3.java", 6, 3, 152)]
        [InlineData("JavaScript.js", 35, 13, 370)]
        //[InlineData("MatLab.txt", "m", 1, 1, 1)]
        //[InlineData("PHP.txt", "php", 1, 1, 1)]
        //[InlineData("PlSql.txt", "prc", 1, 1, 1)]
        //[InlineData("PowerShell.txt", "ps1", 1, 1, 1)]
        //[InlineData("Python.txt", "py", 1, 1, 1)]
        //[InlineData("Ruby.txt", "rb", 1, 1, 1)]
        //[InlineData("Scala.txt", 1, 1, 1)]
        //[InlineData("Sql.txt", 1, 1, 1)]
        //[InlineData("Swift.txt", 1, 1, 1)]
        public void GetFileInfo_HasData_GetsFileInfo(string fileName, int blank, int comment, int code)
        {
            // Arrange
            var filePath = GetFilePath(fileName);

            // Act
            var info = LocService.GetFileInfo(filePath);

            //Assert.Null(info);
            Assert.Equal(blank, info.Blank);
            Assert.Equal(comment, info.Comment);
            Assert.Equal(code, info.Code);
        }

        private static string GetFilePath(string fileName)
        {
            var baseDir = Path.GetDirectoryName(typeof(LocServiceTests).Assembly.Location);

            return Path.Combine(baseDir, "TestData", fileName);
        }
    }
}
