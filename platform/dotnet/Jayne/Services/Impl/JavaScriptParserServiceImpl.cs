using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using Esprima;
using Esprima.Ast;
using Estate.Jayne.ApiModels;
using Estate.Jayne.Common;
using Estate.Jayne.Errors;
using Estate.Jayne.Exceptions;
using Estate.Jayne.Models;
using Estate.Jayne.Models.PreCompiler;
using Estate.Jayne.Models.Protocol;

namespace Estate.Jayne.Services.Impl
{
    public class JavaScriptParserServiceImpl : IParserService
    {
        private const string JavaScriptAnyType = "any";
        private const string DataClassName = "Data";
        private const string MessageClassName = "Message";
        private const string ServiceClassName = "Service";
        private const string WorkerJsonFileName = "worker.json";

        private const ushort UserMethodIdStart = 100; //everything before is reserved for internal use

        //NOTE: This must match what's in native/lib/internal/include/estate/internal/server/server_decl.h
        private const string InternalEstatePrefix = "__internal_estate";
        private const string KernelModuleName = "worker-runtime";
        private const string KernelModuleNameReplacement = "\"" + KernelModuleName + "\"";

        public WorkerLanguage Language { get; } = WorkerLanguage.JavaScript;

        public ParsedClassMappings? ParseClassMappings(WorkerClassMapping[] classMappings)
        {
            if (classMappings == null || classMappings.Length == 0)
                return null;

            const string errorPrefix = "Invalid class mapping: ";

            var classIds = new HashSet<ushort>();
            var dictionary = new Dictionary<string, ushort>();

            foreach (var cm in classMappings)
            {
                if (cm == null)
                {
                    throw new BadCodeParseException(WorkerJsonFileName,
                        errorPrefix + "One or more entries are empty.");
                }

                if (cm.classId == 0)
                {
                    throw new BadCodeParseException(WorkerJsonFileName, errorPrefix + "The class id 0 is invalid.");
                }

                if (string.IsNullOrWhiteSpace(cm.className))
                {
                    throw new BadCodeParseException(WorkerJsonFileName,
                        errorPrefix + "Empty class name specified.");
                }

                if (Regex.IsMatch(cm.className, "^[^A-Za-z_]") ||
                    Regex.IsMatch(cm.className, "[^A-Za-z0-9_]"))
                {
                    throw new BadCodeParseException(WorkerJsonFileName,
                        errorPrefix + $"The class name '{cm.className}' has invalid characters.");
                }

                if (!classIds.Add(cm.classId))
                    throw new BadCodeParseException(WorkerJsonFileName, $"The class id {cm.classId} is a duplicate.");

                if (!dictionary.TryAdd(cm.className, cm.classId))
                    throw new BadCodeParseException(WorkerJsonFileName,
                        $"The class name {cm.className} is a duplicate.");
            }

            return new ParsedClassMappings(dictionary);
        }

        public WorkerClassMapping[] CreateClassMappings(WorkerIndexInfo workerIndex)
        {
            Requires.NotDefault(nameof(workerIndex), workerIndex);

            var mappings = new List<WorkerClassMapping>();

            mappings.AddRange(workerIndex.ServiceClasses.Select(
                i => new WorkerClassMapping {classId = i.ClassId, className = i.ClassName}));
            mappings.AddRange(workerIndex.DataClasses.Select(
                i => new WorkerClassMapping {classId = i.ClassId, className = i.ClassName}));
            mappings.AddRange(workerIndex.MessageClasses.Select(
                i => new WorkerClassMapping {classId = i.ClassId, className = i.ClassName}));

            return mappings.OrderBy(mapping => mapping.classId).ToArray();
        }

        enum ManagedClassType
        {
            Service,
            Data,
            Message
        }

