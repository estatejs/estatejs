using System;
using FirebaseAdmin;
using Google.Apis.Auth.OAuth2;
using Microsoft.AspNetCore.Authentication.JwtBearer;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Http;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.DependencyInjection.Extensions;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using Microsoft.IdentityModel.Tokens;
using Estate.Jayne.AccountsEntities;
using Estate.Jayne.Config;
using Estate.Jayne.Middlewares;
using Estate.Jayne.Protocol;
using Estate.Jayne.Protocol.Impl;
using Estate.Jayne.Services;
using Estate.Jayne.Services.Impl;
using Estate.Jayne.Systems;
using Estate.Jayne.Systems.Impl;
using Estate.Jayne.Util;

namespace Estate.Jayne
{
    public class Startup
    {
        private const string _accountCorsPolicy = "account-cors-policy";
        public Startup(IConfiguration configuration)
        {
            Configuration = configuration;
        }

        public IConfiguration Configuration { get; }

        private ConfigReader SecretsReader { get; } = ConfigReader.FromFileInEnvironmentVariable("ESTATE_JAYNE_SECRETS_FILE");
        private ConfigReader ConfigReader { get; } = ConfigReader.FromFileInEnvironmentVariable("ESTATE_JAYNE_CONFIG_FILE");

        private void AddFirebase(IServiceCollection services)
        {
            var config = ConfigReader.ReadConfig<FirebaseConfig>();

            services
                .AddAuthentication(JwtBearerDefaults.AuthenticationScheme)
                .AddJwtBearer(options =>
                {
                    options.Authority = $"https://securetoken.google.com/{config.FirebaseProject}";
                    options.TokenValidationParameters = new TokenValidationParameters
                    {
                        ValidateIssuer = true,
                        ValidIssuer = $"https://securetoken.google.com/{config.FirebaseProject}",
                        ValidateAudience = true,
                        ValidAudience = config.FirebaseProject,
                        ValidateLifetime = true
                    };
                });
        }

        private void AddAccountsDatabase(IServiceCollection services)
        {
            var config = ConfigReader.ReadConfig<AccountsContextConfig>();
            var secrets = SecretsReader.ReadConfig<AccountsContextSecrets>();

            var serverVersion = new MySqlServerVersion(config.ServerVersion);
            
            services.AddDbContextPool<AccountsContext>(
                dbContextOptions => dbContextOptions
                    .UseMySql(secrets.ConnectionString, serverVersion)
                    .LogTo(Console.WriteLine, LogLevel.Error)
                    .EnableSensitiveDataLogging()
                    .EnableDetailedErrors()
            );
        }

        private void AddProtocol(IServiceCollection services)
        {
            //in this context, protocol is:
            // Always a transient
            // Used by Systems or Services
            // Not used by Controllers directly.

            services.AddTransient<IProtocolDeserializer<SetupWorkerResponseProto>, SetupWorkerProtocolDeserializerImpl>();
            services.AddTransient<IProtocolDeserializer<DeleteWorkerResponseProto>, DeleteWorkerProtocolDeserializerImpl>();

            services.AddSingleton(ConfigReader.ReadConfig<ProtocolSerializerConfig>());
            services.AddTransient<IProtocolSerializer, ProtocolSerializerImpl>();
        }

        private void AddJayneServices(IServiceCollection services)
        {
            //in this context, a service is:
            // Typically a singleton
            // Used by one or more Systems
            // Not used by Controllers directly

            services.AddSingleton(ConfigReader.ReadConfig<LoggingConfig>());
            
            services.AddSingleton(ConfigReader.ReadConfig<WorkerKeyCacheServiceConfig>());
            services.AddSingleton<IWorkerKeyCacheService, WorkerKeyCacheServiceImpl>();
            services.AddSingleton<JavaScriptParserServiceImpl>();

            services.AddSingleton<IParserFactoryService, ParserFactoryServiceImpl>();
            
            services.AddSingleton<ISerenityService, SerenityServiceImpl>();

            services.AddSingleton(ConfigReader.ReadConfig<PlatformLimitsConfig>());
            services.AddSingleton<IPlatformLimitsService, PlatformLimitsServiceImpl>();
        }

