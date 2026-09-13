#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkUsf/Outline/CkUsf_Outline_Types.h"
#include "GameplayTagContainer.h"

class UCkUsf_OutlinePreset;
class AActor;
class UPrimitiveComponent;

namespace ck
{
    struct CKUSF_API FUsf_OutlineClaim
    {
        FCk_Handle Source;
        FGameplayTag OutlineTag;
        ECk_Usf_OutlineScope Scope = ECk_Usf_OutlineScope::EntityOnly;
    };

    struct CKUSF_API FFragment_Usf_OutlineClaims
    {
        CK_GENERATED_BODY(FFragment_Usf_OutlineClaims);
        TArray<FUsf_OutlineClaim> _Claims;
    };

    struct CKUSF_API FFragment_Usf_OutlineResolved
    {
    public:
        CK_GENERATED_BODY(FFragment_Usf_OutlineResolved);

    private:
        FCk_Handle _Source;
        FGameplayTag _OutlineTag;
        FGameplayTag _LayerTag;
        TWeakObjectPtr<UCkUsf_OutlinePreset> _Preset;
        int32 _LayerIndex = INDEX_NONE;
        int32 _OwnershipDistance = MAX_int32;

    public:
        CK_PROPERTY_GET(_Source);
        CK_PROPERTY_GET(_OutlineTag);
        CK_PROPERTY_GET(_LayerTag);
        CK_PROPERTY_GET(_Preset);
        CK_PROPERTY_GET(_LayerIndex);
        CK_PROPERTY_GET(_OwnershipDistance);

        CK_DEFINE_CONSTRUCTORS(
            FFragment_Usf_OutlineResolved,
            _Source,
            _OutlineTag,
            _LayerTag,
            _Preset,
            _LayerIndex,
            _OwnershipDistance);
    };

    struct CKUSF_API FFragment_Usf_OutlineApplied_Actor
    {
    public:
        CK_GENERATED_BODY(FFragment_Usf_OutlineApplied_Actor);

    private:
        TWeakObjectPtr<AActor> _Actor;
        TArray<TWeakObjectPtr<UPrimitiveComponent>> _Components;

    public:
        CK_PROPERTY_GET(_Actor);
        CK_PROPERTY_GET(_Components);
        CK_DEFINE_CONSTRUCTORS(FFragment_Usf_OutlineApplied_Actor, _Actor, _Components);
    };

    struct CKUSF_API FFragment_Usf_OutlineApplied_Component
    {
    public:
        CK_GENERATED_BODY(FFragment_Usf_OutlineApplied_Component);

    private:
        TWeakObjectPtr<UPrimitiveComponent> _Component;

    public:
        CK_PROPERTY_GET(_Component);
        CK_DEFINE_CONSTRUCTORS(FFragment_Usf_OutlineApplied_Component, _Component);
    };
}
