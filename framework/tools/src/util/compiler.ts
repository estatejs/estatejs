import ts from "typescript";
import {EOL} from 'os';
import chalk from "chalk";
import fs, {PathLike} from "fs";
import path from "path";

import {WORKER_RUNTIME_PACKAGE_NAME} from "../constants.js";
import {WorkerConfig} from "../model/worker-config.js";
import {getColor, logLocalError, logVerbose} from "./logging.js";
import {createDir, recreateDir} from "./loud-fs.js";
import {writeRuntime} from "./runtime.js";
import {
    ClientDeclTransformer,
    JavaScriptWorkerExecutionTransformer,
    StageTransformer,
    transformTypeScript
} from "./transformer";
import {SingleBar} from "cli-progress";

export interface CompilerOutput {
    // The directory that contains the JS files ready to be uploaded.
    schemaDir: string;
    // The directory that contains the .d.ts files ready to be used for client-side code gen.
    declDir: string;
}

function writePackageJson(target: PathLike, obj: any) {
    fs.writeFileSync(target, JSON.stringify(obj), {encoding: "utf8"});
    logVerbose(`Wrote ${target}`);
}

function writeRuntimeStub(dir: string, decl: boolean) {
    // make a node_modules directory to make TSC happy
    const modulesDirName = 'node_modules';
    const modulesDir = path.join(dir, modulesDirName);
    fs.mkdirSync(modulesDir);

    const targetDir = path.join(modulesDir, WORKER_RUNTIME_PACKAGE_NAME);
    fs.mkdirSync(targetDir);
    writeRuntime(targetDir, decl, true);

    writePackageJson(path.join(targetDir, 'package.json'), {
        "name": WORKER_RUNTIME_PACKAGE_NAME,
        "main": 'index.mjs',
        "type": "module",
        "private": true
    });
}

function writeWorkerPackageJson(dir: string) {
    writePackageJson(path.join(dir, 'package.json'), {
        "name": "worker-package",
        "type": "module",
        "private": true
    });
}

interface CompileTypeScriptResult {
    outDir: string;
    preSchemaDir: string;
    preDeclDir: string;
}

function compileTypeScript(buildDir: string,
                           stageDir: string,
                           fileNames: string[],
                           compilerOptions: ts.CompilerOptions): CompileTypeScriptResult {
    logVerbose(`Compiling TypeScript...`);

    // path: worker/.build/out
    const outDir = path.join(buildDir, 'out');
    createDir(outDir);

    // path: worker/.build/pre-schema
    const preSchemaDir = path.join(buildDir, 'pre-schema');
    createDir(preSchemaDir);

    // path: worker/.build/pre-decl
    const preDeclDir = path.resolve(buildDir, 'pre-decl');
    createDir(preDeclDir);

    compilerOptions.noEmit = false;
    compilerOptions.declaration = true;
    compilerOptions.noEmitOnError = true;
    compilerOptions.declarationDir = preDeclDir;
    compilerOptions.outDir = preSchemaDir;

    let program = ts.createProgram(fileNames, compilerOptions);

    let emitResult = program.emit();

    let allDiagnostics = ts
        .getPreEmitDiagnostics(program)
        .concat(emitResult.diagnostics);

    let errors: string[] = [];
    let warnings: string[] = [];

    allDiagnostics.forEach(diagnostic => {
        let target;
        if (diagnostic.category === ts.DiagnosticCategory.Error)
            target = errors;
        else if (diagnostic.category === ts.DiagnosticCategory.Warning)
            target = warnings;
        else
            return;

        if (diagnostic.file) {
            let {line, character} = ts.getLineAndCharacterOfPosition(diagnostic.file, diagnostic.start!);
            let message = ts.flattenDiagnosticMessageText(diagnostic.messageText, EOL);
            target.push(`${diagnostic.file.fileName} (${line + 1},${character + 1}): ${message}`);
        } else {
            target.push(ts.flattenDiagnosticMessageText(diagnostic.messageText, EOL));
        }
    });

    if (warnings.length > 0) {
        console.log(`${warnings.length} Warning${warnings.length > 1 ? "s" : ""}:`);
        warnings.forEach(warning => {
            console.log(getColor(chalk.yellowBright, warning));
        });
    }
    if (errors.length > 0) {
        if (warnings.length > 0)
            console.log("");
        console.log(`${errors.length} Error${errors.length > 1 ? "s" : ""}:`);
        errors.forEach(error => {
            console.log(getColor(chalk.redBright, error));
        });
        throw new Error("TypeScript compilation failed");
    }

    return {outDir, preSchemaDir, preDeclDir};
}

