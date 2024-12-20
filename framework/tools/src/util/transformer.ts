import ts, {SourceFile, SyntaxKind} from "typescript";
import chalk from "chalk";
import path, {ParsedPath} from "path";
import {WORKER_CLIENT_PACKAGE_NAME, WORKER_RUNTIME_PACKAGE_NAME} from "../constants";
import fs, {PathLike} from "fs";
import {createDir} from "./loud-fs";
import {ErrorAlreadyLogged, logLocalError, logVerbose} from "./logging";

const modPath = path.posix;

const TRANSFORMER_ERRORS: Map<string, string[]> = new Map<string, string[]>;

function format(str: string, values: string[]) : string {
    return str.replace(/{([0-9]+)}/g, function (match, index) {
        return typeof values[index] == 'undefined' ? match : values[index];
    });
}

const COMPILER_ERROR_PREFIX = "SWC";

class TransformerError {
    constructor(public code: number, public fmt: string) {
    }
    message(values: string[]) : string {
        const c = this.code.toString();
        return format("[" + chalk.redBright(`${COMPILER_ERROR_PREFIX}${c.padStart(3, '0')}`) + "] " + this.fmt, values);
    }
}

let __code = 1;
const ERR = {
    // Note: don't ever change the ordering of these, add new items to the bottom.
    ServiceGetAccessor: new TransformerError(__code++, 'The get accessor {0}.{1} is invalid. Services cannot have get/set accessors. Use a service method instead.'),
    ServiceSetAccessor: new TransformerError(__code++, 'The set accessor {0}.{1} is invalid. Services cannot have get/set accessors. Use a service method instead.'),
    MethodAsyncModifier: new TransformerError(__code++,'The {0} method {1}.{2} has an async modifier.'),
    MethodNameAsync: new TransformerError(__code++,'The {0} method {1}.{2} contains the word "async".'),
    MethodReturnsPromise: new TransformerError(__code++,'The {0} method {1}.{2} returns a Promise. Promises are not supported in worker code.'),
}

function addTransformerError(node: ts.Node, sourceFile: ts.SourceFile, error: TransformerError, ...values: string[]) {
    const path = sourceFile.fileName;
    const pos = sourceFile.getLineAndCharacterOfPosition(node.getStart());
    const message = `${chalk.gray(`at line ${chalk.yellowBright(`${pos.line + 1}`)}:${pos.character}`)} ${error.message(values)}`;
    if (!TRANSFORMER_ERRORS.has(path))
        TRANSFORMER_ERRORS.set(path, [message]);
    else
        TRANSFORMER_ERRORS.get(path)?.push(message);
}

function hasTransformerErrors() {
    return TRANSFORMER_ERRORS.size > 0;
}

function getTransformerErrorCount() : number {
    let errorCount = 0;
    if(hasTransformerErrors())
    {
        for(const value of TRANSFORMER_ERRORS.values())
            errorCount += value.length;
    }
    return errorCount;
}

function consumeAndPrintTransformerErrors() {
    if (!TRANSFORMER_ERRORS || TRANSFORMER_ERRORS.size == 0)
        return null;
    for (const file of TRANSFORMER_ERRORS.keys()) {
        console.log(chalk.yellowBright(`in ${file}:`));
        for (const error of TRANSFORMER_ERRORS.get(file)!) {
            console.log(` ${error}`);
        }
    }
    TRANSFORMER_ERRORS.clear();
}

function hasAsyncKeyword(node: ts.MethodDeclaration) {
    return node && node.modifiers && node.modifiers.find((m) => m.kind === ts.SyntaxKind.AsyncKeyword);
}

