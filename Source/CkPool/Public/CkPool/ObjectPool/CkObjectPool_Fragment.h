#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include <Templates/SubclassOf.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Bookkeeping fragment on an ObjectPool's registry entity — identifies which class's pool the entity
    // represents. The synchronous pool state (free/in-use arrays, counters) deliberately stays on the
    // GC-rooted subsystem; this entity exists for enumeration and tooling visibility (record membership,
    // EcsDebugger), NOT for the acquire/release hot path
    struct CKPOOL_API FFragment_ObjectPool_PoolInfo
    {
    public:
        CK_GENERATED_BODY(FFragment_ObjectPool_PoolInfo);

    private:
        TSubclassOf<UObject> _ObjectClass;

    public:
        CK_PROPERTY_GET(_ObjectClass);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ObjectPool_PoolInfo, _ObjectClass);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // world-level pool registry: every ObjectPool's registry entity connects here at creation; the record
    // lives on the world's transient entity (transient record — pool bookkeeping never snapshots)
    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfObjectPools, FCk_Handle);
}

// --------------------------------------------------------------------------------------------------------------------
