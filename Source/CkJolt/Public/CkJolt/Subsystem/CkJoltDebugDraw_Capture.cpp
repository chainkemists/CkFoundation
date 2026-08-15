#include "CkJolt/Subsystem/CkJolt_DebugRenderer.h"

#if JPH_DEBUG_RENDERER

#include "CkCore/Technique/CkTechnique.h"
#include "CkCore/Validation/CkIsValid_Defaults.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkJolt/CkJolt_Stats.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Character/CkJoltCharacter_Fragment.h"
#include "CkJolt/CollisionLayers/CkJoltCollisionLayerTable.h"
#include "CkJolt/StaticWorld/CkJoltStaticActor_Fragment.h"
#include "CkJolt/Subsystem/CkJolt_DebugDrawTarget_Impl.h"

#include <Engine/CollisionProfile.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/StringTools.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Physics/Body/MassProperties.h>
#include <Jolt/Physics/Body/MotionProperties.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Jolt_DebugDraw_Capture"), STAT_CkJolt_DebugDrawCapture, STATGROUP_CkJolt);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_debugdraw_capture
{
    struct FContext_Capture
    {
        FCk_Jolt_DebugRenderer* _Renderer = nullptr;
        FCk_Jolt_DebugDrawTarget::FImpl* _TargetImpl = nullptr;
        JPH::PhysicsSystem* _PhysicsSystem = nullptr;
        const JPH::BodyLockInterfaceNoLock* _LockInterface = nullptr;
        FCk_Handle _TransientEntity;
        ck::jolt::debug_draw::FCaptureRevisions _Revisions;
        TSet<uint64> _ActiveKeys;
        bool _FullPassRanThisCapture = false;
    };

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_BodyKey(
            const JPH::BodyID& InBodyId)
        -> uint64
    {
        return ck::jolt::debug_draw::Make_BodyKey(InBodyId.GetIndexAndSequenceNumber());
    }

    auto
        TryResolve_Handle(
            uint64 InUserData,
            const FCk_Handle& InTransientEntity)
        -> FCk_Handle
    {
        if (ck::Is_NOT_Valid(InTransientEntity) || InUserData == 0)
        { return {}; }

        const auto Entity = FCk_Entity{FCk_Entity::IdType{static_cast<FCk_Entity::IdType>(InUserData)}};

        if (NOT InTransientEntity.Get_RegistryView().IsValid(Entity))
        { return {}; }

        return InTransientEntity.Get_ValidHandle(Entity.Get_ID());
    }

    // The baked static world and a Static-motion JoltBody share the Static object-layer DOMAIN, so the layer
    // cannot tell them apart. Their attribution entities can: only a baked body's user-data resolves to a
    // JoltStaticActor entity (CkJolt/Claude.md § "ECS-first attribution").
    auto
        Get_IsBakedStaticWorldBody(
            const JPH::Body& InBody,
            const FCk_Handle& InTransientEntity)
        -> bool
    {
        const auto Handle = TryResolve_Handle(InBody.GetUserData(), InTransientEntity);

        if (ck::Is_NOT_Valid(Handle))
        { return false; }

        return Handle.Has<ck::FFragment_JoltStaticActor_Current>();
    }

    auto
        Get_BodyClass(
            const JPH::Body& InBody,
            const FCk_Handle& InTransientEntity,
            bool InResolveBakedAttribution)
        -> ECk_Jolt_DebugDraw_ColorClass
    {
        if (InBody.IsSensor())
        { return ECk_Jolt_DebugDraw_ColorClass::Sensor; }

        switch (InBody.GetMotionType())
        {
            case JPH::EMotionType::Kinematic:
            { return ECk_Jolt_DebugDraw_ColorClass::Kinematic; }

            case JPH::EMotionType::Dynamic:
            {
                return InBody.IsActive()
                    ? ECk_Jolt_DebugDraw_ColorClass::Dynamic_Awake
                    : ECk_Jolt_DebugDraw_ColorClass::Dynamic_Sleeping;
            }

            case JPH::EMotionType::Static:
            default:
            {
                return InResolveBakedAttribution && Get_IsBakedStaticWorldBody(InBody, InTransientEntity)
                    ? ECk_Jolt_DebugDraw_ColorClass::BakedStatic
                    : ECk_Jolt_DebugDraw_ColorClass::Static;
            }
        }
    }

    auto
        Get_SleepStateClass(
            const JPH::Body& InBody)
        -> ck::jolt::debug_draw::ESleepStateClass
    {
        switch (InBody.GetMotionType())
        {
            case JPH::EMotionType::Kinematic:
            { return ck::jolt::debug_draw::ESleepStateClass::Kinematic; }

            case JPH::EMotionType::Dynamic:
            {
                return InBody.IsActive()
                    ? ck::jolt::debug_draw::ESleepStateClass::Awake
                    : ck::jolt::debug_draw::ESleepStateClass::Asleep;
            }

            default:
            { return ck::jolt::debug_draw::ESleepStateClass::Static; }
        }
    }

    // Re-indexed rather than cast: EShapeSubType reserves sixteen User* values in the middle of its range and
    // appends three real types after them, so its raw value is not a table index.
    auto
        Get_ShapeTypeClass(
            const JPH::Shape& InShape)
        -> ck::jolt::debug_draw::EShapeTypeClass
    {
        using EClass = ck::jolt::debug_draw::EShapeTypeClass;

        switch (InShape.GetSubType())
        {
            case JPH::EShapeSubType::Sphere:             return EClass::Sphere;
            case JPH::EShapeSubType::Box:                return EClass::Box;
            case JPH::EShapeSubType::Triangle:           return EClass::Triangle;
            case JPH::EShapeSubType::Capsule:            return EClass::Capsule;
            case JPH::EShapeSubType::TaperedCapsule:     return EClass::TaperedCapsule;
            case JPH::EShapeSubType::Cylinder:           return EClass::Cylinder;
            case JPH::EShapeSubType::TaperedCylinder:    return EClass::TaperedCylinder;
            case JPH::EShapeSubType::ConvexHull:         return EClass::ConvexHull;
            case JPH::EShapeSubType::StaticCompound:     return EClass::StaticCompound;
            case JPH::EShapeSubType::MutableCompound:    return EClass::MutableCompound;
            case JPH::EShapeSubType::RotatedTranslated:  return EClass::RotatedTranslated;
            case JPH::EShapeSubType::Scaled:             return EClass::Scaled;
            case JPH::EShapeSubType::OffsetCenterOfMass: return EClass::OffsetCenterOfMass;
            case JPH::EShapeSubType::Mesh:               return EClass::Mesh;
            case JPH::EShapeSubType::HeightField:        return EClass::HeightField;
            case JPH::EShapeSubType::SoftBody:           return EClass::SoftBody;
            case JPH::EShapeSubType::Plane:              return EClass::Plane;
            case JPH::EShapeSubType::Empty:              return EClass::Empty;
            default:                                     return EClass::Other;
        }
    }

    auto
        Get_ObjectLayerClassIndex(
            const JPH::Body& InBody)
        -> uint8
    {
        const auto Layer = static_cast<uint32>(InBody.GetObjectLayer());

        return static_cast<uint8>(FMath::Min(Layer,
            static_cast<uint32>(ck::jolt::debug_draw::MaxNamedObjectLayer)));
    }

    /*
     * The one place a body becomes a bucket. InResolveBakedAttribution is false on the incremental pass's
     * change-oracle path: BakedStatic is the only class input that costs a registry lookup, and it can only flip
     * when the body's JoltStaticActor entity dies — which already routes the body through the static-revision
     * funnel. Paying it per body per pass would undo Phase 4's incremental walk at 100k bodies.
     */
    auto
        Get_ColorClassIndex(
            ECk_Jolt_DebugDrawColorMode InColorMode,
            const JPH::Body& InBody,
            const FCk_Handle& InTransientEntity,
            bool InResolveBakedAttribution)
        -> uint8
    {
        switch (InColorMode)
        {
            case ECk_Jolt_DebugDrawColorMode::SleepState:
            { return static_cast<uint8>(Get_SleepStateClass(InBody)); }

            case ECk_Jolt_DebugDrawColorMode::ObjectLayer:
            { return Get_ObjectLayerClassIndex(InBody); }

            case ECk_Jolt_DebugDrawColorMode::ShapeType:
            {
                const auto* Shape = InBody.GetShape();
                return Shape != nullptr
                    ? static_cast<uint8>(Get_ShapeTypeClass(*Shape))
                    : static_cast<uint8>(ck::jolt::debug_draw::EShapeTypeClass::Other);
            }

            case ECk_Jolt_DebugDrawColorMode::BodyClass:
            default:
            { return static_cast<uint8>(Get_BodyClass(InBody, InTransientEntity, InResolveBakedAttribution)); }
        }
    }

    /*
     * P5-D61/S6: the ObjectLayer legend's names. Jolt object layers are allocated one per unique collision
     * SIGNATURE and carry no name of their own, so a layer is named after the object channel of the signature
     * registered at it. Strictly read-only — Get_Signature never registers, and a layer index past the table's
     * published count answers with the default signature rather than growing it.
     *
     * Refreshed only while the target is actually colouring by layer, and only when the layer count moved:
     * layers are registered on body spawn, so the count is stable within a frame and usually for a whole session.
     */
    auto
        TryRefresh_ObjectLayerNames(
            FContext_Capture& InCtx)
        -> void
    {
        auto& TargetImpl = *InCtx._TargetImpl;

        if (TargetImpl._ColorMode != ECk_Jolt_DebugDrawColorMode::ObjectLayer)
        { return; }

        if (ck::Is_NOT_Valid(InCtx._TransientEntity))
        { return; }

        const auto* LayerContext = InCtx._TransientEntity.Get_RegistryView()
            .TryGetContext<ck::jolt::FCk_Jolt_LayerContext>();

        if (LayerContext == nullptr || LayerContext->_Table == nullptr)
        { return; }

        const auto NumLayers = FMath::Min(LayerContext->_Table->Get_NumLayers(),
            static_cast<int32>(ck::jolt::debug_draw::MaxNamedObjectLayer));

        if (TargetImpl._ObjectLayerNames.Num() == NumLayers)
        { return; }

        const auto* CollisionProfiles = UCollisionProfile::Get();

        auto Names = TArray<FString>{};
        Names.Reserve(NumLayers);

        for (auto Layer = 0; Layer < NumLayers; ++Layer)
        {
            const auto Channel = LayerContext->_Table->Get_Signature(static_cast<uint16>(Layer))
                .Get_ObjectChannel();

            // The PROJECT's channel name, not the enum's: ECC_GameTraceChannel3 tells a reader nothing, while
            // the name the project gave that channel is the one they authored the collision profile against.
            Names.Emplace(ck::IsValid(CollisionProfiles)
                ? CollisionProfiles->ReturnChannelNameFromContainerIndex(static_cast<int32>(Channel.GetValue()))
                    .ToString()
                : FString{});
        }

        TargetImpl._ObjectLayerNames = MoveTemp(Names);
    }

    auto
        Draw_Shape(
            FContext_Capture& InCtx,
            uint64 InSlotKey,
            uint8 InColorClassIndex,
            const JPH::Shape& InShape,
            JPH::RMat44Arg InCenterOfMassTransform,
            float InScale)
        -> void
    {
        const auto Color = ck::jolt::Conv(
            InCtx._TargetImpl->_Palette.Get_Color(InCtx._TargetImpl->_ColorMode, InColorClassIndex));

        InCtx._Renderer->BeginBody(InSlotKey, InColorClassIndex);

        constexpr auto UseMaterialColors = false;
        constexpr auto DrawWireframe = false;

        // Jolt folds inScale into the model matrix it hands DrawGeometry, and the transform here is the CENTRE
        // OF MASS one — so a scale over 1 swells the overlay about the body's centre rather than sliding it.
        InShape.Draw(InCtx._Renderer, InCenterOfMassTransform, JPH::Vec3::sReplicate(InScale), Color,
            UseMaterialColors, DrawWireframe);

        InCtx._Renderer->EndBody();
    }

    // Isolation is a CAPTURE filter, unlike class visibility which is a component flag: an isolated-out body has
    // no instances at all, so nothing about it can be read off the target while it is hidden.
    auto
        Get_IsIsolatedOut(
            const FCk_Jolt_DebugDrawTarget::FImpl& InTargetImpl,
            uint64 InBodyKey)
        -> bool
    {
        return NOT InTargetImpl._IsolatedBodyKeys.IsEmpty() &&
               NOT InTargetImpl._IsolatedBodyKeys.Contains(InBodyKey);
    }

    auto
        Get_IsPrimarySelection(
            const FCk_Jolt_DebugDrawTarget::FImpl& InTargetImpl,
            uint64 InBodyKey)
        -> bool
    {
        return NOT InTargetImpl._HighlightedBodyKeys.IsEmpty() &&
               InTargetImpl._HighlightedBodyKeys[0] == InBodyKey;
    }

    // The overlays ride the SAME draw path as the thing they trace, so they follow a moving body for free and
    // their slots are reconciled, released and rebuilt exactly like any other body's. Both classes are
    // mode-independent and unhideable, so a selection reads the same whatever the bodies around it are
    // coloured by.
    auto
        Draw_SelectionOverlays(
            FContext_Capture& InCtx,
            uint64 InBodyKey,
            const JPH::Shape& InShape,
            JPH::RMat44Arg InCenterOfMassTransform)
        -> void
    {
        if (InCtx._TargetImpl->_HighlightedBodyKeys.Contains(InBodyKey))
        {
            Draw_Shape(InCtx, ck::jolt::debug_draw::Make_HighlightKey(InBodyKey),
                ck::jolt::debug_draw::HighlightClassIndex, InShape, InCenterOfMassTransform,
                ck::jolt::debug_draw::HighlightOverlayScale);
        }

        const auto& HoveredKey = InCtx._TargetImpl->_HoveredBodyKey;

        if (HoveredKey.IsSet() && *HoveredKey == InBodyKey)
        {
            Draw_Shape(InCtx, ck::jolt::debug_draw::Make_HoverKey(InBodyKey),
                ck::jolt::debug_draw::HoverClassIndex, InShape, InCenterOfMassTransform,
                ck::jolt::debug_draw::HoverOverlayScale);
        }
    }

    auto
        Conv_MotionType(
            JPH::EMotionType InMotionType)
        -> ECk_MotionType
    {
        switch (InMotionType)
        {
            case JPH::EMotionType::Kinematic: return ECk_MotionType::Kinematic;
            case JPH::EMotionType::Dynamic:   return ECk_MotionType::Dynamic;
            default:                          return ECk_MotionType::Static;
        }
    }

    auto
        Conv_MotionQuality(
            JPH::EMotionQuality InMotionQuality)
        -> ECk_MotionQuality
    {
        return InMotionQuality == JPH::EMotionQuality::LinearCast
            ? ECk_MotionQuality::LinearCast
            : ECk_MotionQuality::Discrete;
    }

    auto
        Conv_GroundState(
            JPH::CharacterBase::EGroundState InGroundState)
        -> ECk_JoltCharacter_GroundState
    {
        switch (InGroundState)
        {
            case JPH::CharacterBase::EGroundState::OnGround:      return ECk_JoltCharacter_GroundState::OnGround;
            case JPH::CharacterBase::EGroundState::OnSteepGround: return ECk_JoltCharacter_GroundState::OnSteepSlope;
            case JPH::CharacterBase::EGroundState::NotSupported:  return ECk_JoltCharacter_GroundState::NotSupported;
            default:                                              return ECk_JoltCharacter_GroundState::InAir;
        }
    }

    auto
        Get_ShapeTypeName(
            JPH::EShapeType InShapeType)
        -> FString
    {
        switch (InShapeType)
        {
            case JPH::EShapeType::Convex:      return TEXT("Convex");
            case JPH::EShapeType::Compound:    return TEXT("Compound");
            case JPH::EShapeType::Decorated:   return TEXT("Decorated");
            case JPH::EShapeType::Mesh:        return TEXT("Mesh");
            case JPH::EShapeType::HeightField: return TEXT("HeightField");
            case JPH::EShapeType::SoftBody:    return TEXT("SoftBody");
            case JPH::EShapeType::Plane:       return TEXT("Plane");
            case JPH::EShapeType::Empty:       return TEXT("Empty");
            default:                           return TEXT("User");
        }
    }

    /*
     * The full per-body sample, taken for the PRIMARY selection alone and only from inside the capture window —
     * a presentation consumer must never read a body scalar off the physics system for itself.
     *
     * Assert-safety is the whole difficulty here: JPH_ENABLE_ASSERTS is on in every configuration, so every
     * MotionProperties read goes through an Unchecked accessor, and Body::GetAllowSleeping is NEVER called for a
     * static body (it dereferences mMotionProperties without a guard).
     */
    auto
        Sample_Selection(
            FContext_Capture& InCtx,
            uint64 InBodyKey,
            const JPH::Body& InBody)
        -> void
    {
        if (NOT Get_IsPrimarySelection(*InCtx._TargetImpl, InBodyKey))
        { return; }

        auto Sample = FCk_Jolt_DebugDraw_BodySample{};

        Sample.Set_LinearVelocity(ck::jolt::Conv(InBody.GetLinearVelocity()));
        Sample.Set_AngularVelocity(ck::jolt::Conv(InBody.GetAngularVelocity()));
        Sample.Set_Friction(InBody.GetFriction());
        Sample.Set_Restitution(InBody.GetRestitution());
        Sample.Set_ObjectLayer(static_cast<uint16>(InBody.GetObjectLayer()));
        Sample.Set_BroadPhaseLayer(static_cast<uint8>(InBody.GetBroadPhaseLayer().GetValue()));
        Sample.Set_IsSensor(InBody.IsSensor());
        Sample.Set_UserData(InBody.GetUserData());
        Sample.Set_MotionType(Conv_MotionType(InBody.GetMotionType()));

        const auto& WorldBounds = InBody.GetWorldSpaceBounds();
        Sample.Set_WorldBounds(FBox{ck::jolt::Conv(WorldBounds.mMin), ck::jolt::Conv(WorldBounds.mMax)});

        if (const auto* Shape = InBody.GetShape(); Shape != nullptr)
        {
            Sample.Set_ShapeType(Get_ShapeTypeName(Shape->GetType()));
            Sample.Set_ShapeSubType(FString{ANSI_TO_TCHAR(
                JPH::sSubShapeTypeNames[static_cast<int32>(Shape->GetSubType())])});

            // A Jolt shape carries no scale of its own — only a ScaledShape wraps one, and every other shape is
            // authored at the size it draws at.
            if (Shape->GetSubType() == JPH::EShapeSubType::Scaled)
            {
                Sample.Set_ShapeScale(ck::jolt::Conv(
                    static_cast<const JPH::ScaledShape*>(Shape)->GetScale()));
            }
        }

        const auto* MotionProperties = InBody.GetMotionPropertiesUnchecked();

        if (MotionProperties != nullptr)
        {
            Sample.Set_MotionQuality(Conv_MotionQuality(MotionProperties->GetMotionQuality()));
            Sample.Set_GravityFactor(MotionProperties->GetGravityFactor());

            // Jolt stores the INVERSE mass, so an infinite-mass body has 0 and no finite reciprocal. The sample
            // reports 0 for it and says so, rather than dividing and publishing an infinity.
            const auto InverseMass = MotionProperties->GetInverseMassUnchecked();
            Sample.Set_Mass(InverseMass > 0.0f ? 1.0f / InverseMass : 0.0f);
        }

        // Only ever asked of a body that HAS motion properties: the getter dereferences them unguarded.
        if (NOT InBody.IsStatic())
        { Sample.Set_AllowsSleeping(TOptional<bool>{InBody.GetAllowSleeping()}); }

        InCtx._TargetImpl->_BodySample = MoveTemp(Sample);
    }

    // A CharacterVirtual has no JPH::Body, so none of the rigid-body sample exists for it. What it has is a
    // ground contact, and every getter for one lives on the CharacterBase base rather than on CharacterVirtual.
    auto
        Sample_CharacterSelection(
            FContext_Capture& InCtx,
            uint64 InCharacterKey,
            const JPH::CharacterVirtual& InCharacter)
        -> void
    {
        if (NOT Get_IsPrimarySelection(*InCtx._TargetImpl, InCharacterKey))
        { return; }

        auto Sample = FCk_Jolt_DebugDraw_CharacterSample{};

        Sample.Set_Velocity(ck::jolt::Conv(InCharacter.GetLinearVelocity()));
        Sample.Set_GroundNormal(ck::jolt::Conv(InCharacter.GetGroundNormal()));
        Sample.Set_GroundVelocity(ck::jolt::Conv(InCharacter.GetGroundVelocity()));
        Sample.Set_Up(ck::jolt::Conv(InCharacter.GetUp()));
        Sample.Set_GroundState(Conv_GroundState(InCharacter.GetGroundState()));

        if (const auto GroundBodyId = InCharacter.GetGroundBodyID(); NOT GroundBodyId.IsInvalid())
        { Sample.Set_GroundBodyKey(TOptional<uint64>{Get_BodyKey(GroundBodyId)}); }

        InCtx._TargetImpl->_CharacterSample = MoveTemp(Sample);
    }

    /*
     * P6-D45: the selection's contacts, as a NarrowPhaseQuery::CollideShape of its own shape at its own centre-of-
     * mass transform. This is a QUERY, not the ContactListener's manifold — the listener's contacts exist only
     * inside the solve, while this can be run whenever a consumer asks, which is what makes it on-demand.
     *
     * Two settings carry the whole behaviour: faces must be collected explicitly (the default is NoFaces, and
     * without them there are no contact POINTS to report), and a small positive separation distance is what makes
     * the resting-on-the-floor case — the one a user always means — show up at all, since a resting pair is not
     * penetrating. The self hit is excluded by the body filter rather than skipped afterwards, so it never costs
     * a narrow-phase test.
     */
    auto
        Query_SelectionContacts(
            FContext_Capture& InCtx,
            const JPH::Body& InBody)
        -> void
    {
        constexpr auto SeparationDistance = 2.0f;
        constexpr auto NormalLength = 25.0f;
        constexpr auto PointMarkerSize = 4.0f;

        auto& TargetImpl = *InCtx._TargetImpl;

        const auto* Shape = InBody.GetShape();
        if (Shape == nullptr)
        { return; }

        auto Settings = JPH::CollideShapeSettings{};
        Settings.mCollectFacesMode = JPH::ECollectFacesMode::CollectFaces;
        Settings.mMaxSeparationDistance = SeparationDistance;

        const auto SelfFilter = JPH::IgnoreSingleBodyFilter{InBody.GetID()};
        auto Collector = JPH::AllHitCollisionCollector<JPH::CollideShapeCollector>{};

        InCtx._PhysicsSystem->GetNarrowPhaseQuery().CollideShape(Shape, JPH::Vec3::sReplicate(1.0f),
            InBody.GetCenterOfMassTransform(), Settings, JPH::RVec3::sZero(), Collector,
            JPH::BroadPhaseLayerFilter{}, JPH::ObjectLayerFilter{}, SelfFilter);

        const auto DrawPoints = EnumHasAnyFlags(TargetImpl._DrawFlags, ECk_Jolt_DebugDrawFlags::ContactPoints);
        const auto DrawNormals = EnumHasAnyFlags(TargetImpl._DrawFlags, ECk_Jolt_DebugDrawFlags::ContactNormals);

        for (const auto& Hit : Collector.mHits)
        {
            auto Entry = FCk_Jolt_DebugDraw_ContactEntry{};
            Entry.Set_OtherBodyKey(Get_BodyKey(Hit.mBodyID2));
            Entry.Set_PenetrationDepth(Hit.mPenetrationDepth);

            const auto Normal = -Hit.mPenetrationAxis.Normalized();

            auto Points = TArray<FVector>{};

            const auto& Collect_Point = [&](JPH::Vec3Arg InPoint) -> void
            {
                Points.Emplace(ck::jolt::Conv(InPoint));

                if (DrawPoints)
                { InCtx._Renderer->DrawMarker(InPoint, JPH::Color::sYellow, PointMarkerSize); }

                if (DrawNormals)
                { InCtx._Renderer->DrawLine(InPoint, InPoint + Normal * NormalLength, JPH::Color::sCyan); }
            };

            // The colliding FACE is the contact patch; a pair whose faces were not resolved still has the single
            // deepest point, which is what a point-only consumer would have shown anyway.
            if (Hit.mShape2Face.empty())
            { Collect_Point(Hit.mContactPointOn2); }
            else
            {
                for (const auto& FacePoint : Hit.mShape2Face)
                { Collect_Point(FacePoint); }
            }

            Entry.Set_NumContactPoints(Points.Num());
            Entry.Set_ContactPoints(Points);

            TargetImpl._SelectionContacts.Emplace(MoveTemp(Entry));
        }
    }

    auto
        Make_InactiveRecord(
            const FContext_Capture& InCtx,
            const JPH::Body& InBody)
        -> ck::jolt::debug_draw::FInactiveBodyRecord
    {
        constexpr auto DoNotResolveBakedAttribution = false;

        auto Record = ck::jolt::debug_draw::FInactiveBodyRecord{};
        Record._Position = InBody.GetPosition();
        Record._Rotation = InBody.GetRotation();
        Record._Shape = InBody.GetShape();
        Record._ColorClassIndex = Get_ColorClassIndex(InCtx._TargetImpl->_ColorMode, InBody,
            InCtx._TransientEntity, DoNotResolveBakedAttribution);
        return Record;
    }

    /*
     * Jolt's own per-body extras, re-implemented here because the capture never calls DrawBodies: it needs one
     * BeginBody/EndBody scope per body to reconcile instance slots, which DrawBodies' whole-world walk cannot
     * give it. Every helper below decomposes into DrawLine, so the output lands in the target's line channel.
     *
     * The sizes are Jolt's own constants CONVERTED: this world is centimetres and Jolt's samples are metres, so
     * a 0.1/0.2 arrow-head or axis is two millimetres of screen space — i.e. invisible, which is what the
     * pre-Phase-5 in-world draw shipped. Same ×100 rule the module applies to every other Jolt scalar.
     *
     * Assert-safety: JPH_ENABLE_ASSERTS is on in every configuration, so a static body may not touch a checked
     * MotionProperties accessor. GetLinearVelocity/GetAngularVelocity are internally guarded (they return zero
     * for a static body); everything reaching MotionProperties goes through the Unchecked accessors or sits
     * behind an IsDynamic() gate.
     */
    auto
        Draw_BodyExtras(
            FContext_Capture& InCtx,
            const JPH::Body& InBody)
        -> void
    {
        constexpr auto ArrowSize = 10.0f;
        constexpr auto AxisSize = 20.0f;
        constexpr auto MassTextHeight = 20.0f;

        const auto Flags = InCtx._TargetImpl->_DrawFlags;
        auto* Renderer = InCtx._Renderer;

        if (EnumHasAnyFlags(Flags, ECk_Jolt_DebugDrawFlags::Velocity))
        {
            const auto Position = InBody.GetCenterOfMassPosition();
            Renderer->DrawArrow(Position, Position + InBody.GetLinearVelocity(), JPH::Color::sGreen, ArrowSize);
        }

        if (EnumHasAnyFlags(Flags, ECk_Jolt_DebugDrawFlags::AngularVelocity))
        {
            const auto Position = InBody.GetCenterOfMassPosition();
            Renderer->DrawArrow(Position, Position + InBody.GetAngularVelocity(), JPH::Color::sRed, ArrowSize);
        }

        if (EnumHasAnyFlags(Flags, ECk_Jolt_DebugDrawFlags::WorldTransform))
        { Renderer->DrawCoordinateSystem(InBody.GetWorldTransform(), AxisSize); }

        if (EnumHasAnyFlags(Flags, ECk_Jolt_DebugDrawFlags::CenterOfMassTransform))
        { Renderer->DrawCoordinateSystem(InBody.GetCenterOfMassTransform(), AxisSize); }

        if (EnumHasAnyFlags(Flags, ECk_Jolt_DebugDrawFlags::BoundingBox))
        { Renderer->DrawWireBox(InBody.GetWorldSpaceBounds(), JPH::Color::sWhite); }

        if (NOT EnumHasAnyFlags(Flags, ECk_Jolt_DebugDrawFlags::MassAndInertia) || NOT InBody.IsDynamic())
        { return; }

        const auto* MotionProperties = InBody.GetMotionPropertiesUnchecked();
        if (MotionProperties == nullptr)
        { return; }

        const auto InverseMass = MotionProperties->GetInverseMassUnchecked();
        const auto InverseInertia = MotionProperties->GetInverseInertiaDiagonal();

        if (InverseMass <= 0.0f || JPH::Vec3::sEquals(InverseInertia, JPH::Vec3::sZero()).TestAnyXYZTrue())
        { return; }

        const auto Mass = 1.0f / InverseMass;
        const auto BoxSize = JPH::MassProperties::sGetEquivalentSolidBoxSize(Mass, InverseInertia.Reciprocal());

        Renderer->DrawWireBox(
            InBody.GetCenterOfMassTransform() * JPH::Mat44::sRotation(MotionProperties->GetInertiaRotation()),
            JPH::AABox{-0.5f * BoxSize, 0.5f * BoxSize}, JPH::Color::sOrange);

        // The mass BOX is the MassAndInertia flag's own output; the numeric mass beside it is TEXT, and text is
        // what the Labels flag governs. Without this gate Labels is dead and every consumer of Get_Labels() is
        // fed per-body strings it never asked for (P5-D64/F4).
        if (NOT EnumHasAnyFlags(Flags, ECk_Jolt_DebugDrawFlags::Labels))
        { return; }

        Renderer->DrawText3D(InBody.GetCenterOfMassPosition(),
            JPH::StringFormat("%.2f", static_cast<double>(Mass)), JPH::Color::sOrange, MassTextHeight);
    }

    auto
        Draw_Body(
            FContext_Capture& InCtx,
            const JPH::Body& InBody)
        -> void
    {
        constexpr auto ResolveBakedAttribution = true;
        constexpr auto NoOverlaySwell = 1.0f;

        const auto ColorClassIndex = Get_ColorClassIndex(InCtx._TargetImpl->_ColorMode, InBody,
            InCtx._TransientEntity, ResolveBakedAttribution);
        const auto BodyKey = Get_BodyKey(InBody.GetID());
        const auto& Shape = *InBody.GetShape();
        const auto CenterOfMassTransform = InBody.GetCenterOfMassTransform();

        if (EnumHasAnyFlags(InCtx._TargetImpl->_DrawFlags, ECk_Jolt_DebugDrawFlags::Shape))
        {
            Draw_Shape(InCtx, BodyKey, ColorClassIndex, Shape, CenterOfMassTransform, NoOverlaySwell);
            Draw_SelectionOverlays(InCtx, BodyKey, Shape, CenterOfMassTransform);
        }
        else
        {
            // Shape OFF still has to reconcile: an empty body scope is what releases the instance slots this
            // body held while it was on, and the overlay is keyed separately so it needs its own scope.
            // EXCLUDED from the stats: the body is alive and this capture drew it (as lines), so counting the
            // release would report a live body as churn every time the flag is flipped.
            InCtx._Renderer->Release_BodySlots(BodyKey, ck::jolt::debug_draw::EStatCounting::Excluded);
        }

        Draw_BodyExtras(InCtx, InBody);
        Sample_Selection(InCtx, BodyKey, InBody);

        // On demand only, and for the primary selection only: the shape query is the most expensive thing this
        // capture can be asked to do, and it answers a panel nobody has open most of the time.
        if (InCtx._TargetImpl->_WantsSelectionContacts && Get_IsPrimarySelection(*InCtx._TargetImpl, BodyKey))
        { Query_SelectionContacts(InCtx, InBody); }

        ++InCtx._TargetImpl->_LastCaptureStats._BodiesCaptured;
    }

    // ----------------------------------------------------------------------------------------------------------------

    struct FTechnique_CaptureJoltWorld : ck::Technique<FTechnique_CaptureJoltWorld, FContext_Capture&>
    {
        FTechnique_CaptureJoltWorld()
        {
            AddStep(&FTechnique_CaptureJoltWorld::DrawInactiveBodiesOnSceneRevisionChange);
            AddStep(&FTechnique_CaptureJoltWorld::DrawActiveBodies);
            AddStep(&FTechnique_CaptureJoltWorld::RecolorBodiesThatFellAsleep);
            AddStep(&FTechnique_CaptureJoltWorld::ReleaseDestroyedSleepingBodies);
            AddStep(&FTechnique_CaptureJoltWorld::DrawCharacters);
        }

        static auto DrawInactiveBodiesOnSceneRevisionChange(FTechnique_CaptureJoltWorld& InSelf, FContext_Capture& InCtx) -> ck::EStepResult;
        static auto DrawActiveBodies(FTechnique_CaptureJoltWorld& InSelf, FContext_Capture& InCtx) -> ck::EStepResult;
        static auto RecolorBodiesThatFellAsleep(FTechnique_CaptureJoltWorld& InSelf, FContext_Capture& InCtx) -> ck::EStepResult;
        static auto ReleaseDestroyedSleepingBodies(FTechnique_CaptureJoltWorld& InSelf, FContext_Capture& InCtx) -> ck::EStepResult;
        static auto DrawCharacters(FTechnique_CaptureJoltWorld& InSelf, FContext_Capture& InCtx) -> ck::EStepResult;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Sleeping dynamics belong in this pass, not just statics: neither the active pass nor the sleep diff can
    // see a body that was already asleep when the target began capturing, so without them a settled pile is
    // invisible until something wakes it.
    auto
        FTechnique_CaptureJoltWorld::
        DrawInactiveBodiesOnSceneRevisionChange(
            FTechnique_CaptureJoltWorld& InSelf,
            FContext_Capture& InCtx)
        -> ck::EStepResult
    {
        auto& TargetImpl = *InCtx._TargetImpl;

        // A per-body extra is a LINE, and lines are cleared every capture — so while any extra flag is on, the
        // pass has to run every capture or an inactive body's arrows/axes/box would exist only on the pass that
        // first drew them. This is exactly the cost the pre-Phase-5 in-world DrawBodies path paid unconditionally.
        const auto ExtrasDemandRedraw = EnumHasAnyFlags(TargetImpl._DrawFlags,
            ck::jolt::debug_draw::PerBodyExtraFlags);

        const auto SceneChanged = NOT TargetImpl._FullPassEverRan ||
            TargetImpl._CapturedStaticSceneRevision != InCtx._Revisions._StaticScene ||
            ExtrasDemandRedraw;

        if (NOT SceneChanged)
        { return ck::EStepResult::Continue; }

        auto PreviouslyRetainedKeys = MoveTemp(TargetImpl._StaticBodyKeys);
        PreviouslyRetainedKeys.Append(MoveTemp(TargetImpl._SleepingBodyKeys));

        auto PreviousRecords = MoveTemp(TargetImpl._InactiveBodyRecords);

        TargetImpl._StaticBodyKeys.Reset();
        TargetImpl._SleepingBodyKeys.Reset();
        TargetImpl._InactiveBodyRecords.Reset();
        TargetImpl._InactiveBodyRecords.Reserve(PreviousRecords.Num());

        auto AllBodyIds = JPH::BodyIDVector{};
        InCtx._PhysicsSystem->GetBodies(AllBodyIds);

        for (const auto& BodyId : AllBodyIds)
        {
            const auto* Body = InCtx._LockInterface->TryGetBody(BodyId);
            if (Body == nullptr)
            { continue; }

            // An active body is the active pass's to draw, and drawing it twice in one capture would release
            // and re-add its slots for nothing.
            if (Body->IsActive())
            { continue; }

            const auto Key = Get_BodyKey(BodyId);

            // Isolated out: no record, no key retained — the stale-key loop below is what releases whatever it
            // was drawn as, so the body leaves the screen entirely rather than merely stopping to update.
            if (Get_IsIsolatedOut(TargetImpl, Key))
            { continue; }

            const auto Record = Make_InactiveRecord(InCtx, *Body);

            // The incremental half: a body whose pose, shape and colour class are exactly what the last full
            // pass already drew is skipped outright, which is what a streaming-in cell or a re-bake must not pay
            // for the whole scene. A selected or hovered body is never skipped — its overlay is produced by this
            // draw path, and re-arming the pass is the only way a static or long-asleep body ever gains one.
            const auto* PreviousRecord = PreviousRecords.Find(Key);
            const auto IsOverlaid = TargetImpl._HighlightedBodyKeys.Contains(Key) ||
                (TargetImpl._HoveredBodyKey.IsSet() && *TargetImpl._HoveredBodyKey == Key);
            const auto IsUnchanged = PreviousRecord != nullptr && *PreviousRecord == Record &&
                NOT IsOverlaid && NOT ExtrasDemandRedraw;

            if (NOT IsUnchanged)
            { Draw_Body(InCtx, *Body); }

            TargetImpl._InactiveBodyRecords.Emplace(Key, Record);

            if (Body->GetMotionType() == JPH::EMotionType::Static)
            { TargetImpl._StaticBodyKeys.Emplace(Key); }
            else
            { TargetImpl._SleepingBodyKeys.Emplace(Key); }
        }

        for (const auto StaleKey : PreviouslyRetainedKeys)
        {
            if (TargetImpl._StaticBodyKeys.Contains(StaleKey) || TargetImpl._SleepingBodyKeys.Contains(StaleKey))
            { continue; }

            InCtx._Renderer->Release_BodySlots(StaleKey, ck::jolt::debug_draw::EStatCounting::Counted);
        }

        TargetImpl._CapturedStaticSceneRevision = InCtx._Revisions._StaticScene;
        TargetImpl._FullPassEverRan = true;
        TargetImpl._LastCaptureStats._FullPassRan = true;
        InCtx._FullPassRanThisCapture = true;

        return ck::EStepResult::Continue;
    }

    auto
        FTechnique_CaptureJoltWorld::
        DrawActiveBodies(
            FTechnique_CaptureJoltWorld& InSelf,
            FContext_Capture& InCtx)
        -> ck::EStepResult
    {
        auto& TargetImpl = *InCtx._TargetImpl;

        auto ActiveBodyIds = JPH::BodyIDVector{};
        InCtx._PhysicsSystem->GetActiveBodies(JPH::EBodyType::RigidBody, ActiveBodyIds);

        InCtx._ActiveKeys.Reserve(static_cast<int32>(ActiveBodyIds.size()));

        for (const auto& BodyId : ActiveBodyIds)
        {
            const auto* Body = InCtx._LockInterface->TryGetBody(BodyId);
            if (Body == nullptr)
            { continue; }

            if (Body->GetMotionType() == JPH::EMotionType::Static)
            { continue; }

            const auto Key = Get_BodyKey(BodyId);

            // The key still counts as ACTIVE while isolated out — the sleep diff below is a diff against the
            // active SET, and dropping it there would make the next capture treat the body as freshly asleep.
            if (Get_IsIsolatedOut(TargetImpl, Key))
            { InCtx._Renderer->Release_BodySlots(Key, ck::jolt::debug_draw::EStatCounting::Counted); }
            else
            { Draw_Body(InCtx, *Body); }

            InCtx._ActiveKeys.Emplace(Key);
            TargetImpl._SleepingBodyKeys.Remove(Key);

            // The body now lives in an ACTIVE bucket, so the inactive record no longer describes what is on
            // screen. Left behind, a body that woke and settled again at its old pose would match its stale
            // record and be skipped by the next full pass — staying awake-coloured forever.
            TargetImpl._InactiveBodyRecords.Remove(Key);
        }

        return ck::EStepResult::Continue;
    }

    // Bounded by the PREVIOUS frame's active count.
    auto
        FTechnique_CaptureJoltWorld::
        RecolorBodiesThatFellAsleep(
            FTechnique_CaptureJoltWorld& InSelf,
            FContext_Capture& InCtx)
        -> ck::EStepResult
    {
        auto& TargetImpl = *InCtx._TargetImpl;

        for (const auto FellAsleepKey : TargetImpl._PrevActiveBodyKeys)
        {
            if (InCtx._ActiveKeys.Contains(FellAsleepKey))
            { continue; }

            // The full pass already drew every inactive body this capture, sleeping colour included — drawing
            // again here would double-count the body and churn its slots.
            if (InCtx._FullPassRanThisCapture && TargetImpl._SleepingBodyKeys.Contains(FellAsleepKey))
            { continue; }

            const auto BodyId = JPH::BodyID{static_cast<JPH::uint32>(FellAsleepKey)};
            const auto* Body = InCtx._LockInterface->TryGetBody(BodyId);

            if (Body == nullptr || Get_IsIsolatedOut(TargetImpl, FellAsleepKey))
            {
                InCtx._Renderer->Release_BodySlots(FellAsleepKey, ck::jolt::debug_draw::EStatCounting::Counted);
                continue;
            }

            Draw_Body(InCtx, *Body);
            TargetImpl._SleepingBodyKeys.Emplace(FellAsleepKey);
        }

        TargetImpl._PrevActiveBodyKeys = MoveTemp(InCtx._ActiveKeys);

        return ck::EStepResult::Continue;
    }

    // A sleeping body is invisible to both body passes, so its slots would outlive its destruction. The walk is
    // O(sleeping bodies), so it is gated on the world's body-removed revision: no body has died since the last
    // sweep means there is nothing here to find.
    auto
        FTechnique_CaptureJoltWorld::
        ReleaseDestroyedSleepingBodies(
            FTechnique_CaptureJoltWorld& InSelf,
            FContext_Capture& InCtx)
        -> ck::EStepResult
    {
        auto& TargetImpl = *InCtx._TargetImpl;

        const auto BodiesWereRemoved = NOT TargetImpl._SweepEverRan ||
            TargetImpl._CapturedBodyRemovedRevision != InCtx._Revisions._BodyRemoved;

        if (NOT BodiesWereRemoved)
        { return ck::EStepResult::Continue; }

        auto DeadSleepingKeys = TArray<uint64>{};

        for (const auto SleepingKey : TargetImpl._SleepingBodyKeys)
        {
            const auto BodyId = JPH::BodyID{static_cast<JPH::uint32>(SleepingKey)};

            if (InCtx._LockInterface->TryGetBody(BodyId) == nullptr)
            { DeadSleepingKeys.Emplace(SleepingKey); }
        }

        for (const auto DeadKey : DeadSleepingKeys)
        {
            InCtx._Renderer->Release_BodySlots(DeadKey, ck::jolt::debug_draw::EStatCounting::Counted);
            TargetImpl._SleepingBodyKeys.Remove(DeadKey);
            TargetImpl._InactiveBodyRecords.Remove(DeadKey);
        }

        TargetImpl._CapturedBodyRemovedRevision = InCtx._Revisions._BodyRemoved;
        TargetImpl._SweepEverRan = true;
        TargetImpl._LastCaptureStats._SweepRan = true;

        return ck::EStepResult::Continue;
    }

    // CharacterVirtuals have no BodyID, so they are never in either body pass.
    auto
        FTechnique_CaptureJoltWorld::
        DrawCharacters(
            FTechnique_CaptureJoltWorld& InSelf,
            FContext_Capture& InCtx)
        -> ck::EStepResult
    {
        auto& TargetImpl = *InCtx._TargetImpl;

        auto LiveCharacterKeys = TSet<uint64>{};

        if (ck::IsValid(InCtx._TransientEntity))
        {
            // A handle is a value over (entity id + registry), so the copy addresses the same entity. It exists
            // because FCk_Handle::View() has no usable const overload.
            auto TransientEntity = InCtx._TransientEntity;

            TransientEntity.View<ck::FFragment_JoltCharacter_Current>().ForEach(
            [&](FCk_Entity InEntity, const ck::FFragment_JoltCharacter_Current& InCurrent) -> void
            {
                const auto* Character = InCurrent.Get_Character().GetPtr();
                if (Character == nullptr)
                { return; }

                const auto Key = ck::jolt::debug_draw::Make_CharacterBodyKey_FromEntityId(
                    static_cast<uint64>(InEntity.Get_ID()));

                // Not added to the live set: the release loop below is what takes an isolated-out character's
                // instances away, exactly as the stale-key loop does for a body.
                if (Get_IsIsolatedOut(TargetImpl, Key))
                { return; }

                LiveCharacterKeys.Emplace(Key);

                Sample_CharacterSelection(InCtx, Key, *Character);

                // Counted before the Shape gate so the character path agrees with Draw_Body's: both report a
                // body the capture VISITED and reconciled, whether or not the Shape flag left it any instances.
                ++TargetImpl._LastCaptureStats._BodiesCaptured;

                if (NOT EnumHasAnyFlags(TargetImpl._DrawFlags, ECk_Jolt_DebugDrawFlags::Shape))
                {
                    InCtx._Renderer->Release_BodySlots(Key, ck::jolt::debug_draw::EStatCounting::Excluded);
                    return;
                }

                const auto& Shape = *Character->GetShape();
                const auto CenterOfMassTransform = Character->GetCenterOfMassTransform();

                // A CharacterVirtual has no Body, so none of the other modes' inputs (object layer, island,
                // sleep state) exist for it. It keeps the BodyClass Character index in every mode — the
                // alternative is a capsule that vanishes into a meaningless bucket the moment the mode changes.
                constexpr auto NoOverlaySwell = 1.0f;
                Draw_Shape(InCtx, Key,
                    ck::jolt::debug_draw::Get_ClassIndex(ECk_Jolt_DebugDraw_ColorClass::Character),
                    Shape, CenterOfMassTransform, NoOverlaySwell);
                Draw_SelectionOverlays(InCtx, Key, Shape, CenterOfMassTransform);
            });
        }

        for (const auto GoneKey : TargetImpl._CharacterKeys)
        {
            if (NOT LiveCharacterKeys.Contains(GoneKey))
            { InCtx._Renderer->Release_BodySlots(GoneKey, ck::jolt::debug_draw::EStatCounting::Counted); }
        }

        TargetImpl._CharacterKeys = MoveTemp(LiveCharacterKeys);

        return ck::EStepResult::Continue;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_DebugRenderer::
    Capture_JoltWorld(
        FCk_Jolt_DebugDrawTarget& InTarget,
        JPH::PhysicsSystem& InPhysicsSystem,
        const ck::jolt::debug_draw::FCaptureRevisions& InRevisions,
        const FCk_Handle& InTransientEntity)
    -> void
{
    SCOPE_CYCLE_COUNTER(STAT_CkJolt_DebugDrawCapture);

    using namespace ck_jolt_debugdraw_capture;

    BeginCapture(InTarget);

    auto Context = FContext_Capture{};
    Context._Renderer = this;
    Context._TargetImpl = InTarget._Impl.Get();
    Context._PhysicsSystem = &InPhysicsSystem;
    // NoLock mirrors FJoltWorld::DoCapturePoses_AnyThread's contract: the capture window sits after the async
    // step was consumed and before the next one is kicked, so no worker is mutating bodies.
    Context._LockInterface = &InPhysicsSystem.GetBodyLockInterfaceNoLock();
    Context._TransientEntity = InTransientEntity;
    Context._Revisions = InRevisions;

    // Re-sampled from scratch: a selection this capture does not draw reads as unknown rather than as a value
    // that has since gone stale. The contacts go with them — a manifold from a superseded step is not live state.
    InTarget._Impl->_BodySample.Reset();
    InTarget._Impl->_CharacterSample.Reset();
    InTarget._Impl->_SelectionContacts.Reset();

    TryRefresh_ObjectLayerNames(Context);

    static auto Technique = FTechnique_CaptureJoltWorld{};
    Technique.ProcessAllSteps(Context);

    // Outside every body scope on purpose: constraint drawing is whole-world and line-shaped, so it belongs to
    // the line channel rather than to any one body's instance slots. Still inside the active-target scope, which
    // is what routes those lines into THIS target.
    const auto DrawFlags = InTarget._Impl->_DrawFlags;

    if (EnumHasAnyFlags(DrawFlags, ECk_Jolt_DebugDrawFlags::Constraints))
    { InPhysicsSystem.DrawConstraints(this); }

    if (EnumHasAnyFlags(DrawFlags, ECk_Jolt_DebugDrawFlags::ConstraintLimits))
    { InPhysicsSystem.DrawConstraintLimits(this); }

    if (EnumHasAnyFlags(DrawFlags, ECk_Jolt_DebugDrawFlags::ConstraintReferenceFrames))
    { InPhysicsSystem.DrawConstraintReferenceFrame(this); }

    EndCapture();
}

#endif

// --------------------------------------------------------------------------------------------------------------------
