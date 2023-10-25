#pragma once

#include "CkStats.h"

#include "cleantype/details/cleantype_clean.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
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
}

// --------------------------------------------------------------------------------------------------------------------
