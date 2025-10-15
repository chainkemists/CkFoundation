#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkVfx_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKVFX_API UCk_Utils_VfxCue_Settings : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VfxCue_Settings);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::vfx::settings
{
}

// --------------------------------------------------------------------------------------------------------------------
