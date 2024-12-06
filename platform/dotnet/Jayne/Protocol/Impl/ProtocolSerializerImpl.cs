using System;
using System.Collections.Generic;
using System.Linq;
using FlatBuffers;
using Microsoft.AspNetCore.Mvc.TagHelpers;
using Estate.Jayne.Common;
using Estate.Jayne.Config;
using Estate.Jayne.Models;
using Estate.Jayne.Models.Protocol;

namespace Estate.Jayne.Protocol.Impl
{
    public class ProtocolSerializerImpl : IProtocolSerializer
    {
        private readonly FlatBufferBuilder _builder;

        public ProtocolSerializerImpl(ProtocolSerializerConfig config)
        {
            _builder = new FlatBufferBuilder(config.InitialBufferSize);
        }

        private MethodKindProto GetMethodKindProto(MethodKindInfo methodKindInfo)
        {
            switch (methodKindInfo)
            {
                case MethodKindInfo.Getter:
                    return MethodKindProto.Getter;
                case MethodKindInfo.Setter:
                    return MethodKindProto.Setter;
                case MethodKindInfo.Normal:
                    return MethodKindProto.Normal;
                default:
                    throw new ArgumentOutOfRangeException(nameof(methodKindInfo), methodKindInfo, null);
            }
        }

        private (StringOffset, Offset<ConstructorProto>, Offset<MethodProto>[]) GetClassMetadata(IClassMetadata clazz)
        {
            Offset<ConstructorProto> ctor = default;
            
            if (clazz.Ctor.HasValue)
            {
                var ctorArguments = clazz.Ctor.Value.Arguments.Select(a =>
                    MethodArgumentProto.CreateMethodArgumentProto(_builder, _builder.CreateString(a.Name),
                        _builder.CreateString(a.Type))).ToArray();
             
                ctor = ConstructorProto.CreateConstructorProto(_builder,
                    ConstructorProto.CreateArgumentsVector(_builder, ctorArguments));
            }

            var methods = new List<Offset<MethodProto>>();
            foreach (var method in clazz.Methods)
            {
                var arguments = method.Arguments.Select(a =>
                    MethodArgumentProto.CreateMethodArgumentProto(_builder,
                        _builder.CreateString(a.Name),
                        _builder.CreateString(a.Type))).ToArray();

                methods.Add(MethodProto.CreateMethodProto(_builder,
                    _builder.CreateString(method.MethodName),
                    GetMethodKindProto(method.MethodKind),
                    arguments.Length > 0 ? MethodProto.CreateArgumentsVector(_builder, arguments) : default,
                    _builder.CreateString(method.ReturnType)));
            }

            return (_builder.CreateString(clazz.ClassName), ctor, methods.Count > 0 ? methods.ToArray() : default);
        }

