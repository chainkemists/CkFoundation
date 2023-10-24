#pragma once

#include "CkCore/Format/CkFormat.h"

#include "cleantype/details/cleantype_clean.hpp"

#include <Stats/Stats.h>

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

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

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_StatName>
    auto
        TStat_Id<T_StatName>::
        GetStatName()
        -> const char*
    {
        static auto Name = []()
        {
            auto CleanName = cleantype::clean<ValueType>();
            CleanName = CleanName.substr(0, CleanName.find("<")); // remove template noise
            return CleanName;
        }();
        return Name.data();
    }

    template <typename T_StatName>
    auto
        TStat_Id<T_StatName>::
        GetDescription()
        -> const TCHAR*
    {
        // Description _ideally_ would not be shown when we turn on on-screen stats by `stat mystats` command. Otherwise,
        // the full type (if templated) takes up too much room and truncates the useful name of the stat. If we are able
        // to figure out a way to not display the description on-screen but have it still be there in Insights, that would
        // be ideal. Until then, return nothing.

        //static auto Description = []()
        //{
        //    auto CleanName = cleantype::clean<ValueType>();
        //    return FString{static_cast<int32>(CleanName.length()), CleanName.data()};
        //}();

        static auto Description = []()
        {
            return FString{};
        }();

        return *Description;
    }

    template <typename T_StatName>
    auto
        TStat_Id<T_StatName>::
        GetStatType()
        -> EStatDataType::Type
    {
        return EStatDataType::ST_int64;
    }

    template <typename T_StatName>
    auto
        TStat_Id<T_StatName>::
        IsClearEveryFrame()
        -> bool
    {
        return EnumHasAnyFlags(GetFlags(), EStatFlags::ClearEveryFrame);
    }

    template <typename T_StatName>
    auto
        TStat_Id<T_StatName>::
        IsCycleStat()
        -> bool
    {
        return EnumHasAnyFlags(GetFlags(), EStatFlags::CycleStat);
    }

    template <typename T_StatName>
    auto
        TStat_Id<T_StatName>::
        GetFlags()
        -> EStatFlags
    {
        return EStatFlags::ClearEveryFrame | EStatFlags::CycleStat;
    }

    template <typename T_StatName>
    auto
        TStat_Id<T_StatName>::
        GetMemoryRegion()
        -> FPlatformMemory::EMemoryCounterRegion
    {
        return FPlatformMemory::MCR_Invalid;
    }

    // --------------------------------------------------------------------------------------------------------------------
}

#define CK_DEFINE_STAT(CounterName, Type, GroupId)      \
struct FStat_##CounterName : public ck::TStat_Id<Type>  \
{                                                       \
    using TGroup = GroupId;                             \
};                                                      \
static inline DEFINE_STAT(CounterName)

#define CK_STAT(CounterName)\
    STAT(FScopeCycleCounter TickCounter##CounterName{GET_STATID(##CounterName)});

