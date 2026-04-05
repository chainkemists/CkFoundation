#pragma once

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkShape_Handle.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSHAPES_API FCk_Handle_Shape : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Shape);

    friend struct FCk_Handle_ShapeBox;
    friend struct FCk_Handle_ShapeSphere;
    friend struct FCk_Handle_ShapeCapsule;
    friend struct FCk_Handle_ShapeCylinder;
};
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Shape);

// --------------------------------------------------------------------------------------------------------------------
