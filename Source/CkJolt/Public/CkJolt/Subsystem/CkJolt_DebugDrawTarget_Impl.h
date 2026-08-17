#pragma once

// MODULE-INTERNAL: the Jolt-shaped guts behind FCk_Jolt_DebugDrawTarget's opaque impl. Only the three
// debug-draw translation units include it; CkJolt_DebugDrawTarget.h stays free of every JPH type so a
// presentation consumer can bind a target without seeing Jolt. (CkJolt keeps .cpp files under Public/, so
// path is not the privacy boundary here — inclusion is.)

#include "CkJolt/Subsystem/CkJolt_DebugDrawTarget.h"

#include <Containers/Map.h>
#include <Math/Transform.h>
#include <UObject/WeakObjectPtr.h>

#include <Jolt/Jolt.h>

#if JPH_DEBUG_RENDERER

#include "CkDebugScene/CkDebugScene_Target.h"

#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Renderer/DebugRenderer.h>

// --------------------------------------------------------------------------------------------------------------------

// ReSharper disable once CppInconsistentNaming
namespace JPH
{
    class Shape;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::debug_draw
{
    // The world-agnostic geometry cache entry that lazily owns one CkDebugScene mesh per unique Jolt geometry.
    // Defined in CkJolt_DebugRenderer.cpp because only the renderer builds and owns it.
    class FBatch;
    struct FBucketKey;

    CKJOLT_API auto Get_DebugSceneMesh(FBatch* InBatch) -> TSharedPtr<FCk_DebugScene_Mesh>;

    CKJOLT_API auto Make_DebugSceneAppearance(const FBucketKey& InKey,
        const FCk_Jolt_DebugDrawPalette& InPalette, ECk_Jolt_DebugDraw_RenderMode InRenderMode)
        -> FCk_DebugScene_Appearance;

    // ----------------------------------------------------------------------------------------------------------------

    // Bodies, characters and the three overlays all live in ONE slot map, so the five keyspaces have to be
    // disjoint. A BodyID key occupies the low 32 bits (index + sequence); a character has no BodyID, and each
    // overlay traces a key that is already taken, so all four are lifted clear by their own bit.
    constexpr uint64 CharacterKeyBit = uint64{1} << 40;
    constexpr uint64 HighlightKeyBit = uint64{1} << 41;
    constexpr uint64 HoverKeyBit = uint64{1} << 42;
    constexpr uint64 SensorContactKeyBit = uint64{1} << 43;

    // The overlays swell about the body's centre of mass so the traced shape is visible past the body's own
    // surface. Hover is the smaller of the two on purpose — a hover must never read as a selection.
    constexpr float HighlightOverlayScale = 1.03f;
    constexpr float HoverOverlayScale = 1.02f;
    constexpr float SensorContactOverlayScale = 1.015f;

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

    constexpr auto
        Make_SensorContactKey(
            uint64 InBodyKey)
        -> uint64
    {
        return SensorContactKeyBit | InBodyKey;
    }

    // ----------------------------------------------------------------------------------------------------------------

