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
        
// This option enables setting the description to the type name which is
// desired for Insights, but may be too long for the `stat mystats` command
#if CK_ENABLE_STAT_DESCRIPTION
        
        static auto Description = []()
        {
            auto CleanName = cleantype::clean<ValueType>();
            return FString{static_cast<int32>(CleanName.length()), CleanName.data()};
        }();
#else
        static auto Description = []()
        {
            return FString{};
        }();
#endif
        
        return *Description;
    }

#if STATS
    template <typename T_StatName>
    auto
        TStat_Id<T_StatName>::
        GetStatType()
        -> EStatDataType::Type
    {
        return EStatDataType::ST_int64;
    }
#endif

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
