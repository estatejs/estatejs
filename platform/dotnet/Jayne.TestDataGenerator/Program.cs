// See https://aka.ms/new-console-template for more information

using System.Text;
using Newtonsoft.Json;
using Estate.Jayne.ApiModels;
using Estate.Jayne.Common;
using Estate.Jayne.Config;
using Estate.Jayne.Protocol.Impl;
using Estate.Jayne.Services.Impl;

class Program
{
    static int Main(string[] args)
    {
        if (args.Length != 2)
        {
            Console.Error.WriteLine("<program> [test_data_dir] [output_data_dir]");
            return 1;
        }

        var testDataDir = args[0];
        var outputDataDir = args[1];

        try
        {
            TestDataGenerator.CreateSerenityTestWorkerIndexes(testDataDir, outputDataDir);
            return 0;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine(ex.ToString());
            return 1;
        }
    }
}

public static class TestDataGenerator
{
    public static void CreateSerenityTestWorkerIndexes(string testDataFolder, string outputFolder)
    {
        if (!Directory.Exists(testDataFolder))
        {
            throw new Exception($"{testDataFolder} does not exist");
        }

        var testDir = "contract_call_service_method_tests";
        CreateWorkerIndex("TestWorker", 1, 1, testDataFolder, outputFolder, testDir, "ServerApi");
        CreateWorkerIndex("TestWorker", 1, 1, testDataFolder, outputFolder, testDir, "Duplicate");
        CreateWorkerIndex("TestWorker", 1, 1, testDataFolder, outputFolder, testDir, "MethodAndSerialization");

        testDir = "contract_delete_worker_tests";
        CreateWorkerIndex("TestWorker", 1, 1, testDataFolder, outputFolder, testDir, "CanDeleteWorker");

        testDir = "contract_get_save_object_tests";
        CreateWorkerIndex("TestWorker", 1, 1, testDataFolder, outputFolder, testDir, "GetAndSave");

        testDir = "contract_setup_worker_tests";
        CreateWorkerIndex("TestWorker", 5002, 1, testDataFolder, outputFolder, testDir, "CanSetupNewWorker");
        CreateWorkerIndex("TestWorker", 6001, 5000, testDataFolder, outputFolder, testDir,
            "CanSetupExistingWorker/5000");
        CreateWorkerIndex("TestWorker", 6001, 5005, testDataFolder, outputFolder, testDir,
            "CanSetupExistingWorker/5005");
    }

    private static void WriteAll(string path, string str)
    {
        WriteAll(path, Encoding.UTF8.GetBytes(str));
    }

    private static void WriteAll(string path, byte[] bytes)
    {
        File.WriteAllBytes(path, bytes);
        Console.WriteLine($"Wrote {bytes.Length} bytes to {path}");
    }

    private static void CreateWorkerIndex(string workerName, ulong workerId, ulong workerVersion, string testDataDir,
        string outputTestDataDir, string testDir, string testName)
    {
        var inputDir = Path.Join(testDataDir, testDir, testName);
        var outputDir = Path.Join(outputTestDataDir, testDir, testName);

        var creator = new JavaScriptParserServiceImpl();

        var inputFiles = LoadWorkerFiles(inputDir, inputDir);

        var result = creator.ParseWorkerCode(workerId, workerVersion, workerName,
            inputFiles, null, null);

        var workerFiles = new List<WorkerFileContent>();
        foreach (var preCompilerDirective in result.PreCompilerDirectives)
            workerFiles.Add(preCompilerDirective.PreCompile());

        var config = new ProtocolSerializerConfig
        {
            InitialBufferSize = 10240
        };

        Directory.CreateDirectory(outputDir);

        //write the binary WorkerIndex
        var serializer = new ProtocolSerializerImpl(config);
        var bytes = serializer.SerializeWorkerIndex(result.WorkerIndex);
        WriteAll(Path.Combine(outputDir, "worker_index"), bytes);
        Console.WriteLine($"Created ");

        //write WorkerIndex in json for reference
        WriteAll(Path.Combine(outputDir, "worker_index.json"),
            JsonConvert.SerializeObject(result.WorkerIndex, Formatting.Indented));

        //write the .js files after pre-compilation
        foreach (var workerFile in workerFiles)
        {
            var extension = Path.GetExtension(workerFile.name);
            if (extension == null)
                throw new Exception("Missing extension");
            var outFile = Path.Combine(outputDir, workerFile.name.Replace(extension, ".out" + extension));
            Directory.CreateDirectory(Path.GetDirectoryName(outFile));
            WriteAll(outFile, workerFile.code);
        }
    }

    private static List<WorkerFileContent> LoadWorkerFiles(string srcRootDir, string dir)
    {
        var files = new List<WorkerFileContent>();
        foreach (var file in Directory.GetFiles(dir, "*.*", SearchOption.TopDirectoryOnly))
        {
            if (file.EndsWith(".out.js") || file.EndsWith(".out.mjs") ||
                !(file.EndsWith(".js") || file.EndsWith(".mjs")))
                continue;

            var relFileName = Path.GetRelativePath(srcRootDir, file);

            files.Add(new WorkerFileContent
            {
                code = File.ReadAllText(file),
                name = relFileName
            });
        }

        foreach (var directory in Directory.GetDirectories(dir, "*", SearchOption.TopDirectoryOnly))
        {
            var thoseFiles = LoadWorkerFiles(srcRootDir, directory);
            if (thoseFiles.Count > 0)
                files.AddRange(thoseFiles);
        }

        return files;
    }
}