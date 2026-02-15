#include "CkWatermarkStat_EcsWorldTickingGroup_Widget.h"

#include "CkEcs/Subsystem/CkEcsWorldStats_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_EcsWorldTickingGroup_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    if (!_TickingGroup.IsValid())
    {
        return FText::FromString(TEXT("ECS"));
    }

    // Extract the last segment of the tag (e.g. "GameplayWithPump")
    const FString TagString = _TickingGroup.GetTagName().ToString();
    FString ShortName;
    if (!TagString.Split(TEXT("."), nullptr, &ShortName,
                         ESearchCase::IgnoreCase, ESearchDir::FromEnd))
    {
        ShortName = TagString;
    }
    return FText::FromString(ShortName);
}

auto
    UCkWatermarkStat_EcsWorldTickingGroup_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    if (!_TickingGroup.IsValid())
    {
        return FText::FromString(TEXT("---"));
    }

    const UWorld* World = GetWorld();
    if (!World)
    {
        return FText::FromString(TEXT("---"));
    }

    const UCk_EcsWorld_Stats_Subsystem_UE* StatsSubsystem =
        World->GetSubsystem<UCk_EcsWorld_Stats_Subsystem_UE>();
    if (!StatsSubsystem)
    {
        return FText::FromString(TEXT("---"));
    }

    const float CycleMs = StatsSubsystem->Get_StatDataForEcsWorldTickingGroup(
        _TickingGroup, _StatSource);

    FNumberFormattingOptions Opts;
    Opts.MinimumFractionalDigits = 2;
    Opts.MaximumFractionalDigits = 2;
    Opts.UseGrouping = false;

    return FText::Format(FText::FromString(TEXT("{0} ms")),
                         FText::AsNumber(CycleMs, &Opts));
}

// --------------------------------------------------------------------------------------------------------------------
