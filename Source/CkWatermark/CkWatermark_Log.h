#pragma once

#include "CkCore/Log/CkLog.h"

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

// --------------------------------------------------------------------------------------------------------------------

CKWATERMARK_API DECLARE_LOG_CATEGORY_EXTERN(CkWatermark, Log, All);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::watermark
{
    CK_DEFINE_LOG_FUNCTIONS(CkWatermark);
}

// --------------------------------------------------------------------------------------------------------------------
