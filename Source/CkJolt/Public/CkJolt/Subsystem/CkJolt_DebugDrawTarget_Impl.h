#pragma once

// MODULE-INTERNAL: the Jolt-shaped guts behind FCk_Jolt_DebugDrawTarget's opaque impl. Only the three
// debug-draw translation units include it; CkJolt_DebugDrawTarget.h stays free of every JPH type so a
// presentation consumer can bind a target without seeing Jolt. (CkJolt keeps .cpp files under Public/, so
// path is not the privacy boundary here — inclusion is.)

#include "CkJolt/Subsystem/CkJolt_DebugDrawTarget.h"

#include <Components/LineBatchComponent.h>
#include <Containers/Map.h>
#include <SceneTypes.h>
#include <InstanceDataTypes.h>
#include <Math/Transform.h>
#include <UObject/StrongObjectPtr.h>
#include <UObject/WeakObjectPtr.h>

#include <Jolt/Jolt.h>

#if JPH_DEBUG_RENDERER

#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Renderer/DebugRenderer.h>

// --------------------------------------------------------------------------------------------------------------------

class UMaterialInstanceDynamic;

// ReSharper disable once CppInconsistentNaming
namespace JPH
{
    class Shape;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::debug_draw
{
    // The world-agnostic geometry cache entry: one transient UStaticMesh per unique Jolt geometry. Defined in
    // CkJolt_DebugRenderer.cpp because only the renderer builds and owns them.
    class FBatch;

    // ----------------------------------------------------------------------------------------------------------------

    // Bodies, characters and the two overlays all live in ONE slot map, so the four keyspaces have to be
    // disjoint. A BodyID key occupies the low 32 bits (index + sequence); a character has no BodyID, and each
    // overlay traces a key that is already taken, so all three are lifted clear by their own bit.
    constexpr uint64 CharacterKeyBit = uint64{1} << 40;
    constexpr uint64 HighlightKeyBit = uint64{1} << 41;
    constexpr uint64 HoverKeyBit = uint64{1} << 42;

    // The overlays swell about the body's centre of mass so the traced shape is visible past the body's own
    // surface. Hover is the smaller of the two on purpose — a hover must never read as a selection.
    constexpr float HighlightOverlayScale = 1.03f;
    constexpr float HoverOverlayScale = 1.02f;

    constexpr auto
        Make_CharacterBodyKey_FromEntityId(
            uint64 InEntityId)
        -> uint64
    {
        return CharacterKeyBit | InEntityId;
    }

    constexpr auto
        Make_HighlightKey(
            uint64 InBodyKey)
        -> uint64
    {
        return HighlightKeyBit | InBodyKey;
    }

    constexpr auto
        Make_HoverKey(
            uint64 InBodyKey)
        -> uint64
    {
        return HoverKeyBit | InBodyKey;
    }

    // ----------------------------------------------------------------------------------------------------------------

    // Every line the facility emits lives for exactly one capture: a lifetime of 0 means the component's own tick
    // never expires it, and the capture's Flush is the only thing that clears it.
    constexpr float LineLifeTime = 0.0f;
    constexpr float LineThickness = 0.0f;
    constexpr uint8 LineDepthPriority = static_cast<uint8>(SDPG_World);

    // The per-body flags that emit LINES. Their presence defeats the incremental full pass's skip: a skipped
    // inactive body draws nothing, and because lines are cleared every capture its extras would vanish after the
    // one pass that produced them.
    constexpr ECk_Jolt_DebugDrawFlags PerBodyExtraFlags =
        ECk_Jolt_DebugDrawFlags::Velocity |
        ECk_Jolt_DebugDrawFlags::AngularVelocity |
        ECk_Jolt_DebugDrawFlags::WorldTransform |
        ECk_Jolt_DebugDrawFlags::CenterOfMassTransform |
        ECk_Jolt_DebugDrawFlags::BoundingBox |
        ECk_Jolt_DebugDrawFlags::MassAndInertia;

    inline auto
        Make_DebugDrawLine(
            const FVector& InFrom,
            const FVector& InTo,
            const FLinearColor& InColor)
        -> FBatchedLine
    {
        return FBatchedLine{InFrom, InTo, InColor, LineLifeTime, LineThickness, LineDepthPriority};
    }

