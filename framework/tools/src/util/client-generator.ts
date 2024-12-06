import path from "path";
import fs from "fs";
import {logLocalError, logRemoteError, logVerbose} from "./logging";
import Mustache from "mustache";
import {Unsigned, UnsignedZero} from "./unsigned";
import {requiresTruthy} from "./requires";
import {WorkerIndexProto} from "../protocol/worker-index-proto";
import {DataClassProto} from "../protocol/data-class-proto";
import {MessageClassProto} from "../protocol/message-class-proto";
import {ServiceClassProto} from "../protocol/service-class-proto";
import {ServiceMethodProto} from "../protocol/service-method-proto";
import {MethodArgumentProto} from "../protocol/method-argument-proto";
import {FreeClassProto} from "../protocol/free-class-proto";
import {FreeFunctionProto} from "../protocol/free-function-proto";
import {BUILD_INFO} from "../config";
import {ConstructorProto} from "../protocol/constructor-proto";
import {MethodProto} from "../protocol/method-proto";
import {MethodKindProto} from "../protocol/method-kind-proto";
import {readTemplate} from "./template";
import JSZip from "jszip";

const {version} = require("../package.json");

const EOL = require('os').EOL;

function getMethodSeparator(): string {
    return EOL + EOL;
}

function getClassSeparator(): string {
    return EOL + EOL;
}

enum CodeGetDefClassKind {
    FreeClass,
    Data,
    Service,
    Message
}

function getClassDef<MT>(logContext: string, kind: CodeGetDefClassKind, source: any, templates: any, mt: new() => MT, methodNameMod: ((name: string, returnType: string) => { name: string, returnType: string }) | null = null) {

    let extendsStr = "";
    let what = ""
    switch (kind) {
        case CodeGetDefClassKind.FreeClass:
            what = "free class";
            break;
        case CodeGetDefClassKind.Data:
            what = "data class";
            extendsStr = " extends Data "
            break;
        case CodeGetDefClassKind.Service:
            what = "service class";
            extendsStr = " extends Service "
            break;
        case CodeGetDefClassKind.Message:
            what = "message class";
            extendsStr = " extends Message "
            break;
    }

    const className = source.className();
    if (!className)
        throw new Error(logRemoteError(logContext, `Unable to read ${what} from the worker index because the name was empty`));

    let ctorStr = '';
    if (source.ctor) {
        const ctor = source.ctor(new ConstructorProto());
        if (ctor) {
            let args = [];
            for (let j = 0; j < ctor.argumentsLength(); ++j) {
                const argument = ctor.arguments(j, new MethodArgumentProto());
                if (!argument)
                    throw new Error(logRemoteError(logContext, `Unable to read ${what} constructor argument from the worker index because the argument was empty`));
                const name = argument.name();
                if (!name)
                    throw new Error(logRemoteError(logContext, `Unable to read ${what} constructor argument name from the worker index because the argument name was empty`));
                const type = argument.type();
                if (!type)
                    throw new Error(logRemoteError(logContext, `Unable to read ${what} constructor argument type from the worker index because the argument type was empty`));
                args.push(`${name}: ${type}`);
            }
            ctorStr = Mustache.render(templates.defClassCtorTemplate, {
                ARGUMENTS: args.length > 0 ? args.join(', ') : ""
            }) + EOL;
        }
    }

    let methods = [];
    for (let j = 0; j < source.methodsLength(); ++j) {
        const method = source.methods(j, new mt());
        if (!method)
            throw new Error(logRemoteError(logContext, `Unable to read ${what} method from the worker index because it was empty`));
        let name = method.methodName();
        if (!name)
            throw new Error(logRemoteError(logContext, `Unable to read ${what} method name from the worker index because it was empty`));
        let returnType = method.returnType();
        if (!returnType)
            throw new Error(logRemoteError(logContext, `Unable to read ${what} method return type from the worker index because it was empty`));

        if (methodNameMod) {
            const adj = methodNameMod(name, returnType);
            name = adj.name;
            console.assert(name);
            returnType = adj.returnType;
            console.assert(returnType);
        }

        let args = [];
        for (let j = 0; j < method.argumentsLength(); ++j) {
            const argument = method.arguments(j, new MethodArgumentProto());
            if (!argument)
                throw new Error(logRemoteError(logContext, `Unable to read ${what} constructor argument from the worker index because the argument was empty`));
            const name = argument.name();
            if (!name)
                throw new Error(logRemoteError(logContext, `Unable to read ${what} constructor argument name from the worker index because the argument name was empty`));
            const type = argument.type();
            if (!type)
                throw new Error(logRemoteError(logContext, `Unable to read ${what} constructor argument type from the worker index because the argument type was empty`));
            args.push(`${name}: ${type}`);
        }
        const methodKind = method.methodKind ? method.methodKind() : MethodKindProto.Normal;
        switch (methodKind) {
            case MethodKindProto.Getter:
                if (args.length > 0)
                    throw new Error(logRemoteError(logContext, `Invalid ${what} method getter. It has arguments.`));
                methods.push(Mustache.render(templates.defClassGetterTemplate, {
                    METHOD_NAME: name,
                    RETURN_TYPE: returnType
                }));
                break;
            case MethodKindProto.Setter:
                if (returnType != "void")
                    throw new Error(logRemoteError(logContext, `Invalid ${what} method setter. It has a return type that's is not void.`));
                methods.push(Mustache.render(templates.defClassSetterTemplate, {
                    METHOD_NAME: name,
                    ARGUMENTS: args.length > 0 ? args.join(', ') : ""
                }))
                break;
            case MethodKindProto.Normal:
                methods.push(Mustache.render(templates.defClassMethodTemplate, {
                    METHOD_NAME: name,
                    ARGUMENTS: args.length > 0 ? args.join(', ') : "",
                    RETURN_TYPE: returnType
                }))
                break;
            default:
                throw new Error(logRemoteError(logContext, `Unable to read ${what} method kind from the worker index because it was empty`));
        }
    }

    return Mustache.render(templates.defClassTemplate, {
        EXTENDS: extendsStr,
        CLASS_NAME: className,
        CONSTRUCTOR: ctorStr,
        METHODS: methods.length > 0 ? methods.join(EOL) : ""
    });
}

