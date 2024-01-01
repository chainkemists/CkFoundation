#pragma once

#include <Stats/Stats.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Expanded from DECLARE_CYCLE_STAT
    template <typename T_StatName>
    struct TStat_Id
    {
    public:
        using ValueType = T_StatName;

    public:
        static auto GetStatName() -> const char*;
        static auto GetDescription() -> const TCHAR*;
        static auto GetStatType() -> EStatDataType::Type;

        static auto IsClearEveryFrame() -> bool;
        static auto IsCycleStat() -> bool;
        static auto GetFlags() -> EStatFlags;
        static auto GetMemoryRegion() -> FPlatformMemory::EMemoryCounterRegion;
    };
}

#include "CkStats.inl.h"

// --------------------------------------------------------------------------------------------------------------------

#define CK_DEFINE_STAT(_CounterName_, Type, GroupId)      \
struct FStat_##_CounterName_ : public ck::TStat_Id<Type>  \
{                                                       \
    using TGroup = GroupId;                             \
};                                                      \
static inline DEFINE_STAT(_CounterName_)

// --------------------------------------------------------------------------------------------------------------------

#define CK_STAT(_CounterName_)\
    STAT(FScopeCycleCounter _CounterName_{GET_STATID(_CounterName_)});

// --------------------------------------------------------------------------------------------------------------------
