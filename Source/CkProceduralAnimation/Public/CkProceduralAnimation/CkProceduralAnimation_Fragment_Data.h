#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkProceduralAnimation_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ProceduralAnimation_Status : uint8
{
    PendingSetup,
    Ready,
    Failed
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ProceduralAnimation_Status);

// --------------------------------------------------------------------------------------------------------------------