        private void AddJayneSystems(IServiceCollection services)
        {
            //in this context, a system is:
            // Typically transient
            // Used directly by controllers (not anything else)

            services.AddSingleton(SecretsReader.ReadConfig<BuiltInAccountSecrets>());
            services.AddTransient<IDeveloperAccountSystem, DeveloperAccountSystemImpl>();
            services.AddTransient<IWorkerAdminSystem, WorkerAdminSystemImpl>();
        }


        private void AddCors(IServiceCollection services)
        {
            services.AddCors(options =>
            {
                options.AddPolicy(name: _accountCorsPolicy,
                    builder =>
                    {
                        builder.AllowAnyOrigin()
                            .AllowAnyMethod()
                            .AllowAnyHeader();
                    });
            });
        }
        
        // This method gets called by the runtime. Use this method to add services to the container.
        public void ConfigureServices(IServiceCollection services)
        {
            services.TryAddSingleton<IHttpContextAccessor, HttpContextAccessor>();

            AddCors(services);
            
            AddFirebase(services);
            AddAccountsDatabase(services);
            AddProtocol(services);
            AddJayneServices(services);
            AddJayneSystems(services);
            
            services.AddLogging(logBuilder =>
            {
                var cfg = ConfigReader.ReadConfig<LoggingConfig>();
                
                logBuilder.ClearProviders();
                logBuilder.AddConsole();
                
                logBuilder.SetMinimumLevel(cfg.MinLevel.ToMSLogLevel());
                logBuilder.AddFilter("user", cfg.UserLevel.ToMSLogLevel());
                logBuilder.AddFilter("system", cfg.SystemLevel.ToMSLogLevel());
                logBuilder.AddFilter("Microsoft.Hosting.Lifetime", cfg.MicrosoftHostingLifetimeLevel.ToMSLogLevel());
                logBuilder.AddFilter("Microsoft.AspNetCore", cfg.MicrosoftAspNetCoreLevel.ToMSLogLevel());
                logBuilder.AddFilter("Microsoft.EntityFrameworkCore.Database.Command", cfg.MicrosoftEntityFrameworkCoreDatabaseCommandLevel.ToMSLogLevel());
            });
            
            services.AddControllers();
        }

        // This method gets called by the runtime. Use this method to configure the HTTP request pipeline.
        public void Configure(IApplicationBuilder app, IWebHostEnvironment env)
        {
            if (env.IsDevelopment())
            {
                app.UseDeveloperExceptionPage();
            }

            app.UseAuthentication();

            //NOTE: This server doesn't terminate HTTPS, that's for an LB to take care of.

            app.UseRouting();

            app.UseCors(_accountCorsPolicy);

            app.UseAuthorization();
            
            app.UseWhen(context => context.Request.Path.StartsWithSegments("/tools-account") || 
                                   context.Request.Path.StartsWithSegments("/tools-worker-admin"), appBuilder =>
            {
                appBuilder.UseMiddleware<EstateApiVersionMiddleware>(new EstateApiVersionMiddlewareOptions
                {
                    HeaderName = "X-Estate-Tools-Protocol-Version",
                    LogError = env.IsDevelopment(),
                    RequiredValue = Version.ToolsProtocolVersion
                });
            });
            
            app.UseWhen(context => context.Request.Path.StartsWithSegments("/site-account"), appBuilder =>
            {
                appBuilder.UseMiddleware<EstateApiVersionMiddleware>(new EstateApiVersionMiddlewareOptions
                {
                    HeaderName = "X-Estate-Site-Protocol-Version",
                    LogError = env.IsDevelopment(),
                    RequiredValue = Version.SiteProtocolVersion
                });
            });
            
            app.UseMiddleware<LoggerMiddleware>();
            app.UseMiddleware<ExceptionHandlerMiddleware>();

            app.UseEndpoints(endpoints => { endpoints.MapControllers(); });

            FirebaseApp.Create(new AppOptions()
            {
                Credential = GoogleCredential.GetApplicationDefault()
            });
        }
    }
}