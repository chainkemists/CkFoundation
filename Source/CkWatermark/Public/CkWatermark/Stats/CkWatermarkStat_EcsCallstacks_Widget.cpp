#include "CkWatermarkStat_EcsCallstacks_Widget.h"

#include "CkEcs/Settings/CkEcs_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_EcsCallstacks_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("Callstacks"));
}

auto
    UCkWatermarkStat_EcsCallstacks_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    const bool bCpp = UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Cpp();
    const bool bBP  = UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Blueprint();
    const bool bAS  = UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Angelscript();

    if (!bCpp && !bBP && !bAS)
    {
        return FText::FromString(TEXT("---"));
    }

    FString Result;
    if (bCpp) { Result += TEXT("C++"); }
    if (bBP)  { Result += Result.IsEmpty() ? TEXT("BP") : TEXT(" BP"); }
    if (bAS)  { Result += Result.IsEmpty() ? TEXT("AS") : TEXT(" AS"); }
    return FText::FromString(Result);
}

// --------------------------------------------------------------------------------------------------------------------
