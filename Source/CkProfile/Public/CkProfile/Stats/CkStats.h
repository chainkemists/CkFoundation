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

#if STATS
        static auto GetStatType() -> EStatDataType::Type;
#endif

        static auto IsClearEveryFrame() -> bool;
        static auto IsCycleStat() -> bool;
        static auto GetFlags() -> EStatFlags;
        static auto GetMemoryRegion() -> FPlatformMemory::EMemoryCounterRegion;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Phase tags for parallel processor stat disambiguation
    struct FStatPhase_ForEachEntity {};
    struct FStatPhase_CollectEntities {};
    struct FStatPhase_ParallelDispatch {};
    struct FStatPhase_FlushCommands {};

    // Phase suffix resolution — specializations in CkStats.inl.h
    template <typename T_Phase>
    struct TStatPhase_Suffix;

    // Phase-aware stat identity — appends phase suffix to processor name
    template <typename T_Processor, typename T_Phase>
    struct TStat_PhaseId
    {
    public:
        using ProcessorType = T_Processor;
        using PhaseType = T_Phase;

    public:
        static auto GetStatName() -> const char*;
        static auto GetDescription() -> const TCHAR*;

#if STATS
        static auto GetStatType() -> EStatDataType::Type;
#endif

        static auto IsClearEveryFrame() -> bool;
        static auto IsCycleStat() -> bool;
        static auto GetFlags() -> EStatFlags;
        static auto GetMemoryRegion() -> FPlatformMemory::EMemoryCounterRegion;
    };
}

#include "CkStats.inl.h"

// --------------------------------------------------------------------------------------------------------------------

#if STATS
#define CK_DEFINE_STAT(_CounterName_, Type, GroupId)    \
struct FStat_##_CounterName_ : public ck::TStat_Id<Type>\
{                                                       \
    using TGroup = GroupId;                             \
};                                                      \
static inline DEFINE_STAT(_CounterName_)

#define CK_DEFINE_PHASE_STAT(_CounterName_, Processor, Phase, GroupId)          \
struct FStat_##_CounterName_ : public ck::TStat_PhaseId<Processor, Phase>       \
{                                                                               \
    using TGroup = GroupId;                                                     \
};                                                                              \
static inline DEFINE_STAT(_CounterName_)

#define CK_CREATE_DYNAMIC_STAT_ID(_StatGroup_, _StatNameOrDescription_)\
    FDynamicStats::CreateStatId<STAT_GROUP_TO_FStatGroup(_StatGroup_)>(_StatNameOrDescription_)

// --------------------------------------------------------------------------------------------------------------------

#define CK_STAT(_CounterName_)\
    STAT(FScopeCycleCounter _CounterName_{GET_STATID(_CounterName_)});

// --------------------------------------------------------------------------------------------------------------------
#else
#define CK_DEFINE_STAT(_CounterName_, Type, GroupId)    \
struct FStat_##_CounterName_ : public ck::TStat_Id<Type>\
{                                                       \
};                                                      \

#define CK_DEFINE_PHASE_STAT(_CounterName_, Processor, Phase, GroupId)          \
struct FStat_##_CounterName_ : public ck::TStat_PhaseId<Processor, Phase>       \
{                                                                               \
};                                                                              \

#define CK_CREATE_DYNAMIC_STAT_ID(_StatGroup_, _StatNameOrDescription_)\
    TStatId{}

// --------------------------------------------------------------------------------------------------------------------

#define CK_STAT(_CounterName_)\
    SCOPED_NAMED_EVENT_TCHAR(FStat_##_CounterName_::GetStatName(), FColor::Red);

// --------------------------------------------------------------------------------------------------------------------
#endif
