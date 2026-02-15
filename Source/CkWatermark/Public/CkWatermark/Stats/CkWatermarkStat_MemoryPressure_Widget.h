#pragma once

#include "CkWatermark/Stats/CkWatermarkStat_Base_Widget.h"

#include "CkWatermarkStat_MemoryPressure_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Ck Watermark Stat - Memory Pressure",
               Category    = "CkFoundation|Watermark Stats"))
class CKWATERMARK_API UCkWatermarkStat_MemoryPressure_UWidget_UE : public UCkWatermarkStat_Base_UWidget_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkWatermarkStat_MemoryPressure_UWidget_UE);

    auto NativeGetStatName()  const -> FText         override;
    auto NativeGetStatValue() const -> FText         override;
    auto NativeGetStatColor() const -> FLinearColor  override;
};

// --------------------------------------------------------------------------------------------------------------------
