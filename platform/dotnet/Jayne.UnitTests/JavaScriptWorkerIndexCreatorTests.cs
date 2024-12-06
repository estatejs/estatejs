using System.IO;
using System.Linq;
using Estate.Jayne.ApiModels;
using Estate.Jayne.Common;
using Estate.Jayne.Common.Exceptions;
using Estate.Jayne.Errors;
using Estate.Jayne.Models;
using Estate.Jayne.Models.Protocol;
using Estate.Jayne.Services.Impl;
using Xunit;

// ReSharper disable InconsistentNaming

namespace Estate.Jayne.UnitTests
{
    public class JavaScriptWorkerIndexCreatorTests
    {
        private void ValidatePlayerClass(ServiceClassInfo playerClass, int classOffset)
        {
            Assert.Equal(classOffset + 1, playerClass.ClassId);
            Assert.Equal("Player", playerClass.ClassName);

            var m = playerClass.Methods.ToArray();
            Assert.Equal(3, m.Length);

            Assert.Equal(1, m[0].MethodId);
            Assert.Equal("name", m[0].MethodName);
            Assert.Null(m[0].ReturnType);
            Assert.Single(m[0].Arguments);
            var m0a = m[0].Arguments.Single();
            Assert.Equal("name_value", m0a.Name);
            Assert.Equal("any", m0a.Type);

            Assert.Equal(2, m[1].MethodId);
            Assert.Equal("current_name", m[1].MethodName);
            Assert.Equal("any", m[1].ReturnType);
            Assert.Empty(m[1].Arguments);

            Assert.Equal(3, m[2].MethodId);
            Assert.Equal("reset_name_to", m[2].MethodName);
            Assert.Equal("any", m[2].ReturnType);
            Assert.Single(m[2].Arguments);
            var m2a = m[2].Arguments.Single();
            Assert.Equal("reset_name_to_value", m2a.Name);
            Assert.Equal("any", m2a.Type);
        }

        private void ValidateGameClass(ServiceClassInfo gameClass, int classOffset)
        {
            Assert.Equal(classOffset + 1, gameClass.ClassId);
            Assert.Equal("Game", gameClass.ClassName);

            var m = gameClass.Methods.ToArray();
            Assert.Equal(3, m.Length);

            Assert.Equal(1, m[0].MethodId);
            Assert.Equal("game_name", m[0].MethodName);
            Assert.Null(m[0].ReturnType);
            Assert.Single(m[0].Arguments);
            var m0a = m[0].Arguments.Single();
            Assert.Equal("game_name_value", m0a.Name);
            Assert.Equal("any", m0a.Type);

            Assert.Equal(2, m[1].MethodId);
            Assert.Equal("current_game_name", m[1].MethodName);
            Assert.Equal("any", m[1].ReturnType);
            Assert.Empty(m[1].Arguments);

            Assert.Equal(3, m[2].MethodId);
            Assert.Equal("reset_game_name_to", m[2].MethodName);
            Assert.Equal("any", m[2].ReturnType);
            Assert.Single(m[2].Arguments);
            var m2a = m[2].Arguments.Single();
            Assert.Equal("reset_game_name_to_value", m2a.Name);
            Assert.Equal("any", m2a.Type);
        }

        private void ValidateFile(string name, WorkerFileNameInfo playerFile)
        {
            Assert.Equal(name, playerFile.FileName);
        }

        private WorkerFileContent LoadWorkerFile(string fileName)
        {
            return new WorkerFileContent
            {
                code = File.ReadAllText(Path.Combine("test_data", fileName)),
                name = fileName
            };
        }

        private WorkerFileContent LoadWorkerFileAbs(string path)
        {
            return new WorkerFileContent
            {
                code = File.ReadAllText(path),
                name = Path.GetFileName(path)
            };
        }

        [Fact]
        public void DuplicateClassThrows()
        {
            if (!Log.IsInitialized)
                Log.Init(new FakeLogger<object>());

            //arrange
            var workerFiles = new[]
            {
                LoadWorkerFile("player.js"),
                LoadWorkerFile("player_dup.js"),
            };

            var creator = new JavaScriptParserServiceImpl();

            //act/assert
            var bex =
                Assert.Throws<BusinessLogicException<BusinessLogicErrorCode>>(
                    () => creator.ParseWorkerCode(1, 1, "Test", workerFiles, null, null));
            // Assert.Equal(BusinessLogicErrorCode.DuplicateClassName, bex.Code);
        }

        [Fact]
        public void BadCodeThrows()
        {
            if (!Log.IsInitialized)
                Log.Init(new FakeLogger<object>());

            //arrange
            var workerFiles = new[]
            {
                LoadWorkerFile("bad_code.js")
            };

            var creator = new JavaScriptParserServiceImpl();

            //act/assert
            var bex = Assert.Throws<BusinessLogicException<BusinessLogicErrorCode>>(
                () => creator.ParseWorkerCode(1, 1, "Test", workerFiles, null, null));
            Assert.Equal(BusinessLogicErrorCode.UnknownParserError, bex.Code);
        }

        [Fact]
        public void CanCreateTwoClassIndexFromOneFile()
        {
            //arrange
            var workerFiles = new[]
            {
                LoadWorkerFile("player_game.js")
            };

            var creator = new JavaScriptParserServiceImpl();

            //act
            var result = creator.ParseWorkerCode(1, 1, "Test", workerFiles, null, null);
            var workerIndex = result.WorkerIndex;
            
            //assert
            Assert.Single(workerIndex.FileNames);
            Assert.Equal(2, workerIndex.ServiceClasses.Count());
            ValidatePlayerClass(workerIndex.ServiceClasses.First(), 0);
            ValidateGameClass(workerIndex.ServiceClasses.Last(), 1);
            ValidateFile("player_game.js", workerIndex.FileNames.First());
        }

        [Fact]
        public void CanCreateTwoClassIndexFromTwoFiles()
        {
            //arrange
            var workerFiles = new[]
            {
                LoadWorkerFile("player.js"),
                LoadWorkerFile("game.js"),
            };

            var creator = new JavaScriptParserServiceImpl();

            //act
            var result = creator.ParseWorkerCode(1, 1, "Test", workerFiles, null, null);
            var workerIndex = result.WorkerIndex;

            //assert
            Assert.Equal(2, workerIndex.FileNames.Count());
            Assert.Equal(2, workerIndex.ServiceClasses.Count());
            ValidatePlayerClass(workerIndex.ServiceClasses.First(), 0);
            ValidateFile("player.js", workerIndex.FileNames.First());
            ValidateGameClass(workerIndex.ServiceClasses.Last(), 1);
            ValidateFile("game.js", workerIndex.FileNames.Last());
        }

        [Fact]
        public void CanCreateOneClassIndexFromOneFile()
        {
            //arrange
            var workerFile = LoadWorkerFile("player.js");
            var creator = new JavaScriptParserServiceImpl();

            //act
            var result = creator.ParseWorkerCode(1, 1, "Test", new[] {workerFile}, null, null);
            var workerIndex = result.WorkerIndex;
            
            Assert.Single(workerIndex.ServiceClasses);
            Assert.Single(workerIndex.FileNames);

            //assert
            ValidatePlayerClass(workerIndex.ServiceClasses.First(), 0);
            ValidateFile("player.js", workerIndex.FileNames.First());
        }
    }
}