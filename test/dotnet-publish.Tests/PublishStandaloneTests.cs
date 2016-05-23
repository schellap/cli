// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System.IO;
using Microsoft.DotNet.Tools.Test.Utilities;
using Xunit;
using Microsoft.DotNet.TestFramework;

namespace Microsoft.DotNet.Tools.Publish.Tests
{
    public class PublishStandaloneTests : TestBase
    {
        [Fact]
        public void StandaloneAppDoesNotHaveRuntimeConfigDevJsonFile()
        {
            var testInstance = TestAssetsManager.CreateTestInstance("PortableTests")
                .WithLockFiles();

            var publishDir = Publish(testInstance);

            publishDir.Should().NotHaveFile("StandaloneApp.runtimeconfig.dev.json");
        }

        [Fact]
        public void StandaloneAppWithResourceDepsRunsCorrectly()
        {
            TestInstance instance =
                TestAssetsManager
                    .CreateTestInstance("TestStandaloneAppWithResourceDeps")
                    .WithLockFiles()
                    .WithBuildArtifacts();

            var testRoot = _getProjectJson(instance.TestRoot, "TestStandaloneAppWithResourceDeps");
            var publishCommand = new PublishCommand(testRoot, output: testRoot);
            publishCommand.Execute().Should().Pass();

            var publishedDir = publishCommand.GetOutputDirectory();
            var extension = publishCommand.GetExecutableExtension();
            var outputExe = "TestStandaloneAppWithResourceDeps" + extension;
            publishedDir.Should().HaveFiles(new[] { "TestStandaloneAppWithResourceDeps.dll", outputExe });

            var command = new TestCommand(Path.Combine(publishedDir.FullName, outputExe));
            command.Execute("").Should().ExitWith(0);
        }
        
        private DirectoryInfo Publish(TestInstance testInstance)
        {
            var publishCommand = new PublishCommand(Path.Combine(testInstance.TestRoot, "StandaloneApp"));
            var publishResult = publishCommand.Execute();

            publishResult.Should().Pass();

            return publishCommand.GetOutputDirectory(portable: false);
        }
    }
}