        public byte[] SerializeWorkerIndex(WorkerIndexInfo workerIndex)
        {
            Requires.NotDefault(nameof(workerIndex), workerIndex);

            _builder.Clear();

            var workerFileNameOffsets = new List<Offset<WorkerFileNameProto>>();
            foreach (var workerFileName in workerIndex.FileNames)
            {
                workerFileNameOffsets.Add(WorkerFileNameProto.CreateWorkerFileNameProto(_builder,
                    workerFileName.FileNameId, _builder.CreateString(workerFileName.FileName)));
            }

            var freeFunctionOffsets = new List<Offset<FreeFunctionProto>>();
            foreach (var ff in workerIndex.FreeFunctions)
            {
                var arguments = ff.Arguments.Select(ffa =>
                        MethodArgumentProto.CreateMethodArgumentProto(_builder,
                            _builder.CreateString(ffa.Name),
                            _builder.CreateString(ffa.Type)))
                    .ToArray();

                freeFunctionOffsets.Add(
                    FreeFunctionProto.CreateFreeFunctionProto(_builder,
                        _builder.CreateString(ff.SourceCode),
                        _builder.CreateString(ff.FunctionName),
                        arguments.Length > 0 
                            ? FreeFunctionProto.CreateArgumentsVector(_builder, arguments) 
                            : default,
                        _builder.CreateString(ff.ReturnType)));
            }

            var freeClassOffsets = new List<Offset<FreeClassProto>>();
            foreach (var fc in workerIndex.FreeClasses)
            {
                var (className, ctor, methods) = GetClassMetadata(fc);

                freeClassOffsets.Add(
                    FreeClassProto.CreateFreeClassProto(_builder,
                        ctor,
                        _builder.CreateString(fc.SourceCode),
                        className,
                        methods != default 
                            ? FreeClassProto.CreateMethodsVector(_builder, methods) 
                            : default));
            }

            var workerServiceClassOffsets = new List<Offset<ServiceClassProto>>();
            foreach (var wsc in workerIndex.ServiceClasses)
            {
                var workerServiceMethodOffsets = new List<Offset<ServiceMethodProto>>();
                foreach (var wm in wsc.Methods)
                {
                    VectorOffset mArgs = default;
                    if (wm.Arguments.Any())
                    {
                        var argOffsets = new List<Offset<MethodArgumentProto>>();
                        foreach (var wma in wm.Arguments)
                        {
                            argOffsets.Add(MethodArgumentProto.CreateMethodArgumentProto(_builder,
                                _builder.CreateString(wma.Name),
                                _builder.CreateString(wma.Type)));
                        }

                        mArgs = ServiceMethodProto.CreateArgumentsVector(_builder, argOffsets.ToArray());
                    }

                    workerServiceMethodOffsets.Add(
                        ServiceMethodProto.CreateServiceMethodProto(_builder,
                            wm.MethodId,
                            _builder.CreateString(wm.MethodName), mArgs,
                            _builder.CreateString(wm.ReturnType)));
                }

                workerServiceClassOffsets.Add(
                    ServiceClassProto.CreateServiceClassProto(
                        _builder,
                        wsc.ClassId,
                        _builder.CreateString(wsc.ClassName),
                        wsc.FileNameId,
                        workerServiceMethodOffsets.Count > 0
                            ? ServiceClassProto.CreateMethodsVector(_builder, workerServiceMethodOffsets.ToArray())
                            : default));
            }

            var dataClassOffsets = new List<Offset<DataClassProto>>();
            foreach (var clazz in workerIndex.DataClasses)
            {
                var (className, ctor, methods) = GetClassMetadata(clazz);

                dataClassOffsets.Add(
                    DataClassProto.CreateDataClassProto(
                        _builder,
                        clazz.ClassId,
                        className,
                        _builder.CreateString(clazz.SourceCode),
                        clazz.FileNameId,
                        ctor,
                        methods != default
                            ? DataClassProto.CreateMethodsVector(_builder, methods)
                            : default));
            }

            var messageClassOffsets = new List<Offset<MessageClassProto>>();
            foreach (var clazz in workerIndex.MessageClasses)
            {
                var (className, ctor, methods) = GetClassMetadata(clazz);

                messageClassOffsets.Add(
                    MessageClassProto.CreateMessageClassProto(
                        _builder,
                        clazz.ClassId,
                        className,
                        _builder.CreateString(clazz.SourceCode),
                        clazz.FileNameId,
                        ctor,
                        methods != default
                            ? MessageClassProto.CreateMethodsVector(_builder, methods)
                            : default));
            }

            //TODO: as of 7/3/21 is_debug=true pushes console.log/error from the worker on Serenity to the client.
            var workerIndexProto = WorkerIndexProto.CreateWorkerIndexProto(_builder,
                true,
                workerIndex.WorkerId,
                workerIndex.WorkerVersion,
                _builder.CreateString(workerIndex.WorkerName),
                workerFileNameOffsets.Count > 0
                    ? WorkerIndexProto.CreateFileNamesVector(_builder, workerFileNameOffsets.ToArray())
                    : default,
                (sbyte) workerIndex.WorkerLanguage,
                freeFunctionOffsets.Count > 0
                    ? WorkerIndexProto.CreateFreeFunctionsVector(_builder, freeFunctionOffsets.ToArray())
                    : default,
                freeClassOffsets.Count > 0
                    ? WorkerIndexProto.CreateFreeClassesVector(_builder, freeClassOffsets.ToArray())
                    : default,
                workerServiceClassOffsets.Count > 0
                    ? WorkerIndexProto.CreateServiceClassesVector(_builder, workerServiceClassOffsets.ToArray())
                    : default,
                dataClassOffsets.Count > 0
                    ? WorkerIndexProto.CreateDataClassesVector(_builder, dataClassOffsets.ToArray())
                    : default,
                messageClassOffsets.Count > 0
                    ? WorkerIndexProto.CreateMessageClassesVector(_builder, messageClassOffsets.ToArray())
                    : default);

            WorkerIndexProto.FinishWorkerIndexProtoBuffer(_builder, workerIndexProto);
            return _builder.SizedByteArray();
        }

        public byte[] SerializeSetupWorkerRequest(string logContext, ulong workerId, ulong workerVersion,
            ulong? previousWorkerVersion, byte[] workerIndex, string[] code)
        {
            _builder.Clear();

            var codeAr = new StringOffset[code.Length];
            for (int i = 0; i < code.Length; i++)
                codeAr[i] = _builder.CreateString(code[i]);

            var codeVec = SetupWorkerRequestProto.CreateWorkerCodeVector(_builder, codeAr);
            var workerIndexVec = SetupWorkerRequestProto.CreateWorkerIndexVector(_builder, workerIndex);

            var off = SetupWorkerRequestProto.CreateSetupWorkerRequestProto(_builder,
                _builder.CreateString(logContext), workerId,
                workerVersion,
                previousWorkerVersion ?? 0,
                workerIndexVec,
                codeVec);

            _builder.Finish(off.Value);

            return _builder.SizedByteArray();
        }

        public byte[] SerializeDeleteWorkerRequest(string logContext, ulong workerId, ulong workerVersion)
        {
            _builder.Clear();

            var off = DeleteWorkerRequestProto.CreateDeleteWorkerRequestProto(_builder,
                _builder.CreateString(logContext), workerId, workerVersion);

            _builder.Finish(off.Value);

            return _builder.SizedByteArray();
        }
    }
}