        private (ConstructorInfo?, IEnumerable<MethodInfo>) ParseClassMetadata(WorkerFileContent workerFile,
            ClassDeclaration classDeclaration)
        {
            var className = classDeclaration.Id.Name;
            var methods = new List<MethodInfo>();
            var methodIndex = new Dictionary<string, HashSet<MethodKindInfo>>();
            ConstructorInfo? ctor = default;

            foreach (var classDeclarationChildNode in classDeclaration.ChildNodes)
            {
                if (classDeclarationChildNode.Type != Nodes.ClassBody)
                    continue;

                var classBody = classDeclarationChildNode.As<ClassBody>();

                foreach (var classBodyChild in classBody.ChildNodes)
                {
                    if (classBodyChild.Type != Nodes.MethodDefinition)
                        continue;

                    var methodDef = classBodyChild.As<MethodDefinition>();

                    if (methodDef.Kind == PropertyKind.Constructor)
                    {
                        if (ctor.HasValue)
                        {
                            //NOTE: I can't get this to throw because there's a bug in esprima where
                            //this ends up throwing an ArgumentOutOfRangeException
                            throw new BadCodeParseException(workerFile.name, $"Extra constructor found in {className}.");
                        }

                        using var children = methodDef.ChildNodes.GetEnumerator();
                        if (children.MoveNext() && children.Current != null)
                        {
                            //should be the identifier and should be the constructor
                            if (children.Current.Type == Nodes.Identifier &&
                                children.Current.As<Identifier>().Name == "constructor")
                            {
                                var ctorArgs = new List<MethodArgumentInfo>();
                                var ctorExpression = methodDef.Value.As<FunctionExpression>();
                                //parse the method arguments
                                foreach (var param in ctorExpression.Params)
                                {
                                    var paramIdent = param.As<Identifier>();
                                    ctorArgs.Add(new MethodArgumentInfo(paramIdent.Name, JavaScriptAnyType));
                                }

                                ctor = new ConstructorInfo(ctorArgs);
                            }
                        }

                        continue;
                    }

                    var methodKind = MethodKindInfo.Normal;
                    if (methodDef.Kind.HasFlag(PropertyKind.Get))
                    {
                        methodKind = MethodKindInfo.Getter;
                    }
                    else if (methodDef.Kind.HasFlag(PropertyKind.Set))
                    {
                        methodKind = MethodKindInfo.Setter;
                    }

                    var ident = methodDef.Key.As<Identifier>();
                    var methodName = ident.Name;

                    bool duplicate = false;
                    if (methodIndex.ContainsKey(methodName))
                    {
                        if (methodIndex[methodName].Contains(methodKind) ||
                            ((methodKind == MethodKindInfo.Getter ||
                              methodKind == MethodKindInfo.Setter) &&
                             methodIndex[methodName].Contains(MethodKindInfo.Normal)))
                        {
                            duplicate = true;
                        }
                        else
                        {
                            methodIndex[methodName].Add(methodKind);
                        }
                    }
                    else
                    {
                        methodIndex[methodName] = new HashSet<MethodKindInfo>(new[] {methodKind});
                    }

                    if (duplicate)
                    {
                        throw new BadCodeParseException(workerFile.name,
                            $"The class {className} contains a duplicate method named {methodName}");
                    }

                    var arguments = new List<MethodArgumentInfo>();
                    var expression = methodDef.Value.As<FunctionExpression>();
                    //parse the method arguments
                    foreach (var param in expression.Params)
                    {
                        var paramIdent = param.As<Identifier>();
                        arguments.Add(new MethodArgumentInfo(paramIdent.Name, JavaScriptAnyType));
                    }

                    var returnType = methodKind == MethodKindInfo.Setter ? "void" : JavaScriptAnyType;
                    methods.Add(new MethodInfo(methodName, returnType, arguments, methodKind));
                }
            }

            return (ctor, methods);
        }