class GeneratedClient {
    constructor(public package_json: string, public index_js: string) {
        requiresTruthy('package_json', package_json);
        requiresTruthy('index_js', index_js);
    }
}

function generateClient(logContext: string, isEsm: boolean, userKey: string, workerIndex: WorkerIndexProto, regenCommand: string): GeneratedClient {
    requiresTruthy('logContext', logContext);
    requiresTruthy('userKey', userKey);
    requiresTruthy('workerIndex', workerIndex);
    requiresTruthy('regenCommand', regenCommand);

    const clientExports = [];

    //read in the templates
    const indexTemplate = readTemplate("index.mustache");
    const messageTemplate = readTemplate("message.mustache");
    const dataTemplate = readTemplate("data.mustache");
    const serviceTemplate = readTemplate("service.mustache");
    const serviceMethodTemplate = readTemplate("service-method.mustache");

    const workerName = workerIndex.workerName();
    if (!workerName)
        throw new Error(logRemoteError(logContext, 'Unable to read worker index because the name was empty'));

    const workerId = Unsigned.fromLong(workerIndex.workerId());
    if (workerId.equals(UnsignedZero))
        throw new Error(logRemoteError(logContext, 'Unable to read worker index because the worker id was invalid'));

    const workerVersion = Unsigned.fromLong(workerIndex.workerVersion());
    if (workerVersion.equals(UnsignedZero))
        throw new Error(logRemoteError(logContext, 'Unable to read worker index because the worker version was invalid'));

    let freeFunctions = [];
    for (let i = 0; i < workerIndex.freeFunctionsLength(); ++i) {
        const freeFunction = workerIndex.freeFunctions(i, new FreeFunctionProto());
        if (!freeFunction)
            throw new Error(logRemoteError(logContext, 'Unable to read free function from the worker index because it contained invalid data'));
        const code = freeFunction.sourceCode();
        if (!code)
            throw new Error(logRemoteError(logContext, 'Unable to read free function from the worker index because the source code was empty'))
        freeFunctions.push(code);
        const functionName = freeFunction.functionName();
        if (!functionName)
            throw new Error(logRemoteError(logContext, 'Unable to read free function from the worker index because the function name was empty'));
        clientExports.push(functionName);
    }

    let freeClasses = [];
    for (let i = 0; i < workerIndex.freeClassesLength(); ++i) {
        const freeClass = workerIndex.freeClasses(i, new FreeClassProto());
        if (!freeClass)
            throw new Error(logRemoteError(logContext, 'Unable to read free class from the worker index because it contained invalid data'));
        const code = freeClass.sourceCode();
        if (!code)
            throw new Error(logRemoteError(logContext, 'Unable to read free class from the worker index because the source code was empty'))

        const className = freeClass.className();
        if (!className)
            throw new Error(logRemoteError(logContext, 'Unable to read free class name from the worker index because it contained invalid data'));

        clientExports.push(className);
        freeClasses.push(code);
    }

    let dataClasses = [];
    for (let i = 0; i < workerIndex.dataClassesLength(); ++i) {
        const dataClass = workerIndex.dataClasses(i, new DataClassProto());
        if (!dataClass)
            throw new Error(logRemoteError(logContext, 'Unable to read data class from the worker index because it contained invalid data'));
        const name = dataClass.className();
        if (!name)
            throw new Error(logRemoteError(logContext, 'Unable to read data class from the worker index because the name was empty'));

        clientExports.push(name);

        const classId = dataClass.classId();
        if (classId <= 0)
            throw new Error(logRemoteError(logContext, 'Unable to read data class from the worker index because the class id was invalid'));
        const code = dataClass.sourceCode();
        if (!code)
            throw new Error(logRemoteError(logContext, 'Unable to read data class from the worker index because the source code was empty'));

        dataClasses.push(Mustache.render(dataTemplate, {
            ESM_EXPORT: isEsm ? "export " : "",
            CODE: code,
            CLASS_NAME: name,
            CLASS_ID: classId
        }));
    }

    let messageClasses = [];
    for (let i = 0; i < workerIndex.messageClassesLength(); ++i) {
        const eventClass = workerIndex.messageClasses(i, new MessageClassProto());
        if (!eventClass)
            throw new Error(logRemoteError(logContext, 'Unable to read message class from the worker index because it contained invalid data'));
        const name = eventClass.className();
        if (!name)
            throw new Error(logRemoteError(logContext, 'Unable to read message class from the worker index because the name was empty'));
        const classId = eventClass.classId();
        if (classId <= 0)
            throw new Error(logRemoteError(logContext, 'Unable to read message class from the worker index because the class id was invalid'));
        const code = eventClass.sourceCode();
        if (!code)
            throw new Error(logRemoteError(logContext, 'Unable to read message class from the worker index because the source code was empty'));

        messageClasses.push(Mustache.render(messageTemplate, {
            ESM_EXPORT: isEsm ? "export " : "",
            CODE: code,
            CLASS_NAME: name,
            CLASS_ID: classId
        }));

        clientExports.push(name);
    }

    let serviceClasses = [];
    for (let i = 0; i < workerIndex.serviceClassesLength(); ++i) {
        const serviceClass = workerIndex.serviceClasses(i, new ServiceClassProto());
        if (!serviceClass)
            throw new Error(logRemoteError(logContext, 'Unable to read service class from the worker index because it contained invalid data'));
        const className = serviceClass.className();
        if (!className)
            throw new Error(logRemoteError(logContext, 'Unable to read service class from the worker index because the name was empty'));

        clientExports.push(className);

        const classId = serviceClass.classId();
        if (classId <= 0)
            throw new Error(logRemoteError(logContext, 'Unable to read service class from the worker index because the class id was invalid'));

        const serviceMethods = [];
        for (let j = 0; j < serviceClass.methodsLength(); ++j) {
            const serviceMethod = serviceClass.methods(j, new ServiceMethodProto());
            if (!serviceMethod)
                throw new Error(logRemoteError(logContext, 'Unable to read service method from the worker index because it contained invalid data'));
            const methodName = serviceMethod.methodName();
            if (!methodName)
                throw new Error(logRemoteError(logContext, 'Unable to read service method from the worker index because the name was empty'));
            const methodId = serviceMethod.methodId();
            if (methodId <= 0)
                throw new Error(logRemoteError(logContext, 'Unable to read service method from the worker index because the method id was invalid'));

            const methodArguments = [];
            for (let k = 0; k < serviceMethod.argumentsLength(); ++k) {
                const methodArgument = serviceMethod.arguments(k, new MethodArgumentProto());
                if (!methodArgument)
                    throw new Error(logRemoteError(logContext, 'Unable to read service method argument from the worker index because it contained invalid data'));
                const methodArgumentName = methodArgument.name();
                if (!methodArgumentName)
                    throw new Error(logRemoteError(logContext, 'Unable to read service method argument from the worker index because the name was empty'));
                methodArguments.push(methodArgumentName)
            }

            let methodArgsPassString = "";
            if (methodArguments.length > 0) {
                methodArgsPassString = ", " + methodArguments.join(', ');
            }

            serviceMethods.push(Mustache.render(serviceMethodTemplate, {
                METHOD_NAME: methodName,
                METHOD_ARGS_SIG: methodArguments.join(', '),
                METHOD_ID: methodId,
                METHOD_ARGS_PASS_STRING: methodArgsPassString
            }));
        }

        let methodsString = "";
        if (serviceMethods.length) {
            methodsString = EOL + serviceMethods.join(getMethodSeparator());
        }

        serviceClasses.push(Mustache.render(serviceTemplate, {
            ESM_EXPORT: isEsm ? "export " : "",
            CLASS_NAME: className,
            METHODS: methodsString,
            CLASS_ID: classId
        }));
    }

    let cjsExports = "";
    if (!isEsm) {
        cjsExports = Mustache.render(readTemplate('index-exports-cjs.mustache'), {
            EXPORTS: clientExports.join(', ')
        });
    }

    const index_js = Mustache.render(indexTemplate, {
        "BUILD_INFO": BUILD_INFO,
        ESTATE_TOOLS_VERSION: version,
        IMPORT: isEsm ? readTemplate("index-import-esm.js") : readTemplate("index-import-cjs.js"),
        CJS_EXPORT: cjsExports,
        REGEN_COMMAND: regenCommand,
        WORKER_NAME: workerName,
        USER_KEY: userKey,
        WORKER_ID: workerId.toString(),
        WORKER_VERSION: workerVersion.toString(),
        FREE_FUNCTIONS: freeFunctions.length > 0 ? freeFunctions.join(getClassSeparator()) + getClassSeparator() : "",
        FREE_CLASSES: freeClasses.length > 0 ? freeClasses.join(getClassSeparator()) + getClassSeparator() : "",
        DATA: dataClasses.length > 0 ? dataClasses.join(getClassSeparator()) + getClassSeparator() : "",
        MESSAGES: messageClasses.length > 0 ? messageClasses.join(getClassSeparator()) + getClassSeparator() : "",
        SERVICES: serviceClasses.length > 0 ? serviceClasses.join(getClassSeparator()) : ""
    });

    const package_json = isEsm ?
        readTemplate("package-json-esm.json") :
        readTemplate("package-json-cjs.json");

    return new GeneratedClient(package_json, index_js);
}

