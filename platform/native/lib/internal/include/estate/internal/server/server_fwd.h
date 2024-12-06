//
// Originally written by Scott R. Jones.
// Copyright (c) 2021 Warpdrive Technologies, Inc. All rights reserved.
//

#pragma once

#include "estate/runtime/result.h"
#include "estate/internal/pool.h"

#include <memory>

namespace estate {
    namespace data {
        class WorkingSet;
        using WorkingSetS = std::shared_ptr<WorkingSet>;
        class SavedPropertyState;
        class CurrentPropertyState;
        class Property;
        using PropertyS = std::shared_ptr<Property>;
        class ObjectIndex;
        using ObjectIndexS = std::shared_ptr<ObjectIndex>;
        class Object;
        using ObjectS = std::shared_ptr<Object>;
        class ObjectHandle;
        using ObjectHandleS = std::shared_ptr<ObjectHandle>;
        class ObjectReference;
        using ObjectReferenceS = std::shared_ptr<ObjectReference>;
        class PermanentObjectState;
        class CurrentObjectState;
        class SavedValue;
        class CurrentValue;
        class CellState;
        using CellStateS = std::shared_ptr<CellState>;
    }
    namespace storage {
        class IDatabase;
        using IDatabaseS = std::shared_ptr<IDatabase>;
        class ITransaction;
        using ITransactionS = std::shared_ptr<ITransaction>;
        class DatabaseManager;
        using DatabaseManagerS = std::shared_ptr<DatabaseManager>;
    }
    namespace engine {
        class CallContext;
        using CallContextS = std::shared_ptr<CallContext>;
        using CallContextW = std::weak_ptr<CallContext>;
        class IObjectRuntime;
        using IObjectRuntimeS = std::shared_ptr<IObjectRuntime>;
        class ISetupRuntime;
        using ISetupRuntimeS = std::shared_ptr<ISetupRuntime>;
        class EngineError;
        template<typename R>
        using EngineResultCode = ResultCode<R, EngineError>;
        using UnitEngineResultCode = _UnitResultCode<EngineError>;
        namespace javascript {
            struct Engine;
            using EngineU = std::unique_ptr<Engine>;
            using EngineHandle = typename Pool<Engine, EngineError>::Handle;
        }
    }
}
