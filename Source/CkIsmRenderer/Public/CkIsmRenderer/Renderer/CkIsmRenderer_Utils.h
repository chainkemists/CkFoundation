#pragma once

#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment_Data.h"

#include "CkIsmRenderer_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_IsmRenderer"))
class CKISMRENDERER_API UCk_Utils_IsmRenderer_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IsmRenderer_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_IsmRenderer);

public:
    static FCk_Handle_IsmRenderer
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const UCk_IsmRenderer_Data* InParams);

    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    static int32
    Get_CurrentInstanceCount(
        const FCk_Handle_IsmRenderer& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------