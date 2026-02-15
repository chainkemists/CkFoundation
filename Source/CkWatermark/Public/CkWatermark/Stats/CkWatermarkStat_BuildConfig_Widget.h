#pragma once

#include "CkWatermark/Stats/CkWatermarkStat_Base_Widget.h"

#include "CkWatermarkStat_BuildConfig_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Shows the current build configuration: Development | Test | Shipping
// Evaluated at compile time -- no runtime overhead.

UCLASS(meta = (DisplayName = "Ck Watermark Stat - Build Config",
               Category    = "CkFoundation|Watermark Stats"))
class CKWATERMARK_API UCkWatermarkStat_BuildConfig_UWidget_UE : public UCkWatermarkStat_Base_UWidget_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkWatermarkStat_BuildConfig_UWidget_UE);

    auto NativeGetStatName()  const -> FText         override;
    auto NativeGetStatValue() const -> FText         override;
    auto NativeGetStatColor() const -> FLinearColor  override;
};

// --------------------------------------------------------------------------------------------------------------------
