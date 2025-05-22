#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkScene.generated.h"

// ----------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKCORE_API FCk_Handle_PrimitiveDrawInterface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Handle_PrimitiveDrawInterface);

private:
    FPrimitiveDrawInterface* _PrimitiveDrawInterface;

public:
    CK_PROPERTY_GET(_PrimitiveDrawInterface);

    CK_DEFINE_CONSTRUCTORS(FCk_Handle_PrimitiveDrawInterface, _PrimitiveDrawInterface);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_Handle_PrimitiveDrawInterface, ck::IsValid_Policy_Default, [&](const FCk_Handle_PrimitiveDrawInterface& InObj)
{
    return InObj.Get_PrimitiveDrawInterface() != nullptr;
});

// ----------------------------------------------------------------------------------------------------------------
