#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Tag/CkTag.h"

class UCk_IskmAnimCollection_Data;
class UCk_IskmRenderer_Data;
class ACk_IskmRenderer_Actor_UE;

namespace ck
{
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_IskmRenderer_NeedsSetup);
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_IskmRenderer_PendingAsyncLoad);

    struct CKISKMRENDERER_API FFragment_IskmRenderer_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmRenderer_Params);

    public:
        FFragment_IskmRenderer_Params() = default;
        explicit FFragment_IskmRenderer_Params(UCk_IskmRenderer_Data* InRendererData);

    private:
        TWeakObjectPtr<UCk_IskmRenderer_Data> _RendererData;

    public:
        CK_PROPERTY_GET(_RendererData);
    };

    struct CKISKMRENDERER_API FFragment_IskmRenderer_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmRenderer_Current);

    public:
        friend class FProcessor_IskmRenderer_Setup;
        friend class UCk_Utils_IskmRenderer_UE;

    private:
        TWeakObjectPtr<ACk_IskmRenderer_Actor_UE> _RendererActor;

    public:
        CK_PROPERTY_GET(_RendererActor);
    };
}
