using System;
using System.IO;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Hosting;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using Estate.Jayne.Common;
using Estate.Jayne.Services;
using Estate.Jayne.Services.Impl;
using Estate.Jayne.Systems;

namespace Estate.Jayne
{
    public static class Program
    {
        private static async Task OnStartupAsync(IServiceProvider services)
        {
            using var scope = services.CreateScope();
            
            var loggerFactory = services.GetRequiredService<ILoggerFactory>();
            Log.Init(loggerFactory.CreateLogger("system"));
            
            var developerAccountService =  scope.ServiceProvider.GetRequiredService<IDeveloperAccountSystem>();
            await developerAccountService.PopulateAdminKeyCacheAsync();
            await developerAccountService.PopulateUserKeyCacheAsync();

            var platformLimitsService = scope.ServiceProvider.GetRequiredService<IPlatformLimitsService>();
            await platformLimitsService.InitializeAsync();
        }

        private static void PrintEnvironment()
        {
            Console.WriteLine("================================================================================");
            Console.WriteLine("Environment:");
            var vars = Environment.GetEnvironmentVariables();
            foreach (var key in vars.Keys)
                Console.WriteLine($"{key}={vars[key]}");
        }

        private static void PrintFileSystem(string path)
        {
            Console.WriteLine("================================================================================");
            Console.WriteLine($"File System: {path}");
            
            try
            {
                var directories = Directory.GetDirectories(path, "*", SearchOption.TopDirectoryOnly);
                
                foreach (var dir in directories)
                {
                    try
                    {
                        Console.WriteLine($"+ {dir}/");
                        var fileNames = Directory.GetFiles(dir, "*", SearchOption.TopDirectoryOnly);
                        var l = dir.Length + 1;
                        foreach (var fileName in fileNames)
                            Console.WriteLine($" {fileName[l..]}");
                    }
                    catch (Exception e)
                    {
                        Console.WriteLine(" <error>: " + e.Message);
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(" <error>: " + e.Message);
            }
        }
        
        public static async Task Main(string[] args)
        {
            Console.WriteLine($"Estate Jayne {Version.String} starting...");
            
#if DEBUG
            PrintEnvironment();
            PrintFileSystem("/tmp");
            Console.WriteLine("================================================================================");
#endif

            var host = CreateHostBuilder(args).Build();
            await OnStartupAsync(host.Services);
            await host.RunAsync();
        }

        private static IHostBuilder CreateHostBuilder(string[] args) =>
            Host.CreateDefaultBuilder(args)
                .ConfigureWebHostDefaults(webBuilder => { webBuilder.UseStartup<Startup>(); });
    }
}