        public ScriptParserResult ParseWorkerCode(ulong workerId, ulong version, string workerName,
            IEnumerable<WorkerFileContent> workerFiles, ParsedClassMappings? classMappings, ushort? lastClassId)
        {
            Requires.NotNullOrWhitespace(nameof(workerName), workerName);
            Requires.NotDefaultAndAtLeastOne(nameof(workerFiles), workerFiles);

            var directives = new List<PreCompilerDirective>();

            ushort classId = (ushort) (lastClassId + 1 ?? 1);
            var existingClassMappings = classMappings.HasValue ? classMappings.Value.Mappings : new Dictionary<string, ushort>();

            ushort getClassId(string className)
            {
                if (existingClassMappings.ContainsKey(className))
                    return existingClassMappings[className];
                return classId++;
            }

            ushort fileNameId = 1;
            var allClassNames = new HashSet<string>();
            var managedClassNames = new HashSet<string>();
            var functionNames = new HashSet<string>();

            var files = new List<WorkerFileNameInfo>();
            var freeFunctions = new List<FreeFunctionInfo>();
            var freeClasses = new List<FreeClassInfo>();
            var serviceClasses = new List<ServiceClassInfo>();
            var objectClasses = new List<DataClassInfo>();
            var eventClasses = new List<MessageClassInfo>();
            var filePaths = new HashSet<string>();

            foreach (var workerFile in workerFiles)
            {
                if (!filePaths.Add(Path.ChangeExtension(workerFile.name, null)))
                    throw new BadCodeParseException(workerFile.name, "Duplicate file name");

                //note: this can easily be circumvented by concatenating the string and using obj[str] references.
                if (workerFile.code.Contains(InternalEstatePrefix))
                    throw new BadCodeParseException(workerFile.name, $"Found reference to Estate internal logic");

                Esprima.Ast.Program program;

                try
                {
                    var parser = new JavaScriptParser(workerFile.code, new ParserOptions {Range = true});
                    program = parser.ParseProgram();
                }
                catch (ParserException pe)
                {
                    throw new BadCodeParseException(workerFile.name, pe.Message);
                }
                catch (Exception ex)
                {
                    Log.Error(ex, "Failed to compile");
                    throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.UnknownParserError);
                }

                var instructions = new List<IPreCompilerInstruction>();
                var exportedClasses = new HashSet<string>();
                var exportedItems = new HashSet<string>();
                var classDeclarations = new Dictionary<string, ClassDeclaration>();

                foreach (var statement in program.Body)
                {
                    bool classIsExported = false;
                    ClassDeclaration classDeclaration = default;
                    FunctionDeclaration functionDeclaration = default;

                    switch (statement.Type)
                    {
                        case Nodes.ImportDeclaration:
                        {
                            var import = statement.As<ImportDeclaration>();
                            if (!string.IsNullOrWhiteSpace(import.Source.Raw) &&
                                import.Source.Raw.Contains(KernelModuleName))
                            {
                                var range = import.Source.Range;
                                instructions.Add(new ReplacePreCompilerInstruction(range.Start, range.End,
                                    KernelModuleNameReplacement));
                            }

                            continue;
                        }
                        case Nodes.ExportNamedDeclaration:
                        {
                            var export = statement.As<ExportNamedDeclaration>();
                            if (export.Declaration != null)
                            {
                                if (export.Declaration.Type == Nodes.ClassDeclaration)
                                {
                                    classDeclaration = export.Declaration.As<ClassDeclaration>();
                                    classIsExported = true;
                                    break;
                                }

                                if (export.Declaration.Type == Nodes.FunctionDeclaration)
                                {
                                    functionDeclaration = export.Declaration.As<FunctionDeclaration>();
                                    break;
                                }
                            }
                            else
                            {
                                foreach (var specifier in export.Specifiers)
                                {
                                    if (specifier.Exported != null)
                                    {
                                        exportedItems.Add(specifier.Exported.Name);
                                    }
                                }
                            }

                            continue; //looks like nothing at all to me
                        }
                        case Nodes.ClassDeclaration:
                        {
                            classDeclaration = statement.As<ClassDeclaration>();
                            break;
                        }
                        case Nodes.FunctionDeclaration:
                        {
                            functionDeclaration = statement.As<FunctionDeclaration>();
                            break;
                        }
                        default:
                            continue;
                    }

                    if (classDeclaration != default)
                    {
                        var className = classDeclaration.Id.Name;

                        if (!allClassNames.Add(className))
                            throw new BadCodeParseException(workerFile.name, $"Duplicate class name: " + className);

                        var superIdent = classDeclaration.SuperClass.As<Identifier>();

                        ManagedClassType? managedClassType = superIdent?.Name switch
                        {
                            ServiceClassName => ManagedClassType.Service,
                            DataClassName => ManagedClassType.Data,
                            MessageClassName => ManagedClassType.Message,
                            _ => default
                        };

                        if (managedClassType == ManagedClassType.Service)
                        {
                            ushort methodId = UserMethodIdStart;
                            var methods = new List<ServiceMethodInfo>();

                            bool foundCtor = false;

                            var badCtorMsg =
                                $"Service derived class {className} must have a constructor taking exactly one argument which is then passed to super(). Example: constructor(x) {{ super(x); }}";

                            foreach (var classDeclarationChildNode in classDeclaration.ChildNodes)
                            {
                                if (classDeclarationChildNode.Type != Nodes.ClassBody)
                                    continue;

                                var classBody = classDeclarationChildNode.As<ClassBody>();

                                foreach (var classBodyChild in classBody.ChildNodes)
                                {
                                    if (classBodyChild.Type != Nodes.MethodDefinition)
                                        continue;

                                    var methodDef = classBodyChild.As<MethodDefinition>();

                                    if (methodDef.Static || methodDef.Computed)
                                        continue;

                                    if (methodDef.Kind == PropertyKind.Constructor)
                                    {
                                        if (foundCtor)
                                        {
                                            //NOTE: I can't get this to throw because there's a bug in esprima where
                                            //this ends up throwing an ArgumentOutOfRangeException
                                            throw new BadCodeParseException(workerFile.name,
                                                $"Extra constructor found in {className}.");
                                        }

                                        using var children = methodDef.ChildNodes.GetEnumerator();
                                        if (children.MoveNext() && children.Current != null)
                                        {
                                            //should be the identifier and should be the constructor
                                            if (children.Current.Type == Nodes.Identifier &&
                                                children.Current.As<Identifier>().Name == "constructor")
                                            {
                                                //get the function expression inside the ctor
                                                if (children.MoveNext() &&
                                                    children.Current != null &&
                                                    children.Current.Type == Nodes.FunctionExpression)
                                                {
                                                    var ctorFun = children.Current.As<FunctionExpression>();
                                                    if (ctorFun.Params.Count != 1)
                                                        throw new BadCodeParseException(workerFile.name,
                                                            "Missing constructor parameter. " + badCtorMsg);

                                                    if (ctorFun.Params.Single() is Identifier ctorParam)
                                                    {
                                                        var paramName = ctorParam.Name;
                                                        foreach (var ctorStatementNode in ctorFun.Body.ChildNodes)
                                                        {
                                                            if (ctorStatementNode is ExpressionStatement e &&
                                                                e.Expression is CallExpression call &&
                                                                call.Callee is Super)
                                                            {
                                                                if (call.Arguments.Count == 1)
                                                                {
                                                                    if (call.Arguments[0] is Identifier callArgument &&
                                                                        callArgument.Name == paramName)
                                                                    {
                                                                        foundCtor = true;
                                                                    }
                                                                    else
                                                                    {
                                                                        throw new BadCodeParseException(workerFile.name,
                                                                            "Constructor parameter not passed to super. " +
                                                                            badCtorMsg);
                                                                    }
                                                                }
                                                                else
                                                                {
                                                                    throw new BadCodeParseException(workerFile.name,
                                                                        "Invalid arguments passed to super. Must only pass the constructor parameter. " +
                                                                        badCtorMsg);
                                                                }
                                                            }
                                                        }
                                                    }
                                                    else
                                                    {
                                                        throw new BadCodeParseException(workerFile.name,
                                                            "Missing constructor parameter. " + badCtorMsg);
                                                    }
                                                }
                                            }
                                        }

                                        continue;
                                    }

                                    var ident = methodDef.Key.As<Identifier>();

                                    if (methodDef.Kind.HasFlag(PropertyKind.Get) ||
                                        methodDef.Kind.HasFlag(PropertyKind.Set))
                                    {
                                        throw new BadCodeParseException(workerFile.name,
                                            $"{className} method {ident.Name} is invalid: getters/setters aren't allowed on Service classes.");
                                    }

                                    var arguments = new List<MethodArgumentInfo>();
                                    var expression = methodDef.Value.As<FunctionExpression>();
                                    //parse the method arguments
                                    foreach (var param in expression.Params)
                                    {
                                        var paramIdent = param.As<Identifier>();
                                        arguments.Add(
                                            new MethodArgumentInfo(paramIdent.Name, JavaScriptAnyType));
                                    }

                                    methods.Add(new ServiceMethodInfo(ident.Name, methodId++, JavaScriptAnyType,
                                        arguments));
                                }
                            }

                            if (!foundCtor)
                                throw new BadCodeParseException(workerFile.name, "Missing constructor. " + badCtorMsg);

                            if (!methods.Any())
                                throw new BadCodeParseException(workerFile.name,
                                    $"Service Class {className} must contain at least one method.");

                            serviceClasses.Add(new ServiceClassInfo(className, getClassId(className), fileNameId, methods));
                        }
                        else
                        {
                            var sourceCode = workerFile.code.Substring(classDeclaration.Range.Start,
                                classDeclaration.Range.End - classDeclaration.Range.Start);

                            var (ctor, methods) = ParseClassMetadata(workerFile, classDeclaration);

                            switch (managedClassType)
                            {
                                case ManagedClassType.Data:
                                    if (!ctor.HasValue)
                                        throw new BadCodeParseException(workerFile.name, $"Data class {className} must contain a constructor containing single call to super passing the primary key.");
                                    objectClasses.Add(new DataClassInfo(className, getClassId(className), fileNameId, sourceCode, methods, ctor.Value));
                                    break;
                                case ManagedClassType.Message:
                                    eventClasses.Add(new MessageClassInfo(className, getClassId(className), fileNameId, sourceCode, ctor, methods));
                                    break;
                                default:
                                    freeClasses.Add(new FreeClassInfo(sourceCode, ctor, className, methods));
                                    break;
                            }
                        }

                        if (managedClassType.HasValue)
                        {
                            managedClassNames.Add(className);
                            classDeclarations[className] = classDeclaration;
                        }

                        if (classIsExported)
                            exportedClasses.Add(className);
                    }
                    else if (functionDeclaration != default)
                    {
                        var name = functionDeclaration.Id.Name;
                        if (!functionNames.Add(name))
                            throw new BadCodeParseException(workerFile.name, $"Duplicate function name: " + name);

                        var arguments = new List<MethodArgumentInfo>();
                        foreach (var param in functionDeclaration.Params)
                        {
                            var paramIdent = param.As<Identifier>();
                            arguments.Add(new MethodArgumentInfo(paramIdent.Name, JavaScriptAnyType));
                        }

                        var sourceCode = workerFile.code.Substring(functionDeclaration.Range.Start,
                            functionDeclaration.Range.End - functionDeclaration.Range.Start);

                        freeFunctions.Add(new FreeFunctionInfo(sourceCode, JavaScriptAnyType, name, arguments));
                    }
                }

                foreach (var item in exportedItems)
                {
                    if (managedClassNames.Contains(item))
                        exportedClasses.Add(item);
                }

                //all worker classes need to be exported so the server-factory.mjs module can access them
                foreach (var (className, classDeclaration) in classDeclarations)
                {
                    if (!exportedClasses.Contains(className))
                        instructions.Add(new InsertPreCompilerInstruction(classDeclaration.Range.Start, "export "));
                }

                files.Add(new WorkerFileNameInfo(fileNameId++, workerFile.name));
                directives.Add(new PreCompilerDirective(workerFile, instructions));
            }

            if (classMappings.HasValue)
            {
                var existingClassNames = new HashSet<string>(existingClassMappings.Keys);
                existingClassNames.ExceptWith(managedClassNames);
                if (existingClassNames.Count > 0)
                {
                    throw new BadCodeParseException(WorkerJsonFileName,
                        $"The following classes were found in the class mapping but were not found in code: {string.Join(", ", existingClassNames)}. If you've renamed or deleted these classes, you must update the class mapping.");
                }
            }

            return new ScriptParserResult(directives, new WorkerIndexInfo(workerId, version, workerName, Language, files,
                freeFunctions, freeClasses, serviceClasses, objectClasses, eventClasses));
        }
    }
}