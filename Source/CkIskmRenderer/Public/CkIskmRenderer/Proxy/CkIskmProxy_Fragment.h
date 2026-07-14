#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/StrongObjectPtr.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment_Data.h"

#include <variant>

class UCkUsf_OutlinePreset;

namespace ck
{
    // ---- tags ----

    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_IskmProxy_NeedsSetup);
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_IskmProxy_PendingAsyncLoad);
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_IskmProxy_HasActiveMontage);
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_IskmProxy_Ragdolling);
    // A3: Movable proxies have this tag set at Setup based on ParamsData._IsMovable.
    // The UpdateTransform processor includes FTag_IskmProxy_Movable AND
    // FTag_Transform_Updated (a CkEcsExt convention set when the entity transform
    // changes); static proxies are skipped entirely each frame.
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_IskmProxy_Movable);

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

        // Per-instance local render offset (entity-space), cached from ParamsData at Setup and composed
        // into the SKMC world transform each frame (Setup + UpdateTransform). Lets a proxy render off
        // its entity origin — e.g. a Character-actor entity whose transform is the capsule CENTER drops
        // by the half-height so the feet land on the ground. Zero for the common case (entity at feet).
        FVector _LocalLocationOffset = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_BaseSKMC);
        CK_PROPERTY_GET(_SubmeshSKMCs);
        CK_PROPERTY_GET(_AttachedSubmeshIndices);
        CK_PROPERTY_GET(_LocalLocationOffset);
    };

    // ---- entity outline (see CkUsf/DESIGN_EntityOutlines.md) ----
    //
    // Applied-state for ck::FFragment_Usf_OutlineTarget on Plan-1 proxies: the outline Sync processor set
    // Custom Depth + the preset's stencil on the BaseSKMC (and outfit submeshes, re-asserted per frame so
    // late-attached submeshes inherit it). Undo path clears the flags + releases the stencil refcount;
    // Release_BaseSKMC additionally strips custom depth unconditionally (pool hygiene).
    struct CKISKMRENDERER_API FFragment_IskmProxy_OutlineApplied
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmProxy_OutlineApplied);

    private:
        TWeakObjectPtr<UCkUsf_OutlinePreset> _Preset;
        uint8 _StencilValue = 0;

    public:
        CK_PROPERTY_GET(_Preset);
        CK_PROPERTY_GET_BY_COPY(_StencilValue);

        CK_DEFINE_CONSTRUCTORS(FFragment_IskmProxy_OutlineApplied, _Preset, _StencilValue);
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

    // ---- socket follower ----
    //
    // Lives on a FOLLOWER entity (e.g. a hair/hat cosmetic rendered via CkIsm)
    // whose Transform must track a socket on an IskmProxy LEADER. The follower's
    // world transform is recomputed by FProcessor_IskmProxy_SocketFollower_SyncTransform
    // AFTER the Transform request pass as:
    //     Offset × Socket(Component-space) × LeaderEntityTransform
    // Sampling the SKMC's WORLD-space socket instead (as a SyncFrom-group
    // processor would) reads the leader's previous-frame position — the SKMC is
    // only moved at PostTransform — and the follower trails by one frame of
    // velocity. Component-space sampling keeps only the animation pose a frame
    // stale (sub-cm) while the root-motion term is always current.
    //
    // EXCEPTION — ragdoll: the entity-transform root is only valid while UpdateTransform
    // keeps the SKMC at the entity transform. During ragdoll physics owns the SKMC and
    // UpdateTransform is excluded (TExclude<FTag_IskmProxy_Ragdolling>), so the entity
    // transform is frozen at the death pose. SyncTransform then reads the WORLD-space
    // socket directly (no entity root, no _LocalLocationOffset) so the follower stays on
    // the physics pose. See FProcessor_IskmProxy_SocketFollower_SyncTransform.
    struct CKISKMRENDERER_API FFragment_IskmProxy_SocketFollower
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmProxy_SocketFollower);

    private:
        FCk_Handle_IskmProxy _Leader;
        FName _Socket;
        FTransform _Offset = FTransform::Identity;

    public:
        CK_PROPERTY_GET(_Leader);
        CK_PROPERTY_GET(_Socket);
        CK_PROPERTY_GET(_Offset);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_IskmProxy_SocketFollower, _Leader, _Socket, _Offset);
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

    // ---- per-proxy material overrides ----
    //
    // V1 scope: overrides apply to the BASE SKMC only — submeshes carry their own
    // def-time override materials (FCk_IskmRenderer_MeshDesc::_OverrideMaterials).
    // Sparse: only overridden slots are stored. TStrongObjectPtr pins the material
    // so an applied override can't be GC'd out from under the pooled SKMC.
    //
    // Pooling discipline (load-bearing): the SKMC is borrowed from the renderer
    // pool and OverrideMaterials is a component-level array that survives
    // Release_BaseSKMC's SetSkeletalMesh(nullptr). EndPlay calls
    // EmptyOverrideMaterials() on release so the next borrower sees mesh-default
    // materials; Setup re-applies this map in case the SKMC is (re)acquired after
    // overrides were recorded.
    struct CKISKMRENDERER_API FFragment_IskmProxy_MaterialOverrides
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmProxy_MaterialOverrides);
        friend class FProcessor_IskmProxy_Setup;
        friend class FProcessor_IskmProxy_HandleRequests;

    private:
        TMap<int32, TStrongObjectPtr<UMaterialInterface>> _SlotToMaterial;
        bool _Dirty = false;

    public:
        CK_PROPERTY_GET(_SlotToMaterial);
    };

    // ---- per-proxy morph targets ----
    //
    // V1 scope: morphs apply to the BASE body mesh only — LeaderPoseComponent
    // copies bone transforms, not morph curves, so outfit submeshes do NOT
    // inherit these. If future modular skeletal clothing needs shared morphs,
    // that's a separate change (per-submesh curve propagation).
    //
    // Pooling discipline (load-bearing): MorphTargetCurves is component-level
    // state that survives Release_BaseSKMC. EndPlay calls ClearMorphTargets()
    // on release so the next borrower starts clean; Setup re-applies this map
    // in case the SKMC is (re)acquired after morphs were recorded.
    struct CKISKMRENDERER_API FFragment_IskmProxy_MorphTargets
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmProxy_MorphTargets);
        friend class FProcessor_IskmProxy_Setup;
        friend class FProcessor_IskmProxy_HandleRequests;

    private:
        TMap<FName, float> _Values;
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
            FCk_Request_IskmProxy_SetMaterialOverride,
            FCk_Request_IskmProxy_ClearMaterialOverrides,
            FCk_Request_IskmProxy_SetMorphTarget,
            FCk_Request_IskmProxy_ClearMorphTargets,
            FCk_Request_IskmProxy_SetSkeletalMesh,
            FCk_Request_IskmProxy_AttachSubmesh,
            FCk_Request_IskmProxy_DetachSubmesh,
            FCk_Request_IskmProxy_DetachAllSubmeshes,
            FCk_Request_IskmProxy_SetAnimInstanceClass,
            FCk_Request_IskmProxy_PlayMontage,
            FCk_Request_IskmProxy_StopMontage,
            FCk_Request_IskmProxy_BeginRagdoll,
            FCk_Request_IskmProxy_EndRagdoll,
            FCk_Request_IskmProxy_SetVisibility>;

    public:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };
}
