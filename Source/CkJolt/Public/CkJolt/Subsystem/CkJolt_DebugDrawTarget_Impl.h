#pragma once

// MODULE-INTERNAL: the Jolt-shaped guts behind FCk_Jolt_DebugDrawTarget's opaque impl. Only the three
// debug-draw translation units include it; CkJolt_DebugDrawTarget.h stays free of every JPH type so a
// presentation consumer can bind a target without seeing Jolt. (CkJolt keeps .cpp files under Public/, so
// path is not the privacy boundary here — inclusion is.)

#include "CkJolt/Subsystem/CkJolt_DebugDrawTarget.h"

#include <Containers/Map.h>
#include <InstanceDataTypes.h>
#include <Math/Transform.h>
#include <UObject/StrongObjectPtr.h>
#include <UObject/WeakObjectPtr.h>

#include <Jolt/Jolt.h>

#if JPH_DEBUG_RENDERER

#include <Jolt/Renderer/DebugRenderer.h>

// --------------------------------------------------------------------------------------------------------------------

class UMaterialInstanceDynamic;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::debug_draw
{
    // The world-agnostic geometry cache entry: one transient UStaticMesh per unique Jolt geometry. Defined in
    // CkJolt_DebugRenderer.cpp because only the renderer builds and owns them.
    class FBatch;

    // ----------------------------------------------------------------------------------------------------------------

    // Bodies, characters and selection overlays all live in ONE slot map, so the three keyspaces have to be
    // disjoint. A BodyID key occupies the low 32 bits (index + sequence); a character has no BodyID, and a
    // highlight overlay traces a key that is already taken, so both are lifted clear by their own bit.
    constexpr uint64 CharacterKeyBit = uint64{1} << 40;
    constexpr uint64 HighlightKeyBit = uint64{1} << 41;

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

    // ----------------------------------------------------------------------------------------------------------------

    // Colour class rides the key alongside the packed colour: two classes whose palette entries quantise to the
    // same 8-bit colour must still land in distinct buckets, or the bucket's reported class is a coin flip.
    struct FBucketKey
    {
        FBatch* _Batch = nullptr;
        uint32 _ColorU32 = 0;
        ECk_Jolt_DebugDraw_ColorClass _ColorClass = ECk_Jolt_DebugDraw_ColorClass::Static;

        auto operator==(const FBucketKey& InOther) const -> bool
        {
            return _Batch == InOther._Batch &&
                   _ColorU32 == InOther._ColorU32 &&
                   _ColorClass == InOther._ColorClass;
        }

        friend auto GetTypeHash(const FBucketKey& InKey) -> uint32
        {
            return HashCombine(
                HashCombine(GetTypeHash(InKey._Batch), GetTypeHash(InKey._ColorU32)),
                GetTypeHash(static_cast<uint8>(InKey._ColorClass)));
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

        // Whole-array reconcile (the subsystem's in-world DrawBodies path).
        TArray<FTransform> _Desired;
        TArray<FTransform> _Applied;

        // Persistent-slot reconcile (the capture path); counts live instances added through AddInstanceById.
        int32 _SlotCount = 0;

        bool _Touched = false;
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

    /// Removes every instance held for ONE slot key and forgets it. Exactly one key: pairing a body with its
    /// highlight overlay is the caller's decision, because the capture's rebuild path releases a body's slots
    /// mid-draw and must not take the overlay with it.
    CKJOLT_API auto
    Release_SlotsForKey(
        FCk_Jolt_DebugDrawTarget::FImpl& InOutTargetImpl,
        uint64 InSlotKey) -> void;

    /// Creates the mode's MID on first use and assigns it to material slot 0. InOutRenderMode is written back
    /// when the wireframe material is unavailable and the target degrades to Solid.
    CKJOLT_API auto
    Apply_BucketMaterial(
        FBucket& InOutBucket,
        const FCk_Jolt_DebugDrawPalette& InPalette,
        ECk_Jolt_DebugDraw_RenderMode& InOutRenderMode) -> void;
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

    // The selected body's key in its OWN keyspace (a plain body or character key); its overlay slots live
    // under Make_HighlightKey of it.
    TOptional<uint64> _HighlightedBodyKey;

    // Sampled by the capture for the highlighted RIGID body only, and re-sampled from scratch every capture:
    // a value the current capture did not produce would be reported as live state it no longer is.
    TOptional<FVector> _HighlightedBodyLinearVelocity;

    FCk_Jolt_DebugDrawPalette _Palette;
    ck::jolt::debug_draw::FDebugDrawStats _LastCaptureStats;

    ECk_Jolt_DebugDraw_RenderMode _RenderMode = ECk_Jolt_DebugDraw_RenderMode::Solid;

    // One bit per colour class. Visibility is a component-level flag, never a capture filter: a hidden class
    // keeps capturing so unhiding it is instant and its instances are never stale.
    uint8 _HiddenClassMask = 0;

    FDelegateHandle _WorldCleanupHandle;

    uint64 _CapturedStaticSceneRevision = 0;
    float _AppliedOpacity = -1.0f;
    bool _FullPassEverRan = false;
    bool _AnyLive = false;
    bool _IsDesired = false;
};

#endif

// --------------------------------------------------------------------------------------------------------------------
