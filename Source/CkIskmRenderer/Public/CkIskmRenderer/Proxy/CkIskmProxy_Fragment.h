#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/StrongObjectPtr.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment_Data.h"

#include "CkUsf/Outline/CkUsf_OutlinePreset.h"
#include "CkUsf/Stylize/CkUsf_CelShade_Params.h"

#include <variant>

class UCk_Utils_IskmProxy_UE;

namespace ck
{
    // ---- tags ----

    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_IskmProxy_NeedsSetup);
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_IskmProxy_PendingAsyncLoad);
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_IskmProxy_HasActiveMontage);
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_IskmProxy_Ragdolling);
    // Set at Setup from ParamsData._IsMovable. UpdateTransform requires it alongside
    // FTag_Transform_Updated, so a static proxy is skipped entirely each frame.
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_IskmProxy_Movable);

    // ---- params (non-reflected ECS-side alias) ----

    using FFragment_IskmProxy_Params = FCk_Fragment_IskmProxy_ParamsData;

    // ---- current ----

    // Do NOT leak `_BaseSKMC` access outside `UCk_Utils_IskmProxy_UE` and the proxy processors below:
    // the public API (Get_SocketTransform, LineTrace_Instance, ...) must stay implementable from the
    // Plan-2 SOA shape (instance index + version) that eventually replaces the per-entity SKMC.
    struct CKISKMRENDERER_API FFragment_IskmProxy_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmProxy_Current);
        friend class FProcessor_IskmProxy_Setup;
        friend class FProcessor_IskmProxy_HandleRequests;
        friend class FProcessor_IskmProxy_HandleLateCustomDataRequests;
        friend class FProcessor_IskmProxy_UpdateTransform;
        friend class FProcessor_IskmProxy_EmitFinishedEvents;
        friend class FProcessor_IskmProxy_EndPlay;
        friend class UCk_Utils_IskmProxy_UE;

    private:
        TWeakObjectPtr<USkeletalMeshComponent> _BaseSKMC;

        // child SKMCs that follow BaseSKMC's pose via LeaderPoseComponent
        TArray<TWeakObjectPtr<USkeletalMeshComponent>> _SubmeshSKMCs;

        // index into AnimCollection->Submeshes; parallel to _SubmeshSKMCs
        TArray<int32> _AttachedSubmeshIndices;

        // Entity-space render offset, composed into the SKMC world transform every frame so a proxy can
        // render off its entity origin — e.g. a Character entity whose transform is the capsule CENTER
        // drops by the half-height to land its feet. Zero for the common case (entity at feet).
        FVector _LocalLocationOffset = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_BaseSKMC);
        CK_PROPERTY_GET(_SubmeshSKMCs);
        CK_PROPERTY_GET(_AttachedSubmeshIndices);
        CK_PROPERTY_GET(_LocalLocationOffset);
    };

    // ---- entity outline (see CkUsf/Claude.md § Entity outlines) ----
    //
    // Applied-state for ck::FFragment_Usf_OutlineTarget on Plan-1 proxies: the Sync processor set Custom
    // Depth + the preset's stencil on the BaseSKMC and its submeshes; removing the fragment undoes both.
    struct CKISKMRENDERER_API FFragment_IskmProxy_OutlineApplied
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmProxy_OutlineApplied);

    private:
        // Strong: this fragment holds a stencil refcount keyed by the preset in the outline subsystem's
        // WEAK-keyed _ActivePresets map. A preset that died while claimed would leave a stale key that
        // can never be found again, leaking the stencil slot until world teardown.
        TStrongObjectPtr<UCkUsf_OutlinePreset> _Preset;
        uint8 _StencilValue = 0;

    public:
        CK_PROPERTY_GET(_Preset);
        CK_PROPERTY_GET_BY_COPY(_StencilValue);

        CK_DEFINE_CONSTRUCTORS(FFragment_IskmProxy_OutlineApplied, _Preset, _StencilValue);
    };

    // ---- entity cel pattern (see CkUsf/Claude.md § Cel shade (Stylize)) ----
    //
    // Applied-state for ck::FFragment_Usf_CelPatternTarget on Plan-1 proxies: the Sync processor set Custom
    // Depth + the pattern's stencil on the BaseSKMC and its submeshes; removing the fragment undoes both.
    // No strong preset ref here (unlike the outline twin) because the cel contract is a direct stencil value
    // rather than a refcounted allocation — there is nothing to keep alive and nothing to release.
    struct CKISKMRENDERER_API FFragment_IskmProxy_CelPatternApplied
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmProxy_CelPatternApplied);

    private:
        ECk_Usf_CelPattern _Pattern = ECk_Usf_CelPattern::Bayer;
        int32 _StencilValue = 0;

    public:
        CK_PROPERTY_GET_BY_COPY(_Pattern);
        CK_PROPERTY_GET_BY_COPY(_StencilValue);

        CK_DEFINE_CONSTRUCTORS(FFragment_IskmProxy_CelPatternApplied, _Pattern, _StencilValue);
    };

    // ---- entity stylize effect mask (see CkUsf/Claude.md § Stylize) ----
    //
    // Applied-state for ck::FFragment_Usf_StylizeMaskTarget on Plan-1 proxies: the Sync processor set Custom
    // Depth + the project's mask stencil on the BaseSKMC and its submeshes; removing the fragment undoes
    // both. The cel-pattern twin above minus the pattern — the mask has ONE project-wide value, so the
    // recorded stencil exists only to tell a no-op refresh from a real change.
    struct CKISKMRENDERER_API FFragment_IskmProxy_StylizeMaskApplied
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmProxy_StylizeMaskApplied);

    private:
        int32 _StencilValue = 0;

    public:
        CK_PROPERTY_GET_BY_COPY(_StencilValue);

        CK_DEFINE_CONSTRUCTORS(FFragment_IskmProxy_StylizeMaskApplied, _StencilValue);
    };

    // ---- anim state ----

    struct CKISKMRENDERER_API FFragment_IskmProxy_AnimState
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmProxy_AnimState);
        friend class FProcessor_IskmProxy_HandleRequests;
        friend class FProcessor_IskmProxy_EmitFinishedEvents;
        // Clears _CurrentMontage when the bridged anim's montage ends, before broadcasting OnMontageFinished.
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
    // Do NOT merge into AnimState: staying a separate fragment keeps the eventual promotion to a
    // tag-per-source (which lets views TInclude/TExclude by pose source) a localized change.
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
    // Lives on a FOLLOWER entity whose Transform tracks a socket on an IskmProxy LEADER, recomputed
    // AFTER the Transform pass as: Offset × Socket(component-space) × LeaderEntityTransform. The
    // WORLD-space socket is deliberately NOT sampled — the SKMC only moves at PostTransform, so it
    // trails a frame. EXCEPTION — ragdoll: physics owns the SKMC and the leader's entity transform is
    // frozen at the death pose, so SyncTransform reads the world socket instead.
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
        friend class FProcessor_IskmProxy_HandleLateCustomDataRequests;
    private:
        TArray<float> _Values;
        bool _Dirty = false;
    public:
        CK_PROPERTY_GET(_Values);
    };

    // ---- per-proxy material overrides ----
    //
    // Overrides apply to the BASE SKMC only — submeshes carry their own def-time override materials.
    // Pooling discipline (load-bearing): OverrideMaterials is component-level state that survives
    // Release_BaseSKMC, so EndPlay must EmptyOverrideMaterials() and Setup must re-apply this map.
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
    // Morphs apply to the BASE body mesh only — LeaderPoseComponent copies bone transforms, not morph
    // curves, so submeshes do NOT inherit them. Pooling discipline (load-bearing): MorphTargetCurves
    // survives Release_BaseSKMC, so EndPlay must ClearMorphTargets() and Setup must re-apply this map.
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

    // Opt-in request lane for custom-data writes authored after the normal Gameplay_Rendering request
    // pass. DeferredApply drains it after PostTransform without delaying or reordering the general lane.
    struct CKISKMRENDERER_API FFragment_IskmProxy_LateCustomDataRequests
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmProxy_LateCustomDataRequests);
        friend class FProcessor_IskmProxy_HandleLateCustomDataRequests;
        friend class UCk_Utils_IskmProxy_UE;

    private:
        TArray<FCk_Request_IskmProxy_SetCustomDataFloat> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };
}
