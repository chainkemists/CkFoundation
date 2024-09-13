#pragma once

#include "CkCore/Log/CkLog.h"

// --------------------------------------------------------------------------------------------------------------------

CKABILITY_API DECLARE_LOG_CATEGORY_EXTERN(CkAbility, Log, All);
CKABILITY_API DECLARE_LOG_CATEGORY_EXTERN(CkAbilityCue, Log, All);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::ability
{
    CK_DEFINE_LOG_FUNCTIONS(CkAbility);
}

namespace ck::ability_cue
{
    CK_DEFINE_LOG_FUNCTIONS(CkAbilityCue);
}

// --------------------------------------------------------------------------------------------------------------------