    // Zero asks Unreal's line batcher to use its default one-pixel thickness.
    constexpr float LineThickness = 0.0f;

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
        -> FCk_DebugScene_Line
    {
        return FCk_DebugScene_Line{InFrom, InTo, InColor, LineThickness};
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
        bool _IsSensor = false;

        auto operator==(const FBucketKey& InOther) const -> bool
        {
            return _Batch == InOther._Batch &&
                   _ColorU32 == InOther._ColorU32 &&
                   _ColorClassIndex == InOther._ColorClassIndex &&
                   _IsSensor == InOther._IsSensor;
        }

        friend auto GetTypeHash(const FBucketKey& InKey) -> uint32
        {
            return HashCombine(
                HashCombine(GetTypeHash(InKey._Batch), GetTypeHash(InKey._ColorU32)),
                HashCombine(GetTypeHash(InKey._ColorClassIndex), GetTypeHash(InKey._IsSensor)));
        }
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct FBucket
    {
        JPH::DebugRenderer::Batch _BatchKeepAlive;
        FCk_DebugScene_Appearance _Appearance;
        ECk_Jolt_DebugDraw_RenderMode _AppearanceRenderMode = ECk_Jolt_DebugDraw_RenderMode::Solid;
        float _AppearanceOpacity = -1.0f;
        int32 _SlotCount = 0;
        bool _HasAppearance = false;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // One Jolt body maps to N instances — a compound shape emits one DrawGeometry per child.
    struct FBodySlot
    {
        FBucketKey _Bucket;
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
        bool _IsSensor = false;
        bool _HasSensorContact = false;

        auto operator==(const FInactiveBodyRecord& InOther) const -> bool
        {
            return _Shape == InOther._Shape &&
                   _ColorClassIndex == InOther._ColorClassIndex &&
                   _IsSensor == InOther._IsSensor &&
                   _HasSensorContact == InOther._HasSensorContact &&
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
    Release_Bucket(
        FBucket& InOutBucket) -> void;

    /// Drops every line the component holds and forgets this capture's JPH lines and labels. The retained
    /// External sub-channels are NOT touched — they are owned by their contributors.
    CKJOLT_API auto
    Reset_LineChannels(
        FCk_Jolt_DebugDrawTarget::FImpl& InOutTargetImpl) -> void;

    /// Publishes this capture's JPH lines, contacts, labels, and retained External sub-channels through named
    /// CkDebugScene channels.
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
        EStatCounting InStatCounting,
        bool InRemoveSceneItem = true) -> void;

    /// The alpha one bucket draws at. Sensors stay transparent in every colour mode, including their overlays.
    CKJOLT_API auto
    Get_ClassOpacity(
        uint8 InColorClassIndex,
        bool InIsSensor,
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
    TSharedPtr<FCk_DebugScene_Target> _SceneTarget;

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

    // The same keys as a SET, kept in lockstep with the array by Set_HighlightedBodies. The array carries the
    // order (the primary is its first element); every membership test in a per-body loop goes through this,
    // because a linear scan per body is O(selection) on a walk that is already O(all bodies).
    TSet<uint64> _HighlightedBodyKeySet;

    TOptional<uint64> _HoveredBodyKey;

    // While non-empty, the capture draws ONLY these keys and releases every other body's instances.
    TSet<uint64> _IsolatedBodyKeys;

    // Bodies the facility itself owns (the drag anchor). Never drawn, never picked — unlike isolation, this is not
    // a view filter a user can turn off.
    TSet<uint64> _InternalBodyKeys;

    // Jolt-generic contact state published by FJoltWorld. A dedicated overlay reads this set; the base body class
    // and its colour-mode bucket never change when a contact begins or ends.
    TSet<uint64> _SensorContactBodyKeys;

    // Sampled by the capture for the PRIMARY selection only, and re-sampled from scratch every capture: a value
    // the current capture did not produce would be reported as live state it no longer is.
    TOptional<FCk_Jolt_DebugDraw_BodySample> _BodySample;
    TOptional<FCk_Jolt_DebugDraw_CharacterSample> _CharacterSample;

    // Refilled every capture while contacts are wanted, from the primary selection's own shape query.
    TArray<FCk_Jolt_DebugDraw_ContactEntry> _SelectionContacts;

    // The health scan's arming and its verdict (P8-D57). Unset thresholds mean the scan does not run at all,
    // and the map is refilled from scratch by every capture that does run it — a stale flag is a lie about
    // live state, so nothing here is ever carried forward.
    TOptional<FCk_Jolt_DebugDraw_ProblemThresholds> _ProblemThresholds;
    TMap<uint64, ECk_Jolt_DebugDraw_ProblemFlags> _ProblemBodies;

    // Compatibility mirrors retained for the Jolt public inspection API; rendering is published through named
    // CkDebugScene channels at EndCapture.
    TArray<FCk_DebugScene_Line> _JphLines;

    TArray<FCk_Jolt_DebugDrawLabel> _Labels;

    // Written wholesale by Replay_RecordedContacts on the game thread, before the capture that flushes them.
    // Not cleared by Reset_LineChannels: the replay owns this channel and is the only thing that rewrites it.
    TArray<FCk_DebugScene_Line> _ContactLines;

    // Object-layer display names, indexed by layer, published by the capture from the collision-layer table.
    // Only the ObjectLayer legend reads them.
    TArray<FString> _ObjectLayerNames;

    // RETAINED, and keyed by contributor: a capture re-emits these without clearing them, so a push made
    // between two captures is never dropped and never flickers. Only Clear_External empties one.
    TMap<FName, TArray<FCk_DebugScene_Line>> _ExternalChannels;

    FCk_Jolt_DebugDrawPalette _Palette;
    ck::jolt::debug_draw::FDebugDrawStats _LastCaptureStats;
    FCk_Jolt_DebugDraw_WorldStats _WorldStats;

    // Captures since the expensive half of _WorldStats was refreshed; mirrored into the struct's _SampleAge so a
    // consumer can see the staleness without knowing the cadence.
    int32 _CapturesSinceWorldStatsSample = 0;

    ECk_Jolt_DebugDraw_RenderMode _RenderMode = ECk_Jolt_DebugDraw_RenderMode::Solid;

    ECk_Jolt_DebugDrawFlags _DrawFlags = ECk_Jolt_DebugDrawFlags::Shape;

    ECk_Jolt_DebugDrawColorMode _ColorMode = ECk_Jolt_DebugDrawColorMode::BodyClass;

    float _DirectionGlyphScale = 1.0f;

    // One bit per colour-class INDEX. Visibility is target state, never a capture filter: a hidden
    // class keeps capturing so unhiding it is instant and its instances are never stale.
    uint64 _HiddenClassMask = 0;

    FDelegateHandle _WorldCleanupHandle;

    uint64 _CapturedStaticSceneRevision = 0;
    uint64 _CapturedBodyRemovedRevision = 0;
    float _AppliedOpacity = -1.0f;
    bool _FullPassEverRan = false;
    bool _SweepEverRan = false;
    bool _AnyLive = false;
    bool _IsDesired = false;
    bool _WantsSelectionContacts = false;
};

#endif

// --------------------------------------------------------------------------------------------------------------------