interface StageResult {
    transformedFiles: string[];
    buildDir: string;
    stageDir: string;
}

function stageForCompilation(buildDir: string, workerDir: string, compilerOptions: ts.CompilerOptions): StageResult {
    // Transforms and parse the TypeScript, so it can be compiled
    logVerbose(`Preprocessing ${workerDir}...`);

    // Transform and parse TypeScript into the .build/obj directory so that it can be compiled.
    const stageDir = path.join(buildDir, 'stage');

    const transformedFiles = transformTypeScript(workerDir, stageDir, compilerOptions, StageTransformer, ['.ts', '.js', '.mjs']);

    writeRuntimeStub(stageDir, true); //needed for compilation
    writeWorkerPackageJson(stageDir); //needed for compilation

    return {transformedFiles, buildDir, stageDir};
}

function transformDeclForClientCodeGen(outDir: string, preDeclDir: string, compilerOptions: ts.CompilerOptions): string {
    const declDir = path.join(outDir, 'decl');
    transformTypeScript(preDeclDir, declDir, compilerOptions, ClientDeclTransformer, ['.ts' /* note: file.d.ts has a .ts extension*/]);
    return declDir;
}

function transformJavaScriptForWorkerExecution(outDir: string, preSchemaDir: string, compilerOptions: ts.CompilerOptions): string {
    const schemaDir = path.join(outDir, 'schema');
    transformTypeScript(preSchemaDir, schemaDir, compilerOptions, JavaScriptWorkerExecutionTransformer, ['.mjs', '.js']);
    return schemaDir;
}

/***
 * Options when compiling worker source code.
 */
export class CompilerOptions {
    constructor(public noImplicitAny: boolean = true,
                public allowJs: boolean = true,
                public strict: boolean = true)
    {}
}

/***
 * Compiles TypeScript/JavaScript worker source code so that it can be run inside a worker and have client code generated
 * to talk to it at runtime.
 * @param progress - progress object
 * @param buildDir - The intermediate build directory.
 * @param workerDir - the worker source code root directory
 * @param options - compiler options
 */
export async function compileAsync(progress: SingleBar | null, buildDir: string, workerDir: string, options: CompilerOptions | null): Promise<CompilerOutput> {
    options = options ?? new CompilerOptions();

    if (!WorkerConfig.fileExists(workerDir)) {
        throw new Error(logLocalError(`${workerDir} is not a worker. You may need to run 'worker init' in that directory.`));
    }

    const compilerOptions = <ts.CompilerOptions>{
        noImplicitAny: options.noImplicitAny,
        target: ts.ScriptTarget.ES2022,
        module: ts.ModuleKind.ES2022,
        moduleResolution: ts.ModuleResolutionKind.NodeJs,
        allowJs: options.allowJs,
        strict: options.strict,
        checkJs: false
    };

    // Transform the TS code, so it can be compiled successfully.
    const {transformedFiles, stageDir} = stageForCompilation(buildDir, workerDir, compilerOptions);
    progress?.increment();

    // Compile the TS into JS and .d.ts files
    const {outDir, preSchemaDir, preDeclDir} = compileTypeScript(buildDir, stageDir, transformedFiles, compilerOptions);
    progress?.increment();

    // Transform the .d.ts files, so they'll work for client-side code gen.
    const declDir = transformDeclForClientCodeGen(outDir, preDeclDir, compilerOptions);
    progress?.increment();

    // Transform the .js files, so they'll work in a worker.
    const schemaDir = transformJavaScriptForWorkerExecution(outDir, preSchemaDir, compilerOptions);
    progress?.increment();

    return {schemaDir, declDir};
}

