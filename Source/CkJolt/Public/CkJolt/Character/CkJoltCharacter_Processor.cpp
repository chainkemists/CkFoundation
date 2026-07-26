#include "CkJoltCharacter_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid_Defaults.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/PhysicsOwnership/CkPhysicsOwnership_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkJolt/Body/CkJoltBody_Fragment.h"
#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/CkJoltShapeFactory.h"
#include "CkJolt/CollisionLayers/CkJoltCollisionLayerTable.h"
#include "CkJolt/CollisionLayers/CkJoltCollisionLayer_Utils.h"

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_JoltCharacter_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltCharacter_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltCharacter_PreStep);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltCharacter_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_character_processor
{
    // A world with no Jolt subsystem never publishes the context — absent/null is legal, never an error.
    auto
        TryResolve_JoltWorld(
            const FCk_Handle& InTransientEntity)
        -> ck::FJoltWorld*
    {
        const auto* WorldPtr = InTransientEntity.Get_RegistryView().TryGetContext<TSharedPtr<ck::FJoltWorld>>();
        if (WorldPtr == nullptr || NOT WorldPtr->IsValid())
        { return nullptr; }

        return WorldPtr->Get();
    }

    auto
        TryResolve_PhysicsSystem(
            const FCk_Handle& InTransientEntity)
        -> TWeakPtr<JPH::PhysicsSystem>
    {
        const auto* Ctx = InTransientEntity.Get_RegistryView().TryGetContext<TWeakPtr<JPH::PhysicsSystem>>();
        if (Ctx == nullptr)
        { return {}; }

        return *Ctx;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_JoltCharacter_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        const auto* LayerCtx = _TransientEntity.Get_RegistryView().TryGetContext<ck::jolt::FCk_Jolt_LayerContext>();

        _PhysicsSystem = ck_jolt_character_processor::TryResolve_PhysicsSystem(_TransientEntity);
        _LayerTable = LayerCtx != nullptr ? LayerCtx->_Table : nullptr;
        _JoltWorld = ck_jolt_character_processor::TryResolve_JoltWorld(_TransientEntity);

        const auto PinnedPhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PinnedPhysicsSystem) || _LayerTable == nullptr || _JoltWorld == nullptr)
        { return; }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_JoltCharacter_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltCharacter_Params& InParams,
            FFragment_JoltCharacter_Current& InCurrent)
        -> void
    {
        using namespace JPH;

        InHandle.Remove<MarkedDirtyBy>();

        const auto& EntityTransform = InHandle.Get<ck::FFragment_Transform>().Get_Transform();
        const auto EntityLocation = EntityTransform.GetLocation();
        const auto EntityRotation = EntityTransform.GetRotation();

        const auto DebugName = ck::Format_UE(TEXT("JoltCharacter [{}]"), InHandle);

        // ---- Capsule shape (Z-up upright; axis correction is internal to the shape factory) ----
        auto ShapeDimensions = FCk_Jolt_ShapeDimensions{ECk_Jolt_ShapeType::Capsule};
        ShapeDimensions.Set_Radius(InParams.Get_CapsuleRadius());
        ShapeDimensions.Set_HalfHeight(InParams.Get_CapsuleHalfHeight());

        auto Shape = ck::jolt::CreateShape_FromDimensions(ShapeDimensions, DebugName);
        if (ck::Is_NOT_Valid(Shape.GetPtr(), ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        // ---- Object layer (profile-derived signature; a character is dynamic-domain) ----
        const auto MaybeSignature = ck::jolt::TryDerive_SignatureFromProfile(InParams.Get_CollisionProfileName(), ECk_Jolt_BodyDomain::Dynamic);

        CK_ENSURE_IF_NOT(MaybeSignature.IsSet(),
            TEXT("JoltCharacter on Entity [{}]: collision profile [{}] does not exist in UCollisionProfile (or has "
                 "collision disabled). Cannot assign a Jolt object layer."), InHandle, InParams.Get_CollisionProfileName())
        { return; }

        const auto Layer = _LayerTable->Get_OrRegisterLayer(*MaybeSignature);

        // Table exhaustion already fired Get_OrRegisterLayer's own ensure; an invalid layer reaching the
        // character would make it sweep against nothing.
        if (Layer == JPH::cObjectLayerInvalid)
        { return; }

        // ---- CharacterVirtualSettings ----
        // Jolt's CharacterVirtual defaults are Y-up METRES; this world is Z-up passthrough CENTIMETRES. The
        // support plane is measured from our CENTERED capsule, not the Jolt sample's base-at-origin shape —
        // the sample's -radius constant would let waist-height ledge lips read as ground.
        constexpr auto PredictiveContactDistance_Uu = 10.0f;
        constexpr auto CharacterPadding_Uu          = 2.0f;
        constexpr auto CollisionTolerance_Uu        = 0.1f;
        constexpr auto NewtonsToKgUuPerSecondSq     = 100.0f;

        auto Settings = CharacterVirtualSettings{};
        Settings.mShape = Shape;
        Settings.mUp = Vec3(0.0f, 0.0f, 1.0f);
        Settings.mSupportingVolume = Plane(Vec3::sAxisZ(), InParams.Get_CapsuleHalfHeight());
        Settings.mMaxSlopeAngle = DegreesToRadians(InParams.Get_MaxSlopeAngleDegrees());
        Settings.mMass = InParams.Get_MassKg();
        Settings.mMaxStrength = InParams.Get_MaxStrengthNewtons() * NewtonsToKgUuPerSecondSq;
        Settings.mPredictiveContactDistance = PredictiveContactDistance_Uu;
        Settings.mCharacterPadding = CharacterPadding_Uu;
        Settings.mCollisionTolerance = CollisionTolerance_Uu;

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        const auto EntityId = static_cast<uint64>(InHandle.Get_Entity().Get_ID());

        auto Character = Ref<CharacterVirtual>{new CharacterVirtual(
            &Settings, ck::jolt::Conv(EntityLocation), ck::jolt::Conv(EntityRotation), EntityId, PhysicsSystem.Get())};

        Character->SetListener(_JoltWorld->Get_CharacterContactListener());

        InCurrent._Character = Character;
        InCurrent._ObjectLayer = Layer;

        auto Entry = FCk_Jolt_CharacterEntry{};
        Entry.Character = Character.GetPtr();
        Entry.UserData = EntityId;
        Entry.ObjectLayer = Layer;
        Entry.PushPolicy = InParams.Get_PushPolicy();
        Entry.OutLocation = EntityLocation;
        Entry.OutRotation = EntityRotation;
        _JoltWorld->Register_Character(Entry);

        // Seed prev==curr==spawn so the first interpolation reads a valid pose.
        auto& StepPose = InHandle.Get<ck::FFragment_JoltBody_StepPose>();
        StepPose.Set_PrevLocation(EntityLocation)
                .Set_PrevRotation(EntityRotation)
                .Set_CurrLocation(EntityLocation)
                .Set_CurrRotation(EntityRotation);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_JoltCharacter_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _JoltWorld = ck_jolt_character_processor::TryResolve_JoltWorld(_TransientEntity);

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_JoltCharacter_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_JoltCharacter_Current& InCurrent,
            FFragment_JoltCharacter_Requests& InRequestsComp) const
        -> void
    {
        const auto RequestsCopy = InRequestsComp._Requests;
        InRequestsComp._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            DoHandleRequest(InHandle, InCurrent, InRequest);

            if (InRequest.Get_IsRequestHandleValid())
            {
                InRequest.GetAndDestroyRequestHandle();
            }
        }), policy::DontResetContainer{});

        if (InRequestsComp._Requests.IsEmpty())
        {
            InHandle.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_JoltCharacter_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_JoltCharacter_Current& InCurrent,
            const FCk_Request_JoltCharacter_Move& InRequest) const
        -> void
    {
        // Continuous intent: stored on Current, drained into the FJoltWorld entry by PreStep each frame.
        InCurrent._PendingMoveVelocity = InRequest.Get_DesiredVelocity();
    }

    auto
        FProcessor_JoltCharacter_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_JoltCharacter_Current& InCurrent,
            const FCk_Request_JoltCharacter_Jump& InRequest) const
        -> void
    {
        // One-shot intent: armed on Current, transferred (once) into the entry by PreStep, consumed by the step.
        InCurrent._PendingJumpVelocity = InRequest.Get_JumpVelocity();
        InCurrent._HasPendingJump = true;
    }

    auto
        FProcessor_JoltCharacter_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_JoltCharacter_Current& InCurrent,
            const FCk_Request_JoltCharacter_Teleport& InRequest) const
        -> void
    {
        auto* Character = InCurrent._Character.GetPtr();
        if (ck::Is_NOT_Valid(Character, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        const auto NewLocation = InRequest.Get_Location();
        Character->SetPosition(ck::jolt::Conv(NewLocation));

        auto NewRotation = ck::jolt::Conv(Character->GetRotation());
        if (InRequest.Get_AlsoSetRotation() == ECk_EnableDisable::Enable)
        {
            NewRotation = InRequest.Get_Rotation().Quaternion();
            Character->SetRotation(ck::jolt::Conv(NewRotation));
        }

        auto& TransformFragment = InHandle.Get<ck::FFragment_Transform>();
        auto& PrevTransformFragment = InHandle.Get<ck::FFragment_Transform_Previous>();

        const auto ExistingScale = TransformFragment.Get_Transform().GetScale3D();

        const auto ComponentsModified = UCk_Utils_Transform_UE::Apply_SetTransform_DirectWrite(
            TransformFragment, PrevTransformFragment, FTransform{NewRotation, NewLocation, ExistingScale});

        if (EnumHasAnyFlags(ComponentsModified,
            ECk_TransformComponents::Location |
            ECk_TransformComponents::Rotation |
            ECk_TransformComponents::Scale))
        {
            InHandle.AddOrGet<ck::FTag_Transform_Updated>();
        }

        auto& StepPose = InHandle.Get<ck::FFragment_JoltBody_StepPose>();
        StepPose.Set_PrevLocation(NewLocation)
                .Set_PrevRotation(NewRotation)
                .Set_CurrLocation(NewLocation)
                .Set_CurrRotation(NewRotation);

        // The step-pose snap alone is not enough: the FJoltWorld entry still holds the PRE-teleport out-pose,
        // which the next apply would sweep the character back across. The entry itself must survive (it holds
        // the live Character), so its out-pose is snapped rather than reaped.
        if (_JoltWorld != nullptr)
        {
            const auto EntityId = static_cast<uint64>(InHandle.Get_Entity().Get_ID());
            _JoltWorld->Snap_CharacterOutPose(EntityId, NewLocation, NewRotation);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_JoltCharacter_PreStep::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _JoltWorld = ck_jolt_character_processor::TryResolve_JoltWorld(_TransientEntity);
        if (_JoltWorld == nullptr)
        { return; }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_JoltCharacter_PreStep::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltCharacter_Params& InParams,
            FFragment_JoltCharacter_Current& InCurrent) const
        -> void
    {
        const auto EntityId = static_cast<uint64>(InHandle.Get_Entity().Get_ID());

        _JoltWorld->Push_CharacterIntent(
            EntityId,
            InCurrent._PendingMoveVelocity,
            InParams.Get_PushPolicy(),
            InCurrent._HasPendingJump,
            InCurrent._PendingJumpVelocity);

        // The jump is armed on the entry now, so clear the inbox flag: transferred exactly once, never re-armed.
        InCurrent._HasPendingJump = false;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_JoltCharacter_EndPlay::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _JoltWorld = ck_jolt_character_processor::TryResolve_JoltWorld(_TransientEntity);

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_JoltCharacter_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltCharacter_Params& InParams,
            FFragment_JoltCharacter_Current& InCurrent) const
        -> void
    {
        // ASYNC GUARD: FGroup_EndPlay runs later in the SAME tick that kicked this frame's async step, and
        // unregistering + dropping the Ref while the task-graph loop iterates the registry is a data race and
        // use-after-free. Self-guarded on future validity, so this is free in sync mode.
        if (_JoltWorld != nullptr && _JoltWorld->Get_AsyncFuture().IsValid())
        { _JoltWorld->WaitForAsyncStep(); }

        // Ownership releases unconditionally — it was claimed at Add, before any character existed.
        auto ReleaseHandle = InHandle;
        ck::physics_ownership::Release_Jolt(ReleaseHandle);

        // Unregister BEFORE dropping the Ref, else the step loop / contact listener sees a dangling pointer.
        if (_JoltWorld != nullptr)
        {
            const auto EntityId = static_cast<uint64>(InHandle.Get_Entity().Get_ID());
            _JoltWorld->Unregister_Character(EntityId);
        }

        // Dropping the owning Ref destroys the CharacterVirtual; it is NOT in the body interface, so unlike a
        // JoltBody there is no body to Remove/Destroy.
        InCurrent._Character = nullptr;
    }
}

// --------------------------------------------------------------------------------------------------------------------