function generateRootPackage(packageName: string, workerVersion: Unsigned): string {
    return Mustache.render(readTemplate("root-package-json.mustache"), {
        PACKAGE_NAME: packageName,
        VERSION: `${workerVersion.toString()}.0.0`
    })
}

export async function writeTypeDefinitions(compressedWorkerTypeDefinitionsStr: string, packageDir: string) {
    requiresTruthy('compressedWorkerTypeDefinitionsStr', compressedWorkerTypeDefinitionsStr);
    if(!fs.existsSync(packageDir))
        throw new Error(logLocalError(`Package directory ${packageDir} must already exist`));
    const zip = await JSZip.loadAsync(compressedWorkerTypeDefinitionsStr, {base64: true});

    const files = zip.files;
    const fileNames = Object.keys(files);
    for(const relativePath of fileNames) {
        const archive = files[relativePath];
        if(archive.dir)
            continue; //don't create empty directories

        const fullPath = path.resolve(packageDir, relativePath);
        const dirPath = path.dirname(fullPath);

        if(!fs.existsSync(dirPath)) {
            fs.mkdirSync(dirPath, {recursive: true});
        }

        const content = await archive.async('string');
        fs.writeFileSync(fullPath, content, {encoding: "utf8"});
    }
}

export function createClient(logContext: string, userKey: string, workerIndex: WorkerIndexProto, regenCommand: string, projectDir: string):
    { clientPackageName: string, clientPackageDir: string, wasUpdate: boolean } {

    if (!fs.existsSync(projectDir)) {
        throw new Error(logLocalError("Destination project directory does not exist."));
    }

    // Create the package name and the root package
    const workerName = workerIndex.workerName();
    if (!workerName)
        throw new Error(logLocalError("Worker name was empty"));
    let canonicalWorkerName = workerName.toLowerCase();
    let packageName = ""
    if (!canonicalWorkerName.includes("worker")) {
        packageName = canonicalWorkerName
        if (!canonicalWorkerName.endsWith('-')) {
            packageName += '-';
        }
        packageName += 'worker';
    } else {
        packageName = canonicalWorkerName;
    }

    const workerVersion = Unsigned.fromLong(workerIndex.workerVersion());
    if (workerVersion.equals(UnsignedZero))
        throw new Error(logLocalError(`Invalid worker version`));

    const rootPackage = generateRootPackage(packageName, workerVersion);

    // Generate the CJS client
    const cjsClient = generateClient(logContext, false, userKey, workerIndex, regenCommand);

    // Generate the ESM client
    const esmClient = generateClient(logContext, true, userKey, workerIndex, regenCommand);

    // create necessary directories
    const relRootPath = ".estate/generated-clients/" + canonicalWorkerName;
    const rootDir = path.join(projectDir, relRootPath);
    const cjsDir = path.join(rootDir, "cjs");
    const esmDir = path.join(rootDir, "esm");

    let isUpdate = false;

    // Remove it first so no old code hangs around
    if (fs.existsSync(rootDir)) {
        isUpdate = true;
        fs.rmSync(rootDir, {recursive: true, force: true});
    }

    fs.mkdirSync(rootDir, {recursive: true});
    fs.mkdirSync(cjsDir, {recursive: false});
    fs.mkdirSync(esmDir, {recursive: false});

    //write the files
    fs.writeFileSync(path.join(rootDir, "package.json"), rootPackage, {encoding: "utf8"})
    fs.writeFileSync(path.join(cjsDir, "package.json"), cjsClient.package_json, {encoding: "utf8"});
    fs.writeFileSync(path.join(cjsDir, "index.js"), cjsClient.index_js, {encoding: "utf8"});
    fs.writeFileSync(path.join(esmDir, "package.json"), esmClient.package_json, {encoding: "utf8"});
    fs.writeFileSync(path.join(esmDir, "index.js"), esmClient.index_js, {encoding: "utf8"});

    return {clientPackageName: packageName, clientPackageDir: relRootPath, wasUpdate: isUpdate};
}