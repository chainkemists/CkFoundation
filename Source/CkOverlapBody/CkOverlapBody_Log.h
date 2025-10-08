#pragma once

#include "CkCore/Log/CkLog.h"

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

// --------------------------------------------------------------------------------------------------------------------

CKOVERLAPBODY_API DECLARE_LOG_CATEGORY_EXTERN(CkOverlapBody, Log, All);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::overlap_body
{
    CK_DEFINE_LOG_FUNCTIONS(CkOverlapBody);
}

// --------------------------------------------------------------------------------------------------------------------