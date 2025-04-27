#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment_Data.h"

#include <variant>
#include <Components/InstancedStaticMeshComponent.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_IsmRenderer_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_IsmRenderer_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_IsmRenderer_Movable);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKISMRENDERER_API FFragment_IsmRenderer_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_IsmRenderer_Params);

    public:
        using ParamsType = TWeakObjectPtr<const UCk_IsmRenderer_Data>;

    public:
        FFragment_IsmRenderer_Params() = default;
        explicit FFragment_IsmRenderer_Params(
            const UCk_IsmRenderer_Data* InParams);

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKISMRENDERER_API FFragment_IsmRenderer_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_IsmRenderer_Current);

    public:
        friend class FProcessor_IsmRenderer_Setup;

    private:
        TWeakObjectPtr<UInstancedStaticMeshComponent> _IsmComponent;

    public:
        CK_PROPERTY_GET(_IsmComponent);
    };
}

// --------------------------------------------------------------------------------------------------------------------
