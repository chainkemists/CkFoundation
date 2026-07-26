#pragma once

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

struct CKWATERMARK_API FCkWatermarkActivityState
{
    FName Id;

    FText Label;

    // Per-Id, assigned once at creation and never changed.
    int32 SequenceNumber = 0;

    bool IsActive = false;

    // GFrameCounter values, not world time.
    uint64 ActivatedFrame = 0;
    uint64 DeactivatedFrame = 0;
};

// --------------------------------------------------------------------------------------------------------------------
