#include "CkJolt_DebugDrawTarget.h"

#include <Jolt/Jolt.h>

#if JPH_DEBUG_RENDERER

#include "CkJolt/Subsystem/CkJolt_DebugDrawTarget_Impl.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat_Defaults.h"
#include "CkCore/Validation/CkIsValid_Defaults.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkJolt/CkJolt_Log.h"

#include <Components/InstancedStaticMeshComponent.h>
#include <Components/LineBatchComponent.h>
#include <Engine/StaticMesh.h>
#include <Engine/World.h>
#include <HAL/CriticalSection.h>
#include <Materials/Material.h>
#include <Materials/MaterialInstanceDynamic.h>
#include <Misc/ScopeLock.h>
#include <UObject/StrongObjectPtr.h>

#include <Jolt/Physics/Constraints/ContactConstraintManager.h>

#include <atomic>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_debug_draw_target
{
    const auto ColorParameterName = FName{TEXT("Color")};

    static_assert(static_cast<int32>(ECk_Jolt_DebugDraw_ColorClass::Count) <=
        ck::jolt::debug_draw::HighlightClassIndex,
        "BodyClass mode's classes must fit UNDER the two reserved mode-independent indices, or a body would be "
        "drawn in the Highlight bucket and become unhideable.");

    static_assert(ck::jolt::debug_draw::HoverClassIndex < ck::jolt::debug_draw::MaxColorClasses,
        "The hidden-class mask is a uint64 bitfield. A class index of 64 or more shifts the bit out of range and "
        "silently makes that class permanently visible — widen _HiddenClassMask before adding one.");

    auto
        Get_ClassBit(
            uint8 InColorClassIndex)
        -> uint64
    {
        return uint64{1} << InColorClassIndex;
    }

    /*
     * The shared distinct-colour wheel every "identity" mode draws from — island, object layer and shape type
     * all colour by a number whose value carries no meaning beyond "not the same as its neighbour". Sixteen
     * entries, hand-picked to stay apart at the debug draw's default half opacity rather than generated, so two
     * adjacent layers are never two shades of the same hue.
     */
    auto
        Get_DistinctColor(
            uint8 InIndex)
        -> FLinearColor
    {
        static const FLinearColor Colors[ck::jolt::debug_draw::DistinctColorPaletteSize] =
        {
            FLinearColor{0.90f, 0.25f, 0.25f, 1.0f}, FLinearColor{0.25f, 0.70f, 0.95f, 1.0f},
            FLinearColor{0.35f, 0.85f, 0.35f, 1.0f}, FLinearColor{0.95f, 0.75f, 0.20f, 1.0f},
            FLinearColor{0.80f, 0.35f, 0.90f, 1.0f}, FLinearColor{0.20f, 0.85f, 0.80f, 1.0f},
            FLinearColor{0.95f, 0.50f, 0.20f, 1.0f}, FLinearColor{0.55f, 0.60f, 0.95f, 1.0f},
            FLinearColor{0.70f, 0.90f, 0.25f, 1.0f}, FLinearColor{0.95f, 0.40f, 0.60f, 1.0f},
            FLinearColor{0.30f, 0.55f, 0.45f, 1.0f}, FLinearColor{0.85f, 0.85f, 0.85f, 1.0f},
            FLinearColor{0.60f, 0.40f, 0.20f, 1.0f}, FLinearColor{0.45f, 0.30f, 0.75f, 1.0f},
            FLinearColor{0.20f, 0.45f, 0.85f, 1.0f}, FLinearColor{0.75f, 0.95f, 0.75f, 1.0f},
        };

        return Colors[InIndex % ck::jolt::debug_draw::DistinctColorPaletteSize];
    }

    auto
        Get_BodyClassName(
            uint8 InClassIndex)
        -> FText
    {
        switch (static_cast<ECk_Jolt_DebugDraw_ColorClass>(InClassIndex))
        {
            case ECk_Jolt_DebugDraw_ColorClass::Static:           return FText::FromString(TEXT("Static"));
            case ECk_Jolt_DebugDraw_ColorClass::Kinematic:        return FText::FromString(TEXT("Kinematic"));
            case ECk_Jolt_DebugDraw_ColorClass::Dynamic_Awake:    return FText::FromString(TEXT("Awake"));
            case ECk_Jolt_DebugDraw_ColorClass::Dynamic_Sleeping: return FText::FromString(TEXT("Asleep"));
            case ECk_Jolt_DebugDraw_ColorClass::Sensor:           return FText::FromString(TEXT("Sensor"));
            case ECk_Jolt_DebugDraw_ColorClass::BakedStatic:      return FText::FromString(TEXT("Baked Static"));
            case ECk_Jolt_DebugDraw_ColorClass::Character:        return FText::FromString(TEXT("Character"));
            default:                                              return FText::FromString(TEXT("Unknown"));
        }
    }

    auto
        Get_SleepStateName(
            uint8 InClassIndex)
        -> FText
    {
        switch (static_cast<ck::jolt::debug_draw::ESleepStateClass>(InClassIndex))
        {
            case ck::jolt::debug_draw::ESleepStateClass::Static:    return FText::FromString(TEXT("Static"));
            case ck::jolt::debug_draw::ESleepStateClass::Kinematic: return FText::FromString(TEXT("Kinematic"));
            case ck::jolt::debug_draw::ESleepStateClass::Awake:     return FText::FromString(TEXT("Awake"));
            case ck::jolt::debug_draw::ESleepStateClass::Asleep:    return FText::FromString(TEXT("Asleep"));
            default:                                               return FText::FromString(TEXT("Unknown"));
        }
    }

    auto
        Get_ShapeTypeName(
            uint8 InClassIndex)
        -> FText
    {
        switch (static_cast<ck::jolt::debug_draw::EShapeTypeClass>(InClassIndex))
        {
            case ck::jolt::debug_draw::EShapeTypeClass::Sphere:             return FText::FromString(TEXT("Sphere"));
            case ck::jolt::debug_draw::EShapeTypeClass::Box:                return FText::FromString(TEXT("Box"));
            case ck::jolt::debug_draw::EShapeTypeClass::Triangle:           return FText::FromString(TEXT("Triangle"));
            case ck::jolt::debug_draw::EShapeTypeClass::Capsule:            return FText::FromString(TEXT("Capsule"));
            case ck::jolt::debug_draw::EShapeTypeClass::TaperedCapsule:     return FText::FromString(TEXT("Tapered Capsule"));
            case ck::jolt::debug_draw::EShapeTypeClass::Cylinder:           return FText::FromString(TEXT("Cylinder"));
            case ck::jolt::debug_draw::EShapeTypeClass::TaperedCylinder:    return FText::FromString(TEXT("Tapered Cylinder"));
            case ck::jolt::debug_draw::EShapeTypeClass::ConvexHull:         return FText::FromString(TEXT("Convex Hull"));
            case ck::jolt::debug_draw::EShapeTypeClass::StaticCompound:     return FText::FromString(TEXT("Static Compound"));
            case ck::jolt::debug_draw::EShapeTypeClass::MutableCompound:    return FText::FromString(TEXT("Mutable Compound"));
            case ck::jolt::debug_draw::EShapeTypeClass::RotatedTranslated:  return FText::FromString(TEXT("Rotated/Translated"));
            case ck::jolt::debug_draw::EShapeTypeClass::Scaled:             return FText::FromString(TEXT("Scaled"));
            case ck::jolt::debug_draw::EShapeTypeClass::OffsetCenterOfMass: return FText::FromString(TEXT("Offset COM"));
            case ck::jolt::debug_draw::EShapeTypeClass::Mesh:               return FText::FromString(TEXT("Mesh"));
            case ck::jolt::debug_draw::EShapeTypeClass::HeightField:        return FText::FromString(TEXT("Height Field"));
            case ck::jolt::debug_draw::EShapeTypeClass::SoftBody:           return FText::FromString(TEXT("Soft Body"));
            case ck::jolt::debug_draw::EShapeTypeClass::Plane:              return FText::FromString(TEXT("Plane"));
            case ck::jolt::debug_draw::EShapeTypeClass::Empty:              return FText::FromString(TEXT("Empty"));
            default:                                                        return FText::FromString(TEXT("Other"));
        }
    }

    // Rooted, not a bare static UMaterial*: a function-local raw pointer survives the GC that collects the
    // material it points at, and the next dereference is on freed memory.
    // Loaded directly rather than through GEngine->WireframeMaterial, which is null whenever the platform
    // RequiresCookedData. Both engine debug materials are special-engine materials, so the ISM usage checks
    // do not reject them.
    auto
        Get_SolidBaseMaterial()
        -> UMaterial*
    {
        static auto Material = TStrongObjectPtr<UMaterial>{LoadObject<UMaterial>(
            nullptr,
            TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"))};

        return Material.Get();
    }

    auto
        Get_WireframeBaseMaterial()
        -> UMaterial*
    {
        static auto Material = TStrongObjectPtr<UMaterial>{LoadObject<UMaterial>(
            nullptr,
            TEXT("/Engine/EngineDebugMaterials/WireframeMaterial.WireframeMaterial"))};

        return Material.Get();
    }

    // Where one instance sits and how big its geometry is, in the instance's OWN space. Keeping the box local
    // and the transform beside it is what lets the pick test be an oriented-box test instead of a world AABB
    // one, at no extra cost.
    struct FInstancePlacement
    {
        FTransform _Transform;
        FBox _LocalBounds = FBox{ForceInit};
    };

    auto
        TryGet_InstancePlacement(
            const TMap<ck::jolt::debug_draw::FBucketKey, ck::jolt::debug_draw::FBucket>& InBuckets,
            const ck::jolt::debug_draw::FBodySlot& InSlot)
        -> TOptional<FInstancePlacement>
    {
        const auto* Bucket = InBuckets.Find(InSlot._Bucket);
        if (Bucket == nullptr)
        { return {}; }

        auto* Ism = Bucket->_Ism.Get();
        if (ck::Is_NOT_Valid(Ism) || NOT Ism->IsValidId(InSlot._InstanceId))
        { return {}; }

        const UStaticMesh* Mesh = Ism->GetStaticMesh();
        if (ck::Is_NOT_Valid(Mesh))
        { return {}; }

        const auto InstanceIndex = Ism->GetInstanceIndexForId(InSlot._InstanceId);
        if (InstanceIndex == INDEX_NONE)
        { return {}; }

        auto Placement = FInstancePlacement{};
        Placement._LocalBounds = Mesh->GetBounds().GetBox();

        constexpr auto WorldSpace = true;
        if (NOT Ism->GetInstanceTransform(InstanceIndex, Placement._Transform, WorldSpace))
        { return {}; }

        return Placement;
    }

    // Slab test. The returned distance is PARAMETRIC along InDirection, which is all a nearest-hit comparison
    // needs and spares every caller a normalize. A ray whose origin is already inside the box hits at 0.
    auto
        TryIntersect_RayBox(
            const FVector& InOrigin,
            const FVector& InDirection,
            const FBox& InBox)
        -> TOptional<double>
    {
        auto EntryDistance = 0.0;
        auto ExitDistance = TNumericLimits<double>::Max();

        for (auto Axis = 0; Axis < 3; ++Axis)
        {
            const auto AxisDirection = InDirection[Axis];
            const auto AxisOrigin = InOrigin[Axis];

            if (FMath::IsNearlyZero(AxisDirection))
            {
                if (AxisOrigin < InBox.Min[Axis] || AxisOrigin > InBox.Max[Axis])
                { return {}; }

                continue;
            }

            const auto InverseDirection = 1.0 / AxisDirection;

            auto NearDistance = (InBox.Min[Axis] - AxisOrigin) * InverseDirection;
            auto FarDistance = (InBox.Max[Axis] - AxisOrigin) * InverseDirection;

            if (NearDistance > FarDistance)
            { Swap(NearDistance, FarDistance); }

            EntryDistance = FMath::Max(EntryDistance, NearDistance);
            ExitDistance = FMath::Min(ExitDistance, FarDistance);

            if (EntryDistance > ExitDistance)
            { return {}; }
        }

        return EntryDistance;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Contact recorder state. Jolt's contact draw switches are process-wide statics with no per-PhysicsSystem
    // variant, so the DEMAND is the union over every live target; the record BUFFERS are per world, because a
    // PIE session runs N of them and one world's manifolds replayed into another world's targets is a lie.
    // ----------------------------------------------------------------------------------------------------------------

    // Every constructed target, not only the ones registered with a subsystem: the subsystem's own in-world
    // default target is never "registered", and its contact flags count towards the union just the same.
    static TSet<FCk_Jolt_DebugDrawTarget*> GLiveTargets;

    struct FContactRecordBuffers
    {
        // Appended from Jolt worker threads during the solve; swapped into _Ready whole at End_ContactRecord.
        TArray<FBatchedLine> _Recording;
        TArray<FBatchedLine> _Ready;
    };

    static FCriticalSection GContactRecordLock;
    static TMap<const JPH::PhysicsSystem*, FContactRecordBuffers> GContactRecordBuffers;

    /*
     * The world whose solve currently owns the recorder, or null. Read on Jolt worker threads from inside
     * DrawLine; written by Begin/End_ContactRecord.
     *
     * ONE world at a time, by compare-exchange: a DrawLine carries no world, so two overlapping solves cannot be
     * told apart from inside the renderer. A second world's Begin therefore SKIPS its own record for that step
     * rather than letting its targets be fed a mixed one (P5-D64/F3).
     */
    static std::atomic<const JPH::PhysicsSystem*> GRecordingWorld{nullptr};
    static std::atomic<bool> GContactDemand{false};

    // One frame of a 100k-body pile can emit far more contact lines than a line batcher can carry, and the
    // failure mode is a hitch rather than a message. Cap and say so once.
    constexpr int32 MaxContactLinesPerFrame = 200000;
    static bool GContactCapWarned = false;
    static bool GConcurrentRecordSkipLogged = false;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::debug_draw
{
    auto
        Get_TintedColor(
            const FLinearColor& InBaseColor,
            float InOpacity)
        -> FLinearColor
    {
        auto Color = InBaseColor;
        Color.A = FMath::Clamp(InOpacity, 0.0f, 1.0f);
        return Color;
    }

    auto
        Make_BodyKey(
            uint32 InIndexAndSequenceNumber)
        -> uint64
    {
        return static_cast<uint64>(InIndexAndSequenceNumber);
    }

    auto
        Make_CharacterBodyKey(
            const FCk_Handle& InCharacterEntity)
        -> uint64
    {
        if (ck::Is_NOT_Valid(InCharacterEntity))
        { return 0; }

        return Make_CharacterBodyKey_FromEntityId(
            static_cast<uint64>(InCharacterEntity.Get_Entity().Get_ID()));
    }

    auto
        Get_ClassOpacity(
            uint8 InColorClassIndex,
            float InPaletteOpacity)
        -> float
    {
        if (InColorClassIndex == HighlightClassIndex)
        { return 1.0f; }

        if (InColorClassIndex == HoverClassIndex)
        { return 0.5f; }

        return InPaletteOpacity;
    }

    auto
        Recompute_ContactDrawDemand()
        -> void
    {
        auto WantsPoints = false;
        auto WantsNormals = false;
        auto WantsFaces = false;

        for (const auto* Target : ck_jolt_debug_draw_target::GLiveTargets)
        {
            const auto Flags = Target->Get_DrawFlags();

            WantsPoints |= EnumHasAnyFlags(Flags, ECk_Jolt_DebugDrawFlags::ContactPoints);
            WantsNormals |= EnumHasAnyFlags(Flags, ECk_Jolt_DebugDrawFlags::ContactNormals);
            WantsFaces |= EnumHasAnyFlags(Flags, ECk_Jolt_DebugDrawFlags::SupportingFaces);
        }

        // Process-wide by Jolt's own design (plain statics on ContactConstraintManager), so this is the ONE
        // draw flag a target cannot own privately — disclosed in CkJolt/Claude.md rather than papered over.
        JPH::ContactConstraintManager::sDrawContactPoint = WantsPoints;
        JPH::ContactConstraintManager::sDrawContactManifolds = WantsNormals;
        JPH::ContactConstraintManager::sDrawSupportingFaces = WantsFaces;

        const auto AnyDemand = WantsPoints || WantsNormals || WantsFaces;
        ck_jolt_debug_draw_target::GContactDemand.store(AnyDemand, std::memory_order_release);

        if (AnyDemand)
        { return; }

        // Nothing wants contacts any more: drop whatever is still buffered, for every world, so a later
        // re-enable never replays a record from a step that has long since been superseded.
        auto Lock = FScopeLock{&ck_jolt_debug_draw_target::GContactRecordLock};
        ck_jolt_debug_draw_target::GContactRecordBuffers.Reset();
    }

    auto
        Get_IsAnyTargetDemandingContacts()
        -> bool
    {
        return ck_jolt_debug_draw_target::GContactDemand.load(std::memory_order_acquire);
    }

    auto
        Begin_ContactRecord(
            const JPH::PhysicsSystem* InPhysicsSystem)
        -> void
    {
        if (NOT Get_IsAnyTargetDemandingContacts())
        { return; }

        auto NoWorldRecording = static_cast<const JPH::PhysicsSystem*>(nullptr);

        if (NOT ck_jolt_debug_draw_target::GRecordingWorld.compare_exchange_strong(NoWorldRecording,
            InPhysicsSystem, std::memory_order_acq_rel, std::memory_order_acquire))
        {
            // Another world's solve owns the recorder for this step. Its lines and ours are indistinguishable
            // inside DrawLine, so this world records nothing rather than showing the other one's manifolds.
            if (NOT ck_jolt_debug_draw_target::GConcurrentRecordSkipLogged)
            {
                ck_jolt_debug_draw_target::GConcurrentRecordSkipLogged = true;
                ck::jolt::Verbose(TEXT("Jolt debug-draw contact recording SKIPPED for a world whose solve "
                    "overlapped another recording world's. Contacts are process-wide in Jolt, so only one world "
                    "may record per step."));
            }

            return;
        }

        auto Lock = FScopeLock{&ck_jolt_debug_draw_target::GContactRecordLock};
        ck_jolt_debug_draw_target::GContactRecordBuffers.FindOrAdd(InPhysicsSystem)._Recording.Reset();
    }

    auto
        End_ContactRecord(
            const JPH::PhysicsSystem* InPhysicsSystem)
        -> void
    {
        auto ThisWorld = InPhysicsSystem;

        // Only the world that WON the Begin closes the scope; a skipped world's End must not release someone
        // else's ownership, and must not publish the record it never filled.
        if (NOT ck_jolt_debug_draw_target::GRecordingWorld.compare_exchange_strong(ThisWorld, nullptr,
            std::memory_order_acq_rel, std::memory_order_acquire))
        { return; }

        // Double-buffered: the filled buffer is swapped aside whole, so the game thread's replay never contends
        // with the next step's appends and never sees a half-written frame.
        auto Lock = FScopeLock{&ck_jolt_debug_draw_target::GContactRecordLock};

        auto& Buffers = ck_jolt_debug_draw_target::GContactRecordBuffers.FindOrAdd(InPhysicsSystem);
        Swap(Buffers._Ready, Buffers._Recording);
        Buffers._Recording.Reset();
    }

    auto
        Forget_ContactRecord(
            const JPH::PhysicsSystem* InPhysicsSystem)
        -> void
    {
        auto ThisWorld = InPhysicsSystem;
        ck_jolt_debug_draw_target::GRecordingWorld.compare_exchange_strong(ThisWorld, nullptr,
            std::memory_order_acq_rel, std::memory_order_acquire);

        auto Lock = FScopeLock{&ck_jolt_debug_draw_target::GContactRecordLock};
        ck_jolt_debug_draw_target::GContactRecordBuffers.Remove(InPhysicsSystem);
    }

    auto
        TryRecord_ContactLine(
            const FVector& InFrom,
            const FVector& InTo,
            const FLinearColor& InColor)
        -> bool
    {
        const auto* RecordingWorld = ck_jolt_debug_draw_target::GRecordingWorld.load(std::memory_order_acquire);

        if (RecordingWorld == nullptr)
        { return false; }

        // Jolt's solve is multi-threaded whenever jolt.EnableParallelPhysics is on, so every append is guarded.
        auto Lock = FScopeLock{&ck_jolt_debug_draw_target::GContactRecordLock};

        auto& Recording = ck_jolt_debug_draw_target::GContactRecordBuffers.FindOrAdd(RecordingWorld)._Recording;

        if (Recording.Num() >= ck_jolt_debug_draw_target::MaxContactLinesPerFrame)
        {
            if (NOT ck_jolt_debug_draw_target::GContactCapWarned)
            {
                ck_jolt_debug_draw_target::GContactCapWarned = true;
                ck::jolt::Display(TEXT("Jolt debug-draw contact recording hit its [{}] lines/frame cap — the "
                    "rest of this frame's contacts are dropped. Isolate or reduce the bodies in contact."),
                    ck_jolt_debug_draw_target::MaxContactLinesPerFrame);
            }

            return true;
        }

        Recording.Emplace(Make_DebugDrawLine(InFrom, InTo, InColor));
        return true;
    }

    auto
        Replay_RecordedContacts(
            const JPH::PhysicsSystem* InPhysicsSystem,
            TArrayView<const TSharedPtr<FCk_Jolt_DebugDrawTarget>> InTargets)
        -> void
    {
        auto Recorded = TArray<FBatchedLine>{};

        {
            auto Lock = FScopeLock{&ck_jolt_debug_draw_target::GContactRecordLock};

            if (auto* Buffers = ck_jolt_debug_draw_target::GContactRecordBuffers.Find(InPhysicsSystem))
            { Recorded = MoveTemp(Buffers->_Ready); }
        }

        // The LAST target that wants the record takes it by move; everyone before it copies. At the contact
        // counts this can reach, a deep copy per target is the whole cost of the replay.
        auto LastDemandingIndex = int32{INDEX_NONE};

        const auto& Get_WantsContacts = [](const TSharedPtr<FCk_Jolt_DebugDrawTarget>& InTarget) -> bool
        {
            return InTarget.IsValid() && InTarget->Get_IsDrawFlagSet(
                ECk_Jolt_DebugDrawFlags::ContactPoints |
                ECk_Jolt_DebugDrawFlags::ContactNormals |
                ECk_Jolt_DebugDrawFlags::SupportingFaces);
        };

        for (auto Index = 0; Index < InTargets.Num(); ++Index)
        {
            if (Get_WantsContacts(InTargets[Index]))
            { LastDemandingIndex = Index; }
        }

        for (auto Index = 0; Index < InTargets.Num(); ++Index)
        {
            const auto& Target = InTargets[Index];

            if (NOT Target.IsValid())
            { continue; }

            // Assigned, never appended: a target keeps the contacts of the LAST step until this replay writes
            // the next ones, and a target that stops asking is emptied rather than left showing stale geometry.
            if (NOT Get_WantsContacts(Target))
            {
                Target->_Impl->_ContactLines.Reset();
                continue;
            }

            // Spelled as a branch, not a ternary: a `cond ? MoveTemp(X) : X` collapses to a common VALUE type
            // and copies on both arms, which is the exact copy this is here to avoid.
            if (Index == LastDemandingIndex)
            { Target->_Impl->_ContactLines = MoveTemp(Recorded); }
            else
            { Target->_Impl->_ContactLines = Recorded; }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_DebugDrawPalette::
    Get_Color(
        ECk_Jolt_DebugDrawColorMode InColorMode,
        uint8 InClassIndex) const
    -> FLinearColor
{
    // Mode-independent, and answered before the mode is even looked at: a selection must read the same whatever
    // the bodies around it are coloured by.
    if (InClassIndex == ck::jolt::debug_draw::HighlightClassIndex ||
        InClassIndex == ck::jolt::debug_draw::HoverClassIndex)
    { return _HighlightColor; }

    const auto& Get_SleepingColor = [this]() -> FLinearColor
    {
        const auto Dim = FMath::Clamp(_SleepingDimFactor, 0.0f, 1.0f);
        return FLinearColor{
            _DynamicSleepingColor.R * Dim,
            _DynamicSleepingColor.G * Dim,
            _DynamicSleepingColor.B * Dim,
            _DynamicSleepingColor.A};
    };

    switch (InColorMode)
    {
        case ECk_Jolt_DebugDrawColorMode::BodyClass:
        {
            switch (static_cast<ECk_Jolt_DebugDraw_ColorClass>(InClassIndex))
            {
                case ECk_Jolt_DebugDraw_ColorClass::Static:           return _StaticColor;
                case ECk_Jolt_DebugDraw_ColorClass::Kinematic:        return _KinematicColor;
                case ECk_Jolt_DebugDraw_ColorClass::Dynamic_Awake:    return _DynamicAwakeColor;
                case ECk_Jolt_DebugDraw_ColorClass::Dynamic_Sleeping: return Get_SleepingColor();
                case ECk_Jolt_DebugDraw_ColorClass::Sensor:           return _SensorColor;
                case ECk_Jolt_DebugDraw_ColorClass::BakedStatic:      return _BakedStaticColor;
                case ECk_Jolt_DebugDraw_ColorClass::Character:        return _CharacterColor;
                default:                                              return _StaticColor;
            }
        }

        case ECk_Jolt_DebugDrawColorMode::SleepState:
        {
            switch (static_cast<ck::jolt::debug_draw::ESleepStateClass>(InClassIndex))
            {
                case ck::jolt::debug_draw::ESleepStateClass::Kinematic: return _KinematicColor;
                case ck::jolt::debug_draw::ESleepStateClass::Awake:     return _DynamicAwakeColor;
                case ck::jolt::debug_draw::ESleepStateClass::Asleep:    return Get_SleepingColor();
                default:                                               return _StaticColor;
            }
        }

        case ECk_Jolt_DebugDrawColorMode::ObjectLayer:
        case ECk_Jolt_DebugDrawColorMode::ShapeType:
        default:
        { return ck_jolt_debug_draw_target::Get_DistinctColor(InClassIndex); }
    }
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Jolt_DebugDrawTarget::
    FCk_Jolt_DebugDrawTarget(
        UWorld* InWorld)
    : _Impl(MakePimpl<FImpl>())
{
    _Impl->_World = InWorld;

    // The contact demand is the union over every LIVE target, so membership of that set is the target's own
    // lifetime — a target that is constructed with contact flags already set must arm Jolt's statics at once.
    ck_jolt_debug_draw_target::GLiveTargets.Emplace(this);
    ck::jolt::debug_draw::Recompute_ContactDrawDemand();

    // OnWorldCleanup is the one delegate that fires for BOTH PIE end and map unload (UWorld::CleanupWorld is
    // the shared funnel), and it runs before components are torn down, so releasing here is ordered.
    _Impl->_WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddLambda(
        [this](UWorld* InCleanedWorld, bool, bool) -> void
        {
            if (InCleanedWorld != _Impl->_World.Get())
            { return; }

            HideAll();

            for (auto& Kvp : _Impl->_Buckets)
            { ck::jolt::debug_draw::Destroy_BucketIsm(Kvp.Value); }

            _Impl->_Buckets.Reset();

            ck::jolt::debug_draw::Destroy_LineBatcher(*_Impl);
        });
}

FCk_Jolt_DebugDrawTarget::
    ~FCk_Jolt_DebugDrawTarget()
{
    ck_jolt_debug_draw_target::GLiveTargets.Remove(this);
    ck::jolt::debug_draw::Recompute_ContactDrawDemand();

    FWorldDelegates::OnWorldCleanup.Remove(_Impl->_WorldCleanupHandle);

    for (auto& Kvp : _Impl->_Buckets)
    { ck::jolt::debug_draw::Destroy_BucketIsm(Kvp.Value); }

    ck::jolt::debug_draw::Destroy_LineBatcher(*_Impl);
}

namespace ck::jolt::debug_draw
{
    auto
        Destroy_BucketIsm(
            FBucket& InOutBucket)
        -> void
    {
        if (InOutBucket._BatchKeepAlive.GetPtr() != nullptr)
        { Note_BucketHolderRemoved(InOutBucket._BatchKeepAlive.GetPtr()); }

        auto* Ism = InOutBucket._Ism.Get();
        if (ck::IsValid(Ism))
        { Ism->DestroyComponent(); }

        InOutBucket._Ism.Reset();
        InOutBucket._SolidMid = nullptr;
        InOutBucket._WireframeMid = nullptr;
        InOutBucket._SlotCount = 0;
        InOutBucket._BatchKeepAlive = nullptr;
    }

    auto
        TryEnsure_LineBatcher(
            FCk_Jolt_DebugDrawTarget::FImpl& InOutTargetImpl)
        -> ULineBatchComponent*
    {
        if (auto* Existing = InOutTargetImpl._Lines.Get(); ck::IsValid(Existing))
        { return Existing; }

        if (InOutTargetImpl._LineBatcherCreateFailed)
        { return nullptr; }

        auto* World = InOutTargetImpl._World.Get();
        if (ck::Is_NOT_Valid(World))
        { return nullptr; }

        // Same ownership shape as the bucket ISMs: plain NewObject + RegisterComponentWithWorld, pinned by a
        // TStrongObjectPtr, so a preview or transient world with no pooling subsystem hosts it just as well.
        auto* Lines = NewObject<ULineBatchComponent>(World,
            MakeUniqueObjectName(World, ULineBatchComponent::StaticClass(), TEXT("CkJoltDebugDrawLines")));

        CK_ENSURE_IF_NOT(ck::IsValid(Lines),
            TEXT("Failed to create a LineBatchComponent for the Jolt debug renderer — no line, label or External "
                 "channel content will be drawn for this target"))
        {
            InOutTargetImpl._LineBatcherCreateFailed = true;
            return nullptr;
        }

        // Lifetime 0 is what makes a line last exactly one capture, so the component must never expire one
        // itself; the capture's Flush owns that.
        Lines->DefaultLifeTime = 0.0f;
        Lines->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Lines->SetCanEverAffectNavigation(false);
        Lines->SetCastShadow(false);
        Lines->SetMobility(EComponentMobility::Movable);
        Lines->SetWorldLocation(FVector::ZeroVector);
        Lines->RegisterComponentWithWorld(World);
        Lines->SetHiddenInGame(false);

        InOutTargetImpl._Lines.Reset(Lines);

        return Lines;
    }

    auto
        Destroy_LineBatcher(
            FCk_Jolt_DebugDrawTarget::FImpl& InOutTargetImpl)
        -> void
    {
        auto* Lines = InOutTargetImpl._Lines.Get();
        if (ck::IsValid(Lines))
        { Lines->DestroyComponent(); }

        InOutTargetImpl._Lines.Reset();
    }

    auto
        Reset_LineChannels(
            FCk_Jolt_DebugDrawTarget::FImpl& InOutTargetImpl)
        -> void
    {
        InOutTargetImpl._JphLines.Reset();
        InOutTargetImpl._Labels.Reset();

        auto* Lines = InOutTargetImpl._Lines.Get();
        if (ck::IsValid(Lines))
        { Lines->Flush(); }
    }

    auto
        Flush_LineChannels(
            FCk_Jolt_DebugDrawTarget::FImpl& InOutTargetImpl)
        -> void
    {
        auto AnyExternal = false;

        for (const auto& Kvp : InOutTargetImpl._ExternalChannels)
        { AnyExternal |= NOT Kvp.Value.IsEmpty(); }

        if (InOutTargetImpl._JphLines.IsEmpty() && InOutTargetImpl._ContactLines.IsEmpty() && NOT AnyExternal)
        { return; }

        auto* Lines = TryEnsure_LineBatcher(InOutTargetImpl);
        if (ck::Is_NOT_Valid(Lines))
        { return; }

        if (NOT InOutTargetImpl._JphLines.IsEmpty())
        { Lines->DrawLines(InOutTargetImpl._JphLines); }

        // Recorded around the previous step rather than produced by this capture, but flushed with it: the
        // contacts of a frame belong on screen beside the bodies of that frame.
        if (NOT InOutTargetImpl._ContactLines.IsEmpty())
        { Lines->DrawLines(InOutTargetImpl._ContactLines); }

        for (auto& Kvp : InOutTargetImpl._ExternalChannels)
        {
            if (Kvp.Value.IsEmpty())
            { continue; }

            Lines->DrawLines(Kvp.Value);
        }
    }

    auto
        Release_SlotsForKey(
            FCk_Jolt_DebugDrawTarget::FImpl& InOutTargetImpl,
            uint64 InSlotKey,
            EStatCounting InStatCounting)
        -> void
    {
        auto* Slots = InOutTargetImpl._BodySlots.Find(InSlotKey);
        if (Slots == nullptr)
        { return; }

        for (const auto& Slot : *Slots)
        {
            auto* Bucket = InOutTargetImpl._Buckets.Find(Slot._Bucket);
            if (Bucket == nullptr)
            { continue; }

            auto* Ism = Bucket->_Ism.Get();
            if (ck::Is_NOT_Valid(Ism) || NOT Ism->IsValidId(Slot._InstanceId))
            { continue; }

            Ism->RemoveInstanceById(Slot._InstanceId);
            Bucket->_SlotCount = FMath::Max(0, Bucket->_SlotCount - 1);

            if (InStatCounting == EStatCounting::Counted)
            { ++InOutTargetImpl._LastCaptureStats._InstancesRemoved; }
        }

        InOutTargetImpl._BodySlots.Remove(InSlotKey);
    }

    auto
        Apply_BucketMaterial(
            FBucket& InOutBucket,
            const FCk_Jolt_DebugDrawPalette& InPalette,
            uint8 InColorClassIndex,
            ECk_Jolt_DebugDraw_RenderMode& InOutRenderMode)
        -> void
    {
        auto* Ism = InOutBucket._Ism.Get();
        if (ck::Is_NOT_Valid(Ism))
        { return; }

        const auto TintedColor = Get_TintedColor(InOutBucket._BaseColor,
            Get_ClassOpacity(InColorClassIndex, InPalette.Get_Opacity()));

        const auto& Get_OrCreate_Mid = [&](TWeakObjectPtr<UMaterialInstanceDynamic>& InOutMid, UMaterial* InBase)
            -> UMaterialInstanceDynamic*
        {
            if (auto* Existing = InOutMid.Get(); ck::IsValid(Existing))
            { return Existing; }

            if (ck::Is_NOT_Valid(InBase))
            { return nullptr; }

            auto* Created = UMaterialInstanceDynamic::Create(InBase, Ism);
            InOutMid = Created;
            return Created;
        };

        // Deliberately NON-terminating recovery: an unavailable wireframe material degrades the whole target to
        // Solid and execution CONTINUES to assign the solid MID, because leaving the bucket unmaterialed would
        // be a worse failure than the mode silently not applying.
        if (InOutRenderMode == ECk_Jolt_DebugDraw_RenderMode::Wireframe)
        {
            auto* WireframeBase = ck_jolt_debug_draw_target::Get_WireframeBaseMaterial();

            CK_ENSURE_IF_NOT(ck::IsValid(WireframeBase),
                TEXT("Failed to load WireframeMaterial for the Jolt debug renderer — staying in Solid render mode"))
            {
                InOutRenderMode = ECk_Jolt_DebugDraw_RenderMode::Solid;
            }
        }

        const auto UseWireframe = InOutRenderMode == ECk_Jolt_DebugDraw_RenderMode::Wireframe;

        auto* Mid = UseWireframe
            ? Get_OrCreate_Mid(InOutBucket._WireframeMid, ck_jolt_debug_draw_target::Get_WireframeBaseMaterial())
            : Get_OrCreate_Mid(InOutBucket._SolidMid, ck_jolt_debug_draw_target::Get_SolidBaseMaterial());

        CK_ENSURE_IF_NOT(ck::IsValid(Mid),
            TEXT("Failed to create the Jolt debug-draw material instance — bodies will draw untinted with the default material"))
        { return; }

        Mid->SetVectorParameterValue(ck_jolt_debug_draw_target::ColorParameterName, TintedColor);
        Ism->SetMaterial(0, Mid);
    }
}

auto
    FCk_Jolt_DebugDrawTarget::
    HideAll()
    -> void
{
    if (NOT _Impl->_AnyLive && _Impl->_BodySlots.IsEmpty() && NOT _Impl->_FullPassEverRan)
    { return; }

    for (auto& Kvp : _Impl->_Buckets)
    {
        auto& Bucket = Kvp.Value;

        auto* Ism = Bucket._Ism.Get();
        if (ck::IsValid(Ism))
        { Ism->ClearInstances(); }

        Bucket._SlotCount = 0;
    }

    // The External sub-channels survive: they belong to their contributors, and nothing draws while the target
    // is hidden anyway — the next capture re-emits them. The contact channel does NOT: it describes a step that
    // will have been superseded by the time this target draws again.
    ck::jolt::debug_draw::Reset_LineChannels(*_Impl);
    _Impl->_ContactLines.Reset();

    _Impl->_BodySlots.Reset();
    _Impl->_StaticBodyKeys.Reset();
    _Impl->_PrevActiveBodyKeys.Reset();
    _Impl->_SleepingBodyKeys.Reset();
    _Impl->_CharacterKeys.Reset();
    _Impl->_InactiveBodyRecords.Reset();

    // The samples describe what a capture drew, and nothing is drawn any more. The SELECTION itself survives —
    // a re-opened target shows the same body highlighted.
    _Impl->_BodySample.Reset();
    _Impl->_CharacterSample.Reset();
    _Impl->_SelectionContacts.Reset();

    _Impl->_FullPassEverRan = false;
    _Impl->_SweepEverRan = false;
    _Impl->_AnyLive = false;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_IsDesired(
        bool InIsDesired)
    -> FCk_Jolt_DebugDrawTarget&
{
    if (_Impl->_IsDesired == InIsDesired)
    { return *this; }

    _Impl->_IsDesired = InIsDesired;

    if (NOT InIsDesired)
    { HideAll(); }

    return *this;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_RenderMode(
        ECk_Jolt_DebugDraw_RenderMode InRenderMode)
    -> void
{
    if (_Impl->_RenderMode == InRenderMode)
    { return; }

    _Impl->_RenderMode = InRenderMode;

    for (auto& Kvp : _Impl->_Buckets)
    {
        ck::jolt::debug_draw::Apply_BucketMaterial(Kvp.Value, _Impl->_Palette, Kvp.Key._ColorClassIndex,
            _Impl->_RenderMode);
    }
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_ClassVisibility(
        uint8 InClassIndex,
        bool InIsVisible)
    -> FCk_Jolt_DebugDrawTarget&
{
    const auto IsHideable = InClassIndex != ck::jolt::debug_draw::HighlightClassIndex &&
                            InClassIndex != ck::jolt::debug_draw::HoverClassIndex &&
                            InClassIndex < ck::jolt::debug_draw::MaxColorClasses;

    if (NOT IsHideable)
    { return *this; }

    const auto ClassBit = ck_jolt_debug_draw_target::Get_ClassBit(InClassIndex);

    const auto NewMask = InIsVisible
        ? _Impl->_HiddenClassMask & ~ClassBit
        : _Impl->_HiddenClassMask | ClassBit;

    if (_Impl->_HiddenClassMask == NewMask)
    { return *this; }

    _Impl->_HiddenClassMask = NewMask;

    for (auto& Kvp : _Impl->_Buckets)
    {
        if (Kvp.Key._ColorClassIndex != InClassIndex)
        { continue; }

        auto* Ism = Kvp.Value._Ism.Get();
        if (ck::Is_NOT_Valid(Ism))
        { continue; }

        Ism->SetVisibility(InIsVisible);
    }

    return *this;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_IsClassVisible(
        uint8 InClassIndex) const
    -> bool
{
    if (InClassIndex == ck::jolt::debug_draw::HighlightClassIndex ||
        InClassIndex == ck::jolt::debug_draw::HoverClassIndex)
    { return true; }

    if (InClassIndex >= ck::jolt::debug_draw::MaxColorClasses)
    { return true; }

    return (_Impl->_HiddenClassMask & ck_jolt_debug_draw_target::Get_ClassBit(InClassIndex)) == 0;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_ColorMode(
        ECk_Jolt_DebugDrawColorMode InColorMode)
    -> FCk_Jolt_DebugDrawTarget&
{
    if (_Impl->_ColorMode == InColorMode)
    { return *this; }

    _Impl->_ColorMode = InColorMode;

    // A class index means something different in every mode, so nothing the old mode hid should stay hidden —
    // and every body has to be re-bucketed, which the records would otherwise let the full pass skip.
    _Impl->_HiddenClassMask = 0;
    _Impl->_InactiveBodyRecords.Reset();
    _Impl->_FullPassEverRan = false;

    // Clearing the mask is not enough on its own: visibility lives on the COMPONENT, and TryEnsure_BucketIsm
    // only reads the mask when it creates one. A bucket that survives the mode change keeps the SetVisibility
    // (false) the old mode's hidden class gave it, so its bodies stay invisible under a mask that hides nothing.
    for (auto& Kvp : _Impl->_Buckets)
    {
        auto* Ism = Kvp.Value._Ism.Get();
        if (ck::Is_NOT_Valid(Ism))
        { continue; }

        constexpr auto Visible = true;
        Ism->SetVisibility(Visible);
    }

    return *this;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_ColorMode() const
    -> ECk_Jolt_DebugDrawColorMode
{
    return _Impl->_ColorMode;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_ObjectLayerNames(
        TArray<FString> InLayerNames)
    -> void
{
    _Impl->_ObjectLayerNames = MoveTemp(InLayerNames);
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_ObjectLayerNames() const
    -> const TArray<FString>&
{
    return _Impl->_ObjectLayerNames;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_LegendEntries(
        ECk_Jolt_DebugDrawColorMode InColorMode) const
    -> TArray<FCk_Jolt_DebugDrawLegendEntry>
{
    using namespace ck::jolt::debug_draw;

    auto Entries = TArray<FCk_Jolt_DebugDrawLegendEntry>{};

    const auto& Add_Entry = [&](uint8 InClassIndex, FText InName) -> void
    {
        Entries.Emplace(FCk_Jolt_DebugDrawLegendEntry{InClassIndex, MoveTemp(InName),
            _Impl->_Palette.Get_Color(InColorMode, InClassIndex)});
    };

    switch (InColorMode)
    {
        case ECk_Jolt_DebugDrawColorMode::BodyClass:
        {
            for (auto Index = uint8{0}; Index < static_cast<uint8>(ECk_Jolt_DebugDraw_ColorClass::Count); ++Index)
            { Add_Entry(Index, ck_jolt_debug_draw_target::Get_BodyClassName(Index)); }
            break;
        }

        case ECk_Jolt_DebugDrawColorMode::SleepState:
        {
            for (auto Index = uint8{0}; Index < static_cast<uint8>(ESleepStateClass::Count); ++Index)
            { Add_Entry(Index, ck_jolt_debug_draw_target::Get_SleepStateName(Index)); }
            break;
        }

        case ECk_Jolt_DebugDrawColorMode::ShapeType:
        {
            for (auto Index = uint8{0}; Index < static_cast<uint8>(EShapeTypeClass::Count); ++Index)
            { Add_Entry(Index, ck_jolt_debug_draw_target::Get_ShapeTypeName(Index)); }
            break;
        }

        case ECk_Jolt_DebugDrawColorMode::ObjectLayer:
        {
            // The S6 reverse lookup, read out of what the capture published: the layer table is signature-keyed
            // and its layers carry no names of their own, so a layer is named after the object channel of the
            // signature registered at it. Nothing here can register a layer.
            const auto NamedLayers = FMath::Min(_Impl->_ObjectLayerNames.Num(),
                static_cast<int32>(MaxNamedObjectLayer));

            auto LayerIndices = TSet<int32>{};

            for (auto Index = 0; Index < NamedLayers; ++Index)
            { LayerIndices.Emplace(Index); }

            // Names are published only while a layer TABLE is reachable — the capture needs a live layer context
            // to read one. Without it (a headless fixture, a world whose Jolt subsystem published none) the
            // legend would name nothing at all while the viewport is visibly drawing layer-coloured bodies, so
            // every index a live bucket is actually using earns a bare `Layer N` row of its own.
            // Only when the asked-for mode IS the live one: a bucket index is meaningless in any other mode, so
            // reading the buckets while colouring by body class would invent layers out of body classes.
            if (_Impl->_ColorMode == ECk_Jolt_DebugDrawColorMode::ObjectLayer)
            {
                for (const auto ClassIndex : Get_BucketColorClasses())
                {
                    if (ClassIndex >= MaxNamedObjectLayer)
                    { continue; }

                    LayerIndices.Emplace(static_cast<int32>(ClassIndex));
                }
            }

            auto SortedLayerIndices = LayerIndices.Array();
            SortedLayerIndices.Sort();

            for (const auto Index : SortedLayerIndices)
            {
                const auto* ChannelName = _Impl->_ObjectLayerNames.IsValidIndex(Index)
                    ? &_Impl->_ObjectLayerNames[Index]
                    : nullptr;

                Add_Entry(static_cast<uint8>(Index),
                    FText::FromString(ChannelName == nullptr || ChannelName->IsEmpty()
                        ? ck::Format_UE(TEXT("Layer {}"), Index)
                        : ck::Format_UE(TEXT("Layer {} — {}"), Index, *ChannelName)));
            }

            Add_Entry(MaxNamedObjectLayer,
                FText::FromString(ck::Format_UE(TEXT("Layer {}+"), MaxNamedObjectLayer)));
            break;
        }
    }

    // Mode-independent and always last, so a legend reads its bodies first and its selection markers after.
    Add_Entry(HighlightClassIndex, FText::FromString(TEXT("Selected")));
    Add_Entry(HoverClassIndex, FText::FromString(TEXT("Hovered")));

    return Entries;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_DrawFlags(
        ECk_Jolt_DebugDrawFlags InFlags)
    -> FCk_Jolt_DebugDrawTarget&
{
    if (_Impl->_DrawFlags == InFlags)
    { return *this; }

    _Impl->_DrawFlags = InFlags;

    // Same reasoning as Set_Palette: re-arming the pass alone would skip every body whose pose is unchanged, and
    // those are exactly the bodies whose extras have to appear (or whose shape instances have to be released).
    _Impl->_InactiveBodyRecords.Reset();
    _Impl->_FullPassEverRan = false;

    // The contact flags are the one part of this that is NOT private to the target: they drive Jolt's own
    // process-wide statics, so every change re-derives the union across every live target.
    ck::jolt::debug_draw::Recompute_ContactDrawDemand();

    return *this;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_DrawFlags() const
    -> ECk_Jolt_DebugDrawFlags
{
    return _Impl->_DrawFlags;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_IsDrawFlagSet(
        ECk_Jolt_DebugDrawFlags InFlag) const
    -> bool
{
    return EnumHasAnyFlags(_Impl->_DrawFlags, InFlag);
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_Labels() const
    -> const TArray<FCk_Jolt_DebugDrawLabel>&
{
    return _Impl->_Labels;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_NumLines() const
    -> int32
{
    const auto* Lines = _Impl->_Lines.Get();
    if (ck::Is_NOT_Valid(Lines))
    { return 0; }

    return Lines->BatchedLines.Num();
}

auto
    FCk_Jolt_DebugDrawTarget::
    Draw_ExternalLine(
        FName InChannel,
        const FVector& InFrom,
        const FVector& InTo,
        const FLinearColor& InColor)
    -> void
{
    _Impl->_ExternalChannels.FindOrAdd(InChannel).Emplace(
        ck::jolt::debug_draw::Make_DebugDrawLine(InFrom, InTo, InColor));
}

auto
    FCk_Jolt_DebugDrawTarget::
    Draw_ExternalBox(
        FName InChannel,
        const FBox& InLocalBox,
        const FTransform& InTransform,
        const FLinearColor& InColor)
    -> void
{
    const auto Min = InLocalBox.Min;
    const auto Max = InLocalBox.Max;

    const FVector LocalCorners[8] =
    {
        FVector{Min.X, Min.Y, Min.Z}, FVector{Max.X, Min.Y, Min.Z},
        FVector{Max.X, Max.Y, Min.Z}, FVector{Min.X, Max.Y, Min.Z},
        FVector{Min.X, Min.Y, Max.Z}, FVector{Max.X, Min.Y, Max.Z},
        FVector{Max.X, Max.Y, Max.Z}, FVector{Min.X, Max.Y, Max.Z},
    };

    auto Corners = TArray<FVector>{};
    Corners.Reserve(8);

    for (const auto& LocalCorner : LocalCorners)
    { Corners.Emplace(InTransform.TransformPosition(LocalCorner)); }

    const int32 Edges[12][2] =
    {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7},
    };

    auto& Channel = _Impl->_ExternalChannels.FindOrAdd(InChannel);

    for (const auto& Edge : Edges)
    { Channel.Emplace(ck::jolt::debug_draw::Make_DebugDrawLine(Corners[Edge[0]], Corners[Edge[1]], InColor)); }
}

auto
    FCk_Jolt_DebugDrawTarget::
    Draw_ExternalSphere(
        FName InChannel,
        const FVector& InCenter,
        float InRadius,
        const FLinearColor& InColor)
    -> void
{
    constexpr auto NumSegments = 16;

    auto& Channel = _Impl->_ExternalChannels.FindOrAdd(InChannel);

    const auto& Add_Circle = [&](const FVector& InAxisU, const FVector& InAxisV) -> void
    {
        auto Previous = InCenter + InAxisU * InRadius;

        for (auto Segment = 1; Segment <= NumSegments; ++Segment)
        {
            const auto Angle = 2.0 * UE_DOUBLE_PI * static_cast<double>(Segment) / NumSegments;
            const auto Current = InCenter +
                (InAxisU * FMath::Cos(Angle) + InAxisV * FMath::Sin(Angle)) * InRadius;

            Channel.Emplace(ck::jolt::debug_draw::Make_DebugDrawLine(Previous, Current, InColor));
            Previous = Current;
        }
    };

    Add_Circle(FVector::XAxisVector, FVector::YAxisVector);
    Add_Circle(FVector::YAxisVector, FVector::ZAxisVector);
    Add_Circle(FVector::ZAxisVector, FVector::XAxisVector);
}

auto
    FCk_Jolt_DebugDrawTarget::
    Draw_ExternalArrow(
        FName InChannel,
        const FVector& InFrom,
        const FVector& InTo,
        const FLinearColor& InColor)
    -> void
{
    auto& Channel = _Impl->_ExternalChannels.FindOrAdd(InChannel);
    Channel.Emplace(ck::jolt::debug_draw::Make_DebugDrawLine(InFrom, InTo, InColor));

    const auto Direction = InTo - InFrom;
    const auto Length = Direction.Size();

    if (FMath::IsNearlyZero(Length))
    { return; }

    const auto Forward = Direction / Length;
    const auto HeadSize = FMath::Min(Length * 0.25, 25.0);

    auto Right = FVector::CrossProduct(Forward, FVector::ZAxisVector);

    if (Right.IsNearlyZero())
    { Right = FVector::CrossProduct(Forward, FVector::XAxisVector); }

    Right.Normalize();
    const auto Up = FVector::CrossProduct(Forward, Right);

    for (const auto& Offset : {Right, -Right, Up, -Up})
    {
        Channel.Emplace(ck::jolt::debug_draw::Make_DebugDrawLine(
            InTo, InTo - Forward * HeadSize + Offset * HeadSize * 0.5, InColor));
    }
}

auto
    FCk_Jolt_DebugDrawTarget::
    Clear_External(
        FName InChannel)
    -> void
{
    _Impl->_ExternalChannels.Remove(InChannel);
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_NumExternalLines(
        FName InChannel) const
    -> int32
{
    const auto* Channel = _Impl->_ExternalChannels.Find(InChannel);
    return Channel != nullptr ? Channel->Num() : 0;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_NumContactLines() const
    -> int32
{
    return _Impl->_ContactLines.Num();
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_ContentBounds() const
    -> FBox
{
    auto Bounds = FBox{ForceInit};

    for (const auto& Kvp : _Impl->_Buckets)
    {
        if (NOT Get_IsClassVisible(Kvp.Key._ColorClassIndex))
        { continue; }

        const auto* Ism = Kvp.Value._Ism.Get();
        if (ck::Is_NOT_Valid(Ism) || Ism->GetInstanceCount() == 0)
        { continue; }

        // CalcBounds, not the cached Bounds: the cache is refreshed by the deferred render-state update, so a
        // component whose instances were added this frame still reports its registration-time (empty) box.
        Bounds += Ism->CalcBounds(Ism->GetComponentTransform()).GetBox();
    }

    return Bounds;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_HighlightedBodies(
        TArray<uint64> InBodyKeys)
    -> FCk_Jolt_DebugDrawTarget&
{
    if (_Impl->_HighlightedBodyKeys == InBodyKeys)
    { return *this; }

    // Only the set DIFFERENCE is released. Releasing every overlay and letting the next capture rebuild them
    // would make each addition to a multi-selection blink the bodies already in it.
    for (const auto DroppedKey : _Impl->_HighlightedBodyKeys)
    {
        if (InBodyKeys.Contains(DroppedKey))
        { continue; }

        // Excluded from the stats: this runs BETWEEN captures, and counting it would rewrite the last
        // capture's reported removals after the fact.
        ck::jolt::debug_draw::Release_SlotsForKey(*_Impl,
            ck::jolt::debug_draw::Make_HighlightKey(DroppedKey),
            ck::jolt::debug_draw::EStatCounting::Excluded);
    }

    _Impl->_HighlightedBodyKeys = MoveTemp(InBodyKeys);

    // The old selection's samples must not survive as the new one's: the next capture is what fills them.
    _Impl->_BodySample.Reset();
    _Impl->_CharacterSample.Reset();
    _Impl->_SelectionContacts.Reset();

    // The overlay is produced by the capture's own draw path, so a body only the revision-keyed full pass ever
    // draws — a static, or one asleep since before this target opened — would not gain its overlay until the
    // scene happened to change. Re-arming the full pass makes the very next capture produce it.
    _Impl->_FullPassEverRan = false;

    return *this;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_HighlightedBody(
        TOptional<uint64> InBodyKey)
    -> FCk_Jolt_DebugDrawTarget&
{
    return InBodyKey.IsSet()
        ? Set_HighlightedBodies(TArray<uint64>{*InBodyKey})
        : Set_HighlightedBodies(TArray<uint64>{});
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_HighlightedBodies() const
    -> const TArray<uint64>&
{
    return _Impl->_HighlightedBodyKeys;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_HighlightedBody() const
    -> TOptional<uint64>
{
    if (_Impl->_HighlightedBodyKeys.IsEmpty())
    { return {}; }

    return _Impl->_HighlightedBodyKeys[0];
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_IsolatedBodies(
        TSet<uint64> InBodyKeys)
    -> FCk_Jolt_DebugDrawTarget&
{
    if (_Impl->_IsolatedBodyKeys.Num() == InBodyKeys.Num() &&
        _Impl->_IsolatedBodyKeys.Includes(InBodyKeys))
    { return *this; }

    _Impl->_IsolatedBodyKeys = MoveTemp(InBodyKeys);

    // Same reasoning as Set_Palette: re-arming the pass alone would skip every body whose pose is unchanged, and
    // those are exactly the bodies that have to be released (or drawn again once isolation lifts).
    _Impl->_InactiveBodyRecords.Reset();
    _Impl->_FullPassEverRan = false;

    return *this;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Clear_Isolation()
    -> FCk_Jolt_DebugDrawTarget&
{
    return Set_IsolatedBodies(TSet<uint64>{});
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_IsolatedBodies() const
    -> const TSet<uint64>&
{
    return _Impl->_IsolatedBodyKeys;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_HoveredBody(
        TOptional<uint64> InBodyKey)
    -> FCk_Jolt_DebugDrawTarget&
{
    if (_Impl->_HoveredBodyKey == InBodyKey)
    { return *this; }

    if (_Impl->_HoveredBodyKey.IsSet())
    {
        // Excluded from the stats for the same reason the highlight's release is: this runs BETWEEN captures.
        ck::jolt::debug_draw::Release_SlotsForKey(*_Impl,
            ck::jolt::debug_draw::Make_HoverKey(*_Impl->_HoveredBodyKey),
            ck::jolt::debug_draw::EStatCounting::Excluded);
    }

    _Impl->_HoveredBodyKey = InBodyKey;

    // Same reason Set_HighlightedBody re-arms it: the overlay is produced by the capture's draw path, so a
    // static or long-asleep body would not gain one until the scene happened to change.
    _Impl->_FullPassEverRan = false;

    return *this;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_HoveredBody() const
    -> TOptional<uint64>
{
    return _Impl->_HoveredBodyKey;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_HighlightedBodyBounds() const
    -> TOptional<FBox>
{
    auto Bounds = FBox{ForceInit};

    // The UNION over the whole selection: a consumer framing a multi-selection wants one camera move that shows
    // all of it, not the first body's box.
    for (const auto HighlightedKey : _Impl->_HighlightedBodyKeys)
    {
        const auto* Slots = _Impl->_BodySlots.Find(HighlightedKey);
        if (Slots == nullptr)
        { continue; }

        for (const auto& Slot : *Slots)
        {
            const auto Placement = ck_jolt_debug_draw_target::TryGet_InstancePlacement(_Impl->_Buckets, Slot);

            if (NOT Placement.IsSet())
            { continue; }

            Bounds += Placement->_LocalBounds.TransformBy(Placement->_Transform);
        }
    }

    if (Bounds.IsValid == 0)
    { return {}; }

    return Bounds;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_BodySample() const
    -> TOptional<FCk_Jolt_DebugDraw_BodySample>
{
    return _Impl->_BodySample;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_CharacterSample() const
    -> TOptional<FCk_Jolt_DebugDraw_CharacterSample>
{
    return _Impl->_CharacterSample;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_WantsSelectionContacts(
        bool InWantsContacts)
    -> FCk_Jolt_DebugDrawTarget&
{
    if (_Impl->_WantsSelectionContacts == InWantsContacts)
    { return *this; }

    _Impl->_WantsSelectionContacts = InWantsContacts;

    // Dropping the demand empties the list here rather than at the next capture: a consumer that stopped asking
    // must not keep reading a manifold from a frame that has been superseded.
    if (NOT InWantsContacts)
    { _Impl->_SelectionContacts.Reset(); }

    return *this;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_WantsSelectionContacts() const
    -> bool
{
    return _Impl->_WantsSelectionContacts;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_SelectionContacts() const
    -> const TArray<FCk_Jolt_DebugDraw_ContactEntry>&
{
    return _Impl->_SelectionContacts;
}

auto
    FCk_Jolt_DebugDrawTarget::
    TryPick_Body(
        const FVector& InOrigin,
        const FVector& InDirection) const
    -> TOptional<uint64>
{
    const auto DirectionIsUsable = NOT InDirection.IsNearlyZero();

    CK_ENSURE_IF_NOT(DirectionIsUsable,
        TEXT("TryPick_Body was given a degenerate ray direction [{}] — no body can be picked from it"),
        InDirection)
    { return {}; }

    auto NearestKey = TOptional<uint64>{};
    auto NearestDistance = TNumericLimits<double>::Max();

    for (const auto& Kvp : _Impl->_BodySlots)
    {
        for (const auto& Slot : Kvp.Value)
        {
            // Neither overlay is pickable: they trace a body that is already pickable in its own right, and a
            // pick that returned an overlay key would name a slot no consumer can resolve to a body.
            if (Slot._Bucket._ColorClassIndex == ck::jolt::debug_draw::HighlightClassIndex ||
                Slot._Bucket._ColorClassIndex == ck::jolt::debug_draw::HoverClassIndex)
            { continue; }

            if (NOT Get_IsClassVisible(Slot._Bucket._ColorClassIndex))
            { continue; }

            const auto Placement = ck_jolt_debug_draw_target::TryGet_InstancePlacement(_Impl->_Buckets, Slot);

            if (NOT Placement.IsSet())
            { continue; }

            // The ray goes into instance space rather than the box coming out of it: an affine transform
            // preserves the parametric distance, so the test stays an oriented-box one and the hits from
            // differently-rotated instances remain directly comparable.
            const auto LocalOrigin = Placement->_Transform.InverseTransformPosition(InOrigin);
            const auto LocalDirection = Placement->_Transform.InverseTransformVector(InDirection);

            const auto HitDistance = ck_jolt_debug_draw_target::TryIntersect_RayBox(
                LocalOrigin, LocalDirection, Placement->_LocalBounds);

            if (NOT HitDistance.IsSet() || *HitDistance >= NearestDistance)
            { continue; }

            NearestDistance = *HitDistance;
            NearestKey = Kvp.Key;
        }
    }

    return NearestKey;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_Opacity(
        float InOpacity)
    -> void
{
    _Impl->_Palette.Set_Opacity(InOpacity);
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_Palette(
        const FCk_Jolt_DebugDrawPalette& InPalette)
    -> FCk_Jolt_DebugDrawTarget&
{
    _Impl->_Palette = InPalette;

    // Re-arming the pass is not enough on its own: it skips bodies whose pose is unchanged, and every one of
    // them still has to be repainted through the new palette.
    _Impl->_InactiveBodyRecords.Reset();
    _Impl->_FullPassEverRan = false;
    _Impl->_AppliedOpacity = -1.0f;

    return *this;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_World() const
    -> TWeakObjectPtr<UWorld>
{
    return _Impl->_World;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_IsDesired() const
    -> bool
{
    return _Impl->_IsDesired;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_RenderMode() const
    -> ECk_Jolt_DebugDraw_RenderMode
{
    return _Impl->_RenderMode;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_Palette() const
    -> const FCk_Jolt_DebugDrawPalette&
{
    return _Impl->_Palette;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_LastCaptureStats() const
    -> const ck::jolt::debug_draw::FDebugDrawStats&
{
    return _Impl->_LastCaptureStats;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_NumInstances() const
    -> int32
{
    auto Total = 0;

    for (const auto& Kvp : _Impl->_Buckets)
    {
        const auto* Ism = Kvp.Value._Ism.Get();
        if (ck::Is_NOT_Valid(Ism))
        { continue; }

        Total += Ism->GetInstanceCount();
    }

    return Total;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_NumBuckets() const
    -> int32
{
    return _Impl->_Buckets.Num();
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_Isms() const
    -> TArray<UInstancedStaticMeshComponent*>
{
    auto Isms = TArray<UInstancedStaticMeshComponent*>{};
    Isms.Reserve(_Impl->_Buckets.Num());

    for (const auto& Kvp : _Impl->_Buckets)
    {
        auto* Ism = Kvp.Value._Ism.Get();
        if (ck::Is_NOT_Valid(Ism))
        { continue; }

        Isms.Emplace(Ism);
    }

    return Isms;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_BucketColorClasses() const
    -> TArray<uint8>
{
    auto ColorClasses = TArray<uint8>{};
    ColorClasses.Reserve(_Impl->_Buckets.Num());

    for (const auto& Kvp : _Impl->_Buckets)
    {
        // LIVE buckets only. A bucket whose instances have all been released — the previous colour mode's, or a
        // population that has left the world — survives in the map whenever its geometry is one of the
        // renderer's shared unit primitives, which is never prunable. Reporting it would name a class nothing
        // on screen is drawn in.
        if (Kvp.Value._SlotCount == 0)
        { continue; }

        ColorClasses.Emplace(Kvp.Key._ColorClassIndex);
    }

    return ColorClasses;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
