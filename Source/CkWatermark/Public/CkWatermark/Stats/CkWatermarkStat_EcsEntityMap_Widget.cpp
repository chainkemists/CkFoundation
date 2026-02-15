#include "CkWatermarkStat_EcsEntityMap_Widget.h"

#include "CkEcs/Settings/CkEcs_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_EcsEntityMap_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("Entity Map"));
}

auto
    UCkWatermarkStat_EcsEntityMap_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    switch (UCk_Utils_Ecs_Settings_UE::Get_EntityMapPolicy())
    {
        case ECk_Ecs_EntityMap_Policy::DoNotLog:  return FText::FromString(TEXT("Off"));
        case ECk_Ecs_EntityMap_Policy::AlwaysLog: return FText::FromString(TEXT("Always Log"));
        default:                                   return FText::FromString(TEXT("---"));
    }
}

// --------------------------------------------------------------------------------------------------------------------
