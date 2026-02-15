#include "CkWatermarkStat_BuildConfig_Widget.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_BuildConfig_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("Build"));
}

auto
    UCkWatermarkStat_BuildConfig_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
#if UE_BUILD_SHIPPING
    return FText::FromString(TEXT("SHIPPING"));
#elif UE_BUILD_TEST
    return FText::FromString(TEXT("TEST"));
#else
    return FText::FromString(TEXT("DEV"));
#endif
}

auto
    UCkWatermarkStat_BuildConfig_UWidget_UE::
    NativeGetStatColor() const
    -> FLinearColor
{
#if UE_BUILD_SHIPPING
    // Shipping is green — this is the release build
    return FLinearColor(0.0f, 1.0f, 0.4f, 1.0f);
#elif UE_BUILD_TEST
    // Test is yellow — QA / testing build
    return FLinearColor(1.0f, 0.85f, 0.0f, 1.0f);
#else
    // Development is a muted blue — always-on debug build
    return FLinearColor(0.3f, 0.7f, 1.0f, 1.0f);
#endif
}

// --------------------------------------------------------------------------------------------------------------------