export const StageTransformer = (program: ts.Program) => {
    const transformerFactory: ts.TransformerFactory<ts.SourceFile> = context => {
        return sourceFile => {
            const visitor = (node: ts.Node): ts.Node => {
                // Doctor the path so compilation will work locally
                if (node.kind === ts.SyntaxKind.StringLiteral &&
                    node.parent &&
                    node.parent.kind === ts.SyntaxKind.ImportDeclaration) {
                    const literal = node as ts.StringLiteral;
                    const dir = modPath.dirname(sourceFile.fileName);
                    const updated = getUpdatedPath(dir, literal.text);
                    if (updated) {
                        return context.factory.createStringLiteral(updated);
                    }
                }

                // Services can't have getters
                if (node.kind === ts.SyntaxKind.GetAccessor &&
                    hasBaseClass('Service', node.parent)) {
                    const property = node as ts.GetAccessorDeclaration;
                    const className = (<ts.ClassDeclaration>node.parent).name?.text;
                    const propertyName = (<ts.Identifier>property.name)?.text;
                    addTransformerError(node, sourceFile, ERR.ServiceGetAccessor, className!, propertyName);
                }

                // Services can't have setters
                if (node.kind === ts.SyntaxKind.SetAccessor &&
                    hasBaseClass('Service', node.parent)) {
                    const property = node as ts.GetAccessorDeclaration;
                    const className = (<ts.ClassDeclaration>node.parent).name?.text;
                    const propertyName = (<ts.Identifier>property.name)?.text;
                    addTransformerError(node, sourceFile, ERR.ServiceSetAccessor, className!, propertyName);
                }

                if (node.kind === ts.SyntaxKind.MethodDeclaration) {
                    const method = node as ts.MethodDeclaration;
                    const methodName = (<ts.Identifier>method.name)?.text;
                    const className = (<ts.ClassDeclaration>node.parent).name?.text;

                    let typeInfo = null;
                    if (hasBaseClass("Service", node.parent)) {
                        typeInfo = 'service';
                    } else if (hasBaseClass("Data", node.parent)) {
                        typeInfo = 'data';
                    } else if (hasBaseClass("Message", node.parent)) {
                        typeInfo = 'message';
                    }
                    if (typeInfo) {
                        if (hasAsyncKeyword(method)) {
                            addTransformerError(node, sourceFile, ERR.MethodAsyncModifier, typeInfo,className!,methodName);
                        }

                        // All calls inside worker process are made synchronously, but executed in parallel with optimistic
                        //  concurrency at the db layer. As such, the concept of a Promise doesn't make sense as there's
                        //  no disks/filesystems and no networking, I.e. nothing to make a Promise useful.
                        if(hasPromiseReturnType(method)) {
                            addTransformerError(node, sourceFile, ERR.MethodReturnsPromise, typeInfo,className!,methodName);
                        }

                        // Reasoning:
                        // Service method calls execute synchronously from the call's perspective.
                        // The generated client does, in fact have an Async suffix because from the client's perspective
                        // all calls are async.
                        // Having the word async anywhere in the method name could be very confusing.
                        if (methodName?.toLowerCase().includes('async')) {
                            addTransformerError(node, sourceFile, ERR.MethodNameAsync, typeInfo, className!, methodName);
                        }
                    }
                }

                return ts.visitEachChild(node, visitor, context);
            };

            return ts.visitNode(sourceFile, visitor);
        };
    };

    return transformerFactory;
};

export const ClientDeclTransformer = (program: ts.Program) => {
    const transformerFactory: ts.TransformerFactory<ts.SourceFile> = context => {
        return sourceFile => {
            const visitor = (node: ts.Node): ts.Node => {
                // Change worker-runtime imports to import warp-client instead.
                if (node.kind === ts.SyntaxKind.StringLiteral &&
                    node.parent &&
                    node.parent.kind === ts.SyntaxKind.ImportDeclaration) {
                    const literal = node as ts.StringLiteral;
                    if (literal.text.indexOf(WORKER_RUNTIME_PACKAGE_NAME) > -1) {
                        const updated = literal.text.replace(WORKER_RUNTIME_PACKAGE_NAME, WORKER_CLIENT_PACKAGE_NAME);
                        return context.factory.createStringLiteral(updated);
                    }
                }

                if (node.kind === ts.SyntaxKind.MethodDeclaration &&
                    hasBaseClass("Service", node.parent)) {
                    const method = node as ts.MethodDeclaration;
                    const methodName = (<ts.Identifier>method.name)?.text;
                    const type = method.type;

                    // Wrap the return type in a Promise
                    let adjustedType;

                    if(type?.kind === ts.SyntaxKind.UnionType) {
                        const unionRef = type as ts.UnionTypeNode;
                        adjustedType = context.factory.createTypeReferenceNode('Promise', [
                            context.factory.createUnionTypeNode(unionRef.types)
                        ]);
                    } else {
                        const typeNode = type as ts.TypeNode;
                        if(!typeNode)
                            throw new Error("Unknown type found");
                        adjustedType = context.factory.createTypeReferenceNode('Promise', [
                            typeNode
                        ]);
                    }

                    return context.factory.updateMethodDeclaration(
                        method,
                        method.decorators,
                        method.modifiers,
                        method.asteriskToken,
                        // From the client's perspective, it's an Async call.
                        context.factory.createIdentifier(methodName + 'Async'),
                        method.questionToken,
                        method.typeParameters,
                        method.parameters,
                        adjustedType ?? undefined,
                        method.body);
                }

                // Remove property declarations off Service classes
                if (node.kind === ts.SyntaxKind.PropertyDeclaration &&
                    hasBaseClass("Service", node.parent)) {
                    return <ts.Node><unknown>undefined;
                }

                return ts.visitEachChild(node, visitor, context);
            };

            return ts.visitNode(sourceFile, visitor);
        };
    };

    return transformerFactory;
};

