#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment_Data.h"

#include <variant>

namespace ck
{
    // ---- tags ----

    CK_DEFINE_ECS_TAG(FTag_IskmProxy_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_IskmProxy_PendingAsyncLoad);
    CK_DEFINE_ECS_TAG(FTag_IskmProxy_HasActiveMontage);
    CK_DEFINE_ECS_TAG(FTag_IskmProxy_Ragdolling);
    // A3: Movable proxies have this tag set at Setup based on ParamsData._IsMovable.
    // The UpdateTransform processor includes FTag_IskmProxy_Movable AND
    // FTag_Transform_Updated (a CkEcsExt convention set when the entity transform
    // changes); static proxies are skipped entirely each frame.
    CK_DEFINE_ECS_TAG(FTag_IskmProxy_Movable);

    // ---- params (non-reflected ECS-side alias) ----

    using FFragment_IskmProxy_Params = FCk_Fragment_IskmProxy_ParamsData;

    // ---- current ----

    // ============================================================================
    // A2: Plan-1 → Plan-2 migration — load-bearing fragment.
    //
    // Plan-1 stores a `TWeakObjectPtr<USkeletalMeshComponent>` per entity. Plan-2 will
    // replace this with `int32 _InstanceIndex + uint32 _InstanceVersion` — an SOA index
    // into the renderer's instance arrays, no per-entity SKMC for sequence-mode entities.
    //
    // **Do not leak `_BaseSKMC` access outside `UCk_Utils_IskmProxy_UE` and the proxy
    // processors below.** Public API methods (`Get_SocketTransform`, `LineTrace_Instance`,
    // `Get_PlayingAnimation`, etc.) must be implementable from either shape. Reaching
    // into `_BaseSKMC` from a different module, fragment, or external processor would
    // multiply the Plan-2 migration cost.
    // ============================================================================
    struct CKISKMRENDERER_API FFragment_IskmProxy_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmProxy_Current);
        friend class FProcessor_IskmProxy_Setup;
        friend class FProcessor_IskmProxy_HandleRequests;
        friend class FProcessor_IskmProxy_UpdateTransform;
        friend class FProcessor_IskmProxy_EmitFinishedEvents;
        friend class FProcessor_IskmProxy_EndPlay;
        friend class UCk_Utils_IskmProxy_UE;

    private:
        // Plan-1 implementation; replaced in Plan-2 by InstanceIndex + InstanceVersion.
        TWeakObjectPtr<USkeletalMeshComponent> _BaseSKMC;

        // child SKMCs that follow BaseSKMC's pose via LeaderPoseComponent
        TArray<TWeakObjectPtr<USkeletalMeshComponent>> _SubmeshSKMCs;

        // index into AnimCollection->Submeshes; parallel to _SubmeshSKMCs
        TArray<int32> _AttachedSubmeshIndices;

    public:
        CK_PROPERTY_GET(_BaseSKMC);
        CK_PROPERTY_GET(_SubmeshSKMCs);
        CK_PROPERTY_GET(_AttachedSubmeshIndices);
    };

    // ---- anim state ----

    struct CKISKMRENDERER_API FFragment_IskmProxy_AnimState
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmProxy_AnimState);
        friend class FProcessor_IskmProxy_HandleRequests;
        friend class FProcessor_IskmProxy_EmitFinishedEvents;
        // Phase M: UCk_IskmNotify_AnimInstance::NativeOnMontageBlendingOut clears
        // _CurrentMontage when the bridged anim's montage ends, before broadcasting
        // OnMontageFinished. Friend access required for the private-member reset.
        friend class UCk_IskmNotify_AnimInstance;

    private:
        TWeakObjectPtr<UAnimSequenceBase> _CurrentSequence;
        TWeakObjectPtr<UAnimMontage> _CurrentMontage;
        bool _LastFinishedDispatched = true;
    public:
        CK_PROPERTY_GET(_CurrentSequence);
        CK_PROPERTY_GET(_CurrentMontage);
    };

    // ---- pose source ----
    //
    // Plan-1 stores the pose source as a single enum on a dedicated fragment. Plan-2 will
    // promote this to a tag-per-source (FTag_IskmProxy_PoseSource_Sequence / _AnimBP /
    // _Ragdoll) so that the cluster-update processor can include sequence-mode entities and
    // exclude AnimBP/Ragdoll entities via TInclude/TExclude without reading every entity's
    // fragment. Keeping it as a separate fragment now makes that promotion a localized
    // change. Don't merge into AnimState.
    struct CKISKMRENDERER_API FFragment_IskmProxy_PoseSource
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmProxy_PoseSource);
        friend class FProcessor_IskmProxy_Setup;
        friend class FProcessor_IskmProxy_HandleRequests;
        friend class UCk_Utils_IskmProxy_UE;
    private:
        ECk_IskmProxy_PoseSource _PoseSource = ECk_IskmProxy_PoseSource::Sequence;
    public:
        CK_PROPERTY_GET(_PoseSource);
    };

    // ---- per-instance custom data ----

    struct CKISKMRENDERER_API FFragment_IskmProxy_CustomData
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmProxy_CustomData);
        friend class FProcessor_IskmProxy_Setup;
        friend class FProcessor_IskmProxy_HandleRequests;
    private:
        TArray<float> _Values;
        bool _Dirty = false;
    public:
        CK_PROPERTY_GET(_Values);
    };

    // ---- request fragment ----

    struct CKISKMRENDERER_API FFragment_IskmProxy_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmProxy_Requests);
        friend class FProcessor_IskmProxy_HandleRequests;
        friend class UCk_Utils_IskmProxy_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_IskmProxy_PlayAnimation,
            FCk_Request_IskmProxy_StopAnimation,
            FCk_Request_IskmProxy_SetPlayRate,
            FCk_Request_IskmProxy_SetCustomDataFloat,
            FCk_Request_IskmProxy_PlayMontage,
            FCk_Request_IskmProxy_StopMontage,
            FCk_Request_IskmProxy_BeginRagdoll>;

    public:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };
}