    // ----------------------------------------------------------------------------------------------------------------

    // The colour-class INDEX rides the key alongside the packed colour: two classes whose palette entries
    // quantise to the same 8-bit colour must still land in distinct buckets, or the bucket's reported class is a
    // coin flip. An index rather than the BodyClass enum because the index space is shared by every colour mode.
    struct FBucketKey
    {
        FBatch* _Batch = nullptr;
        uint32 _ColorU32 = 0;
        uint8 _ColorClassIndex = 0;

        auto operator==(const FBucketKey& InOther) const -> bool
        {
            return _Batch == InOther._Batch &&
                   _ColorU32 == InOther._ColorU32 &&
                   _ColorClassIndex == InOther._ColorClassIndex;
        }

        friend auto GetTypeHash(const FBucketKey& InKey) -> uint32
        {
            return HashCombine(
                HashCombine(GetTypeHash(InKey._Batch), GetTypeHash(InKey._ColorU32)),
                GetTypeHash(InKey._ColorClassIndex));
        }
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct FBucket
    {
        JPH::DebugRenderer::Batch _BatchKeepAlive;
        FLinearColor _BaseColor = FLinearColor::White;

        // STRONG: an actorless component has no owner to root it, and the debug draw runs in worlds (preview,
        // transient) that have no ObjectPooling subsystem to pin it either.
        TStrongObjectPtr<UInstancedStaticMeshComponent> _Ism;
        TWeakObjectPtr<UMaterialInstanceDynamic> _SolidMid;
        TWeakObjectPtr<UMaterialInstanceDynamic> _WireframeMid;

        // Persistent-slot reconcile (the only reconcile path); counts live instances added through AddInstanceById.
        int32 _SlotCount = 0;

        bool _IsmCreateFailed = false;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // One Jolt body maps to N instances — a compound shape emits one DrawGeometry per child.
    struct FBodySlot
    {
        FBucketKey _Bucket;
        FPrimitiveInstanceId _InstanceId;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // What the revision-keyed full pass last drew ONE inactive body as. A body whose pose, shape and colour
    // inputs all match its record is already on screen exactly as it would be re-drawn, so the pass skips it
    // entirely — that is what keeps a scene-revision bump O(changed) instead of O(all bodies). Pose is compared
    // exactly: any difference at all, however small, is a body that moved.
    //
    // The colour-class index rides along because it is what decides the bucket: a record that compared pose
    // alone would skip a body whose class had changed and leave it painted the old colour. It is the CHEAP
    // index — the one input deliberately excluded is BakedStatic attribution, which needs a registry lookup per
    // body and can only flip when the body's JoltStaticActor entity dies, which routes the body through the
    // static-revision funnel already.
    //
    // The map is rebuilt wholesale each pass, so a record is only ever compared against the SAME BodyID. A
    // recycled id landing on a body at an identical position, rotation, shape address and class would alias
    // onto the dead body's record and be skipped — accepted.
    struct FInactiveBodyRecord
    {
        JPH::RVec3 _Position = JPH::RVec3::sZero();
        JPH::Quat _Rotation = JPH::Quat::sIdentity();
        const JPH::Shape* _Shape = nullptr;
        uint8 _ColorClassIndex = 0;

        auto operator==(const FInactiveBodyRecord& InOther) const -> bool
        {
            return _Shape == InOther._Shape &&
                   _ColorClassIndex == InOther._ColorClassIndex &&
                   _Position == InOther._Position &&
                   _Rotation == InOther._Rotation;
        }
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Whether releasing a slot belongs to the capture the stats describe. A release driven from a consumer
    // (clearing a selection) happens BETWEEN captures, so counting it would rewrite the last capture's reported
    // numbers after the fact.
    enum class EStatCounting : uint8
    {
        Counted,
        Excluded
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Per-batch count of live buckets holding a keep-alive, across EVERY target. A batch is prunable only when
    // its refcount has fallen to exactly that count — i.e. no Jolt geometry references it any more. Owned by
    // the renderer TU but reached through free functions so a target destructing after the renderer is gone
    // (engine exit) still balances its bookkeeping.
    // Taken as the BASE ref type: the target translation unit only ever holds an erased FBatch and must not
    // need its definition to balance the census.
    CKJOLT_API auto Note_BucketHolderAdded(JPH::RefTargetVirtual* InBatch) -> void;
    CKJOLT_API auto Note_BucketHolderRemoved(JPH::RefTargetVirtual* InBatch) -> void;
    CKJOLT_API auto Get_BucketHolderCount(JPH::RefTargetVirtual* InBatch) -> int32;

    CKJOLT_API auto
    Get_TintedColor(
        const FLinearColor& InBaseColor,
        float InOpacity) -> FLinearColor;

    // Free rather than members so the public target header never has to name FBucket.
    CKJOLT_API auto
    Destroy_BucketIsm(
        FBucket& InOutBucket) -> void;

    /// The target's line component, created on first line and registered with its world. Null when the world is
    /// gone or the component could not be created (already ensured, once).
    CKJOLT_API auto
    TryEnsure_LineBatcher(
        FCk_Jolt_DebugDrawTarget::FImpl& InOutTargetImpl) -> ULineBatchComponent*;

    CKJOLT_API auto
    Destroy_LineBatcher(
        FCk_Jolt_DebugDrawTarget::FImpl& InOutTargetImpl) -> void;

    /// Drops every line the component holds and forgets this capture's JPH lines and labels. The retained
    /// External sub-channels are NOT touched — they are owned by their contributors.
    CKJOLT_API auto
    Reset_LineChannels(
        FCk_Jolt_DebugDrawTarget::FImpl& InOutTargetImpl) -> void;

    /// Pushes this capture's JPH lines plus every retained External sub-channel into the line component, in one
    /// DrawLines call rather than one MarkRenderStateDirty per line.
    CKJOLT_API auto
    Flush_LineChannels(
        FCk_Jolt_DebugDrawTarget::FImpl& InOutTargetImpl) -> void;

    /// Removes every instance held for ONE slot key and forgets it. Exactly one key: pairing a body with its
    /// highlight overlay is the caller's decision, because the capture's rebuild path releases a body's slots
    /// mid-draw and must not take the overlay with it.
    CKJOLT_API auto
    Release_SlotsForKey(
        FCk_Jolt_DebugDrawTarget::FImpl& InOutTargetImpl,
        uint64 InSlotKey,
        EStatCounting InStatCounting) -> void;

    /// Creates the mode's MID on first use and assigns it to material slot 0. InOutRenderMode is written back
    /// when the wireframe material is unavailable and the target degrades to Solid. The class index is what
    /// decides the alpha: Highlight is forced fully opaque and Hover half-transparent, both independent of the
    /// palette's own opacity, so a translucent population can never wash the selection out.
    CKJOLT_API auto
    Apply_BucketMaterial(
        FBucket& InOutBucket,
        const FCk_Jolt_DebugDrawPalette& InPalette,
        uint8 InColorClassIndex,
        ECk_Jolt_DebugDraw_RenderMode& InOutRenderMode) -> void;

    /// The alpha one class index draws at. Highlight ignores the palette opacity entirely (P5-D41) and Hover is
    /// pinned to half; every other class follows the palette.
    CKJOLT_API auto
    Get_ClassOpacity(
        uint8 InColorClassIndex,
        float InPaletteOpacity) -> float;

    /// Recomputes the process-wide contact-draw demand from every live target's flags and pushes it into Jolt's
    /// ContactConstraintManager statics. Called whenever a target is created, destroyed, or has its flags set.
    CKJOLT_API auto
    Recompute_ContactDrawDemand() -> void;

    /// The renderer's hook into the recorder: consumes one line while the record scope is open and reports
    /// whether it did. A no-op (and false) whenever nothing is being recorded, which is the whole capture path.
    CKJOLT_API auto
    TryRecord_ContactLine(
        const FVector& InFrom,
        const FVector& InTo,
        const FLinearColor& InColor) -> bool;
}

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Jolt_DebugDrawTarget::FImpl
{
    TWeakObjectPtr<UWorld> _World;

    TMap<ck::jolt::debug_draw::FBucketKey, ck::jolt::debug_draw::FBucket> _Buckets;

    // Persistent body -> instance slots (capture path). Keyed by BodyID::GetIndexAndSequenceNumber(), with
    // characters lifted clear of that keyspace (they have no BodyID).
    TMap<uint64, TArray<ck::jolt::debug_draw::FBodySlot>> _BodySlots;
    TSet<uint64> _StaticBodyKeys;
    TSet<uint64> _PrevActiveBodyKeys;
    TSet<uint64> _SleepingBodyKeys;
    TSet<uint64> _CharacterKeys;

    // Pose+shape of every body the last full pass drew, its change oracle for the next one.
    TMap<uint64, ck::jolt::debug_draw::FInactiveBodyRecord> _InactiveBodyRecords;

    // The selected bodies' keys in their OWN keyspace (plain body or character keys); each one's overlay slots
    // live under Make_HighlightKey of it, which is what lets N coexist. The FIRST is the primary — the only one
    // sampled and the only one whose contacts are queried. The hovered body is the same idea under
    // Make_HoverKey, and the two are independent — a body may be selected AND hovered.
    TArray<uint64> _HighlightedBodyKeys;
    TOptional<uint64> _HoveredBodyKey;

    // While non-empty, the capture draws ONLY these keys and releases every other body's instances.
    TSet<uint64> _IsolatedBodyKeys;

    // Sampled by the capture for the PRIMARY selection only, and re-sampled from scratch every capture: a value
    // the current capture did not produce would be reported as live state it no longer is.
    TOptional<FCk_Jolt_DebugDraw_BodySample> _BodySample;
    TOptional<FCk_Jolt_DebugDraw_CharacterSample> _CharacterSample;

    // Refilled every capture while contacts are wanted, from the primary selection's own shape query.
    TArray<FCk_Jolt_DebugDraw_ContactEntry> _SelectionContacts;

    // STRONG for the same reason the bucket ISMs are: an actorless component has no owner to root it, and the
    // preview/transient worlds this target binds to host no pooling subsystem either. Created on first line.
    TStrongObjectPtr<ULineBatchComponent> _Lines;

    // This capture's JPH line output, accumulated and pushed to the component in one DrawLines at EndCapture.
    TArray<FBatchedLine> _JphLines;

    TArray<FCk_Jolt_DebugDrawLabel> _Labels;

    // Written wholesale by Replay_RecordedContacts on the game thread, before the capture that flushes them.
    // Not cleared by Reset_LineChannels: the replay owns this channel and is the only thing that rewrites it.
    TArray<FBatchedLine> _ContactLines;

    // Object-layer display names, indexed by layer, published by the capture from the collision-layer table.
    // Only the ObjectLayer legend reads them.
    TArray<FString> _ObjectLayerNames;

    // RETAINED, and keyed by contributor: a capture re-emits these without clearing them, so a push made
    // between two captures is never dropped and never flickers. Only Clear_External empties one.
    TMap<FName, TArray<FBatchedLine>> _ExternalChannels;

    FCk_Jolt_DebugDrawPalette _Palette;
    ck::jolt::debug_draw::FDebugDrawStats _LastCaptureStats;

    ECk_Jolt_DebugDraw_RenderMode _RenderMode = ECk_Jolt_DebugDraw_RenderMode::Solid;

    ECk_Jolt_DebugDrawFlags _DrawFlags = ECk_Jolt_DebugDrawFlags::Shape;

    ECk_Jolt_DebugDrawColorMode _ColorMode = ECk_Jolt_DebugDrawColorMode::BodyClass;

    // One bit per colour-class INDEX. Visibility is a component-level flag, never a capture filter: a hidden
    // class keeps capturing so unhiding it is instant and its instances are never stale.
    uint64 _HiddenClassMask = 0;

    FDelegateHandle _WorldCleanupHandle;

    uint64 _CapturedStaticSceneRevision = 0;
    uint64 _CapturedBodyRemovedRevision = 0;
    float _AppliedOpacity = -1.0f;
    bool _LineBatcherCreateFailed = false;
    bool _FullPassEverRan = false;
    bool _SweepEverRan = false;
    bool _AnyLive = false;
    bool _IsDesired = false;
    bool _WantsSelectionContacts = false;
};

#endif

// --------------------------------------------------------------------------------------------------------------------
