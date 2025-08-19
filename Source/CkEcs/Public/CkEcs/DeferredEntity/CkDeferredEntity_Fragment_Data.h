#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkDeferredEntity_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKECS_API FCk_Handle_DeferredEntity : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_DeferredEntity);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_DeferredEntity);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_DeferredEntity_OnComplete, FCk_Handle_DeferredEntity, InDeferredEntity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCk_Delegate_DeferredEntity_OnComplete_MC, FCk_Handle_DeferredEntity, InDeferredEntity);

DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_DeferredEntity_OnFullyComplete, FCk_Handle_DeferredEntity, InDeferredEntity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCk_Delegate_DeferredEntity_OnFullyComplete_MC, FCk_Handle_DeferredEntity, InDeferredEntity);

// --------------------------------------------------------------------------------------------------------------------