function hasPromiseReturnType(method: ts.MethodDeclaration) {
    if (method && method.kind === ts.SyntaxKind.MethodDeclaration) {
        if(method.type && method.type.kind === ts.SyntaxKind.TypeReference) {
            const type = method.type as ts.TypeReferenceNode;
            const typeName = type.typeName?.getText();
            return typeName === 'Promise';
        }
    }
    return false;
}

function hasBaseClass(baseClassName: string, node: ts.Node) {
    if (node && node.kind === ts.SyntaxKind.ClassDeclaration) {
        const clz = node as ts.ClassDeclaration;
        if (clz.heritageClauses && clz.heritageClauses.length >= 1 &&
            clz.heritageClauses[0].kind === ts.SyntaxKind.HeritageClause) {
            const hc = clz.heritageClauses[0] as ts.HeritageClause;
            if (hc.types && hc.types[0].kind === ts.SyntaxKind.ExpressionWithTypeArguments) {
                const exp = hc.types[0] as ts.ExpressionWithTypeArguments;
                if (exp.expression && exp.expression.kind === ts.SyntaxKind.Identifier) {
                    const ident = exp.expression as ts.Identifier;
                    if (ident.text === baseClassName) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

export const JavaScriptWorkerExecutionTransformer = (program: ts.Program) => {
    const transformerFactory: ts.TransformerFactory<ts.SourceFile> = context => {
        return sourceFile => {
            const visitor = (node: ts.Node): ts.Node => {

                // Remove async modifier from Service methods
                //  (because everything is synchronous inside a worker service method call.)
                if (node.kind === ts.SyntaxKind.AsyncKeyword &&
                    node.parent &&
                    node.parent.kind == ts.SyntaxKind.MethodDeclaration &&
                    hasBaseClass("Service", node.parent.parent)) {
                    return <ts.Node><unknown>undefined;
                }

                // Remove all property declarations
                if (node.kind === ts.SyntaxKind.PropertyDeclaration) {
                    return <ts.Node><unknown>undefined;
                }

                return ts.visitEachChild(node, visitor, context);
            };

            return ts.visitNode(sourceFile, visitor);
        };
    };

    return transformerFactory;
};

class Module {
    public file: ParsedPath;
    public isTypeDef: boolean;
    public isIndex: boolean;
    public path: string;

    constructor(fileName: string, dirName: string) {
        this.file = modPath.parse(modPath.join(dirName, fileName));
        this.isTypeDef = this.file.name.endsWith('.d') && this.file.ext === '.ts';
        this.isIndex = this.file.name === "index" || this.file.name === "index.d";
        if (!this.isIndex) {
            if (this.isTypeDef) {
                const baseName = modPath.basename(this.file.name, '.d');
                this.path = modPath.join(this.file.dir, baseName);
            } else {
                this.path = modPath.join(this.file.dir, this.file.name);
            }
        } else {
            this.path = modPath.join(this.file.dir);
        }
    }
}

let MODULES: Module[];

function getUpdatedPath(fromDir: string, importPath: string): string | null {
    if (path.extname(importPath))
        return null;
    const res = modPath.resolve(fromDir, importPath).toString();
    const i = res.indexOf('/' + WORKER_RUNTIME_PACKAGE_NAME);
    if (i > -1) {
        const x = res.substring(i + WORKER_RUNTIME_PACKAGE_NAME.length + 1);
        if (x) {
            return WORKER_RUNTIME_PACKAGE_NAME + x;
        } else {
            return WORKER_RUNTIME_PACKAGE_NAME;
        }
    }
    for (const module of MODULES) {
        if (module.path === res) {
            if (module.isTypeDef) {
                return importPath;
            } else {
                let rel = modPath.relative(fromDir, module.file.dir);
                if (module.isIndex) {
                    rel = modPath.join(rel, "index.js");
                } else {
                    rel = modPath.join(rel, module.file.name + ".js");
                }
                if (!rel.startsWith('.')) {
                    rel = './' + rel;
                }
                return rel;
            }
        }
    }
    return null;
}

function parseModules(dir: string): Module[] {
    let files = fs.readdirSync(dir, {withFileTypes: true});
    let modules: Module[] = [];
    for (const file of files) {
        if (file.isDirectory()) {
            const those = parseModules(modPath.join(dir, file.name));
            modules.push(...those);
        } else {
            switch (path.extname(file.name)) {
                case ".ts":
                case ".js":
                case ".mjs":
                    modules.push(new Module(file.name, dir));
            }
        }
    }
    return modules;
}

function _findFilesByExtension(baseDir: string,
                               dir: string,
                               extensions: Set<string>,
                               files: string[],
                               badExtensions: Set<string> | null,
                               skipDirs: Set<string> | null,
                               uniqueBaseNames: Set<string> | null): string[] {

    let list = fs.readdirSync(dir);
    list.forEach(function (fileName) {
        const pathName = path.join(dir, fileName);
        let stat = fs.statSync(pathName);
        if (stat && stat.isDirectory()) {
            if (!skipDirs?.has(fileName)) {
                _findFilesByExtension(baseDir, pathName, extensions, files, badExtensions, skipDirs, uniqueBaseNames);
            }
        }
        const ext = path.extname(pathName);
        if (badExtensions && badExtensions.has(ext)) {
            throw new Error(logLocalError('Invalid file type found: ' + pathName));
        } else if (extensions.has(ext)) {
            if (uniqueBaseNames) {
                const p = path.parse(pathName);
                const baseName = path.join(p.dir, p.name);
                if (uniqueBaseNames.has(baseName)) {
                    throw new Error(logLocalError(`Duplicate file found. Cannot have multiple modules with the same base name (name without a file extension). File: ${pathName}`));
                }
                uniqueBaseNames.add(baseName);
            }
            files!.push(pathName);
        }
    });
    return files;
}

function findFilesByExtension(dir: string,
                              extensions: string[],
                              uniqueBaseName: boolean = true,
                              badExtensions: string[] | null,
                              skipDirs: string[] | null) {
    return _findFilesByExtension(
        dir,
        dir,
        new Set<string>(extensions),
        [],
        badExtensions ? new Set<string>(badExtensions) : null,
        skipDirs ? new Set<string>(skipDirs) : null,
        uniqueBaseName ? new Set<string>() : null);
}

export function transformTypeScript(inDir: PathLike,
                                    outDir: PathLike,
                                    compilerOptions: ts.CompilerOptions,
                                    transformer: any,
                                    fileExtensions: string[]): string[] {
    const inDir_ = path.resolve(inDir.toString()).toString();
    const outDir_ = path.resolve(outDir.toString()).toString();

    MODULES = parseModules(inDir_);

    let sourceFileNames = findFilesByExtension(
        inDir_,
        fileExtensions,
        true, // Because that's how the worker runtime works; it doesn't care about extensions.
        null,
        [WORKER_RUNTIME_PACKAGE_NAME]);

    const opts = Object.assign({}, compilerOptions);
    opts.noEmit = true;

    const compilerHost = ts.createCompilerHost(opts, true);
    let program = ts.createProgram(sourceFileNames, opts, compilerHost);
    const sourceFiles: SourceFile[] = [];
    for (const fileName of sourceFileNames) {
        sourceFiles.push(program.getSourceFile(fileName)!);
    }

    const result = ts.transform<ts.SourceFile>(sourceFiles, [transformer(program)], opts);

    if (hasTransformerErrors()) {
        const errorCount = getTransformerErrorCount();
        logLocalError(`${errorCount} error${errorCount > 1 ? "s" : ""} occurred during compilation`);
        consumeAndPrintTransformerErrors();
        throw new ErrorAlreadyLogged();
    }

    createDir(outDir_);

    const printer = ts.createPrinter();
    const outputFiles = [];
    for (const transformedSourceFile of result.transformed) {
        const f = transformedSourceFile.fileName;
        const relTarget = f.substring(inDir_.length);
        const target = path.join(outDir_, relTarget);
        const p = path.dirname(target);
        if (!fs.existsSync(p))
            fs.mkdirSync(p, {recursive: true});

        fs.writeFileSync(target, printer.printFile(transformedSourceFile), {encoding: "utf8"});
        logVerbose(`Transformed ${f}`);
        outputFiles.push(target);
    }
    return outputFiles;
}