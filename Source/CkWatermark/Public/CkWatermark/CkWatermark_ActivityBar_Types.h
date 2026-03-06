#pragma once

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// State of a single activity signal in the watermark activity bar.
// Pushed by game code via UCk_Watermark_Subsystem_UE::Request_ActivityActive / Inactive.

struct CKWATERMARK_API FCkWatermarkActivityState
{
    // Unique identifier for this signal (e.g. "W", "LMB", "Jump").
    FName Id;

    // Short display text rendered inside the chip brackets.
    FText Label;

    // Monotonically increasing sequence number per Id (1, 2, 3…).
    // Assigned once when the entry is created — never changes.
    int32 SequenceNumber = 0;

    // Whether the signal is currently active.
    bool bActive = false;

    // GFrameCounter when the signal was last activated.
    uint64 ActivatedFrame = 0;

    // GFrameCounter when the signal was last deactivated.
    uint64 DeactivatedFrame = 0;
};

// --------------------------------------------------------------------------------------------------------------------
