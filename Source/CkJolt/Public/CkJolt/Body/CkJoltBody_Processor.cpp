#include "CkJoltBody_Processor.h"
#include "CkJoltBody_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Payload/CkPayload.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/PhysicsOwnership/CkPhysicsOwnership_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkJolt/CkJolt_ActivationEvent.h"
#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Stats.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/CkJoltShapeFactory.h"
#include "CkJolt/CollisionLayers/CkJoltCollisionLayerTable.h"
#include "CkJolt/CollisionLayers/CkJoltCollisionLayer_Utils.h"
#include "CkJolt/StaticWorld/CkJoltMeshShape_Utils.h"
#include "CkJolt/StaticWorld/CkJoltBakeExtraction.h"

#include "CkResourceLoader/CkResourceLoader_Utils.h"

#include <type_traits>

#include <Engine/StaticMesh.h>
#include <PhysicalMaterials/PhysicalMaterial.h>
#include <PhysicsEngine/BodySetup.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/EActivation.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/DecoratedShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_JoltBody_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltBody_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltBody_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltBody_SleepStateMirror);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltBody_KinematicPush);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltBody_WritebackInterpolated);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltBody_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("JoltBody_WritebackInterpolated"), STAT_CkJolt_Writeback, STATGROUP_CkJolt);
DECLARE_CYCLE_STAT(TEXT("JoltBody_KinematicPush"), STAT_CkJolt_KinematicPush, STATGROUP_CkJolt);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_body_processor
{
    // Consumer id — flip to Synchronous per-project in the ResourceLoader settings to debug.
    static const auto PreloadConsumerId = FName{TEXT("JoltBody.Setup")};

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

    auto
        Get_LeafShape(
            const JPH::Shape* InShape)
        -> const JPH::Shape*
    {
        auto* Leaf = InShape;
        for (;;)
        {
            const auto SubType = Leaf->GetSubType();
            if (SubType == JPH::EShapeSubType::RotatedTranslated ||
                SubType == JPH::EShapeSubType::Scaled ||
                SubType == JPH::EShapeSubType::OffsetCenterOfMass)
            {
                Leaf = static_cast<const JPH::DecoratedShape*>(Leaf)->GetInnerShape();
                continue;
            }

            return Leaf;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_JoltBody_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        const auto* LayerCtx = _TransientEntity.Get_RegistryView().TryGetContext<ck::jolt::FCk_Jolt_LayerContext>();

        _PhysicsSystem = ck_jolt_body_processor::TryResolve_PhysicsSystem(_TransientEntity);
        _LayerTable = LayerCtx != nullptr ? LayerCtx->_Table : nullptr;
        _JoltWorld = ck_jolt_body_processor::TryResolve_JoltWorld(_TransientEntity);

        const auto PinnedPhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PinnedPhysicsSystem) || _LayerTable == nullptr)
        { return; }

        _PendingActivate.Reset();
        _PendingDontActivate.Reset();

        TProcessor::DoTick(InDeltaT);

        if (_PendingActivate.IsEmpty() && _PendingDontActivate.IsEmpty())
        { return; }

        auto& BodyInterface = PinnedPhysicsSystem->GetBodyInterface();

        DoBatchAdd(BodyInterface, _PendingActivate, JPH::EActivation::Activate);
        DoBatchAdd(BodyInterface, _PendingDontActivate, JPH::EActivation::DontActivate);
    }

    auto
        FProcessor_JoltBody_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltBody_Params& InParams,
            FFragment_JoltBody_Current& InCurrent)
        -> void
    {
        using namespace JPH;

        if (InParams.Get_ShapeSource() == ECk_JoltBody_ShapeSource::StaticMeshAsset)
        {
            auto& PreloadBatch = InCurrent._MeshPreloadBatch;

            if (NOT PreloadBatch.Get_IsRequested() && ck::IsValid(InParams.Get_StaticMesh()))
            {
                PreloadBatch = UCk_Utils_ResourceLoader_UE::RequestLoad_RootedBatch(
                    ck_jolt_body_processor::PreloadConsumerId,
                    {InParams.Get_StaticMesh().ToSoftObjectPath()});
            }

            // Returning before the NeedsSetup removal below is what re-arms this Setup next tick; the
            // pending tag is not the dirty marker, so marking it cannot re-pump the frame.
            if (PreloadBatch.Get_IsRequested() && NOT PreloadBatch.Get_IsReady())
            {
                InHandle.AddOrGet<FTag_JoltBody_PendingAssetLoad>();
                return;
            }

            InHandle.Try_Remove<FTag_JoltBody_PendingAssetLoad>();
        }

        InHandle.Remove<MarkedDirtyBy>();

        const auto& EntityTransform = InHandle.Get<ck::FFragment_Transform>().Get_Transform();
        const auto EntityLocation = EntityTransform.GetLocation();
        const auto EntityRotation = EntityTransform.GetRotation();
        // _ShapeScale folds into the build scale for owners whose display scale lives outside the
        // entity transform (see its declaration). Setup-time only, like every other shape input.
        const auto EntityScale = EntityTransform.GetScale3D() * InParams.Get_ShapeScale();

        const auto DebugName = ck::Format_UE(TEXT("JoltBody [{}]"), InHandle);

        // ---- Shape ----
        auto* MeshBodySetup = static_cast<UBodySetup*>(nullptr);
        auto Shape = Ref<JPH::Shape>{};

        switch (InParams.Get_ShapeSource())
        {
            case ECk_JoltBody_ShapeSource::ExplicitShape:
            {
                Shape = ck::jolt::CreateShape_FromDimensions(InParams.Get_ShapeDimensions(), DebugName);
                break;
            }
            case ECk_JoltBody_ShapeSource::StaticMeshAsset:
            {
                const auto& PreloadBatch = InCurrent._MeshPreloadBatch;
                const auto PreloadFailed = PreloadBatch.Get_IsRequested() && PreloadBatch.Get_HasFailed();

                CK_ENSURE_IF_NOT(NOT PreloadFailed,
                    TEXT("JoltBody on Entity [{}]: preload of StaticMesh [{}] failed — the body is never created."),
                    InHandle, InParams.Get_StaticMesh().ToSoftObjectPath().ToString())
                { return; }

                // Batch-first so the cooked mesh is the one the batch roots; the resident-or-null
                // fallback covers params built raw with an already-loaded mesh.
                const UStaticMesh* Mesh = PreloadBatch.Get_IsRequested()
                    ? ::Cast<UStaticMesh>(PreloadBatch.Get_ResolvedObject(InParams.Get_StaticMesh().ToSoftObjectPath()))
                    : InParams.Get_StaticMesh().Get();

                CK_ENSURE_IF_NOT(ck::IsValid(Mesh),
                    TEXT("JoltBody on Entity [{}] uses ShapeSource StaticMeshAsset but has NO StaticMesh set (or it failed to resolve)."), InHandle)
                { return; }

                auto* BodySetup = Mesh->GetBodySetup();

                CK_ENSURE_IF_NOT(ck::IsValid(BodySetup),
                    TEXT("JoltBody on Entity [{}]: StaticMesh [{}] has NO BodySetup (no collision geometry)."),
                    InHandle, Mesh->GetName())
                { return; }

                MeshBodySetup = BodySetup;

                // Pre-baked mesh shape first (scale-1 blob + ScaledShape wrap); a null result —
                // missing/stale asset or a scale the topology cannot wrap — builds from the
                // BodySetup with the scale baked into the geometry, as always.
                const auto ScaleOneShape = ck::jolt::bake::mesh_shape_utils::TryGet_ScaleOneShape(*Mesh);
                Shape = ck::jolt::bake::mesh_shape_utils::TryWrap_AtScale(ScaleOneShape, EntityScale, DebugName);

                if (ck::Is_NOT_Valid(Shape))
                { Shape = ck::jolt::bake::BuildShape_FromBodySetup(*BodySetup, EntityScale, DebugName); }
                break;
            }
        }

        if (ck::Is_NOT_Valid(Shape))
        { return; }

        // ---- Trimesh-on-Dynamic guard: Jolt forbids a MeshShape leaf on a dynamic body ----
        if (InParams.Get_MotionType() == ECk_MotionType::Dynamic)
        {
            const auto* Leaf = ck_jolt_body_processor::Get_LeafShape(Shape.GetPtr());

            CK_ENSURE_IF_NOT(Leaf->GetSubType() != EShapeSubType::Mesh,
                TEXT("JoltBody on Entity [{}] has MotionType Dynamic but its shape resolves to a triangle Mesh "
                     "(complex/tri-mesh collision). Jolt forbids dynamic mesh bodies — use a primitive/convex "
                     "shape, or make the body Static/Kinematic."), InHandle)
            { return; }
        }

        // ---- Center of mass offset (wraps the shape) ----
        if (InParams.Get_ComSource() == ECk_JoltBody_ComSource::ExplicitOffset)
        {
            const auto ComSettings = OffsetCenterOfMassShapeSettings{ck::jolt::Conv(InParams.Get_ComOffset()), Shape.GetPtr()};
            const auto ComResult = ComSettings.Create();

            CK_ENSURE_IF_NOT(ComResult.IsValid(),
                TEXT("JoltBody on Entity [{}]: failed to build the COM-offset shape wrapper.\nJolt Error: [{}]"),
                InHandle, FString{ComResult.GetError().c_str()})
            { return; }

            Shape = ComResult.Get();
        }

        // ---- Object layer (profile-derived signature; mirrors the layer-table seeding) ----
        const auto Domain = InParams.Get_MotionType() == ECk_MotionType::Static
            ? ECk_Jolt_BodyDomain::Static
            : ECk_Jolt_BodyDomain::Dynamic;

        const auto MaybeSignature = ck::jolt::TryDerive_SignatureFromProfile(InParams.Get_CollisionProfileName(), Domain);

        CK_ENSURE_IF_NOT(MaybeSignature.IsSet(),
            TEXT("JoltBody on Entity [{}]: collision profile [{}] does not exist in UCollisionProfile (or has "
                 "collision disabled). Cannot assign a Jolt object layer."), InHandle, InParams.Get_CollisionProfileName())
        { return; }

        const auto Layer = _LayerTable->Get_OrRegisterLayer(*MaybeSignature);

        // Table exhaustion already fired Get_OrRegisterLayer's own ensure; an invalid layer reaching
        // BodyCreationSettings would silently create a body that collides with nothing.
        if (Layer == JPH::cObjectLayerInvalid)
        { return; }

        // ---- Body creation settings ----
        auto Settings = BodyCreationSettings{
            Shape,
            ck::jolt::Conv(EntityLocation),
            ck::jolt::Conv(EntityRotation),
            ck::jolt::Conv(InParams.Get_MotionType()),
            ObjectLayer{Layer}
        };
        Settings.mMotionQuality = ck::jolt::Conv(InParams.Get_MotionQuality());
        Settings.mGravityFactor = InParams.Get_GravityFactor();
        Settings.mLinearDamping = InParams.Get_LinearDamping();
        Settings.mAngularDamping = InParams.Get_AngularDamping();

        // Jolt's default is 500 in ITS units and Conv does not rescale, so a Jolt unit is a
        // centimetre here — leaving this unset caps every dynamic body at 5 m/s.
        Settings.mMaxLinearVelocity = InParams.Get_MaxLinearVelocity();

        switch (InParams.Get_MassSource())
        {
            case ECk_JoltBody_MassSource::FromShape:
            {
                // Jolt default: mass + inertia calculated from the shape density.
                break;
            }
            case ECk_JoltBody_MassSource::Explicit:
            {
                // ClampMin only guards the editor UI — C++/AS callers can set 0, and a non-positive mass reaches
                // Jolt's SetMassProperties (JPH_ASSERT in debug, inf inverse-mass -> NaN poses in shipping).
                CK_ENSURE_IF_NOT(InParams.Get_MassKg() > 0.0f,
                    TEXT("JoltBody on Entity [{}] has MassSource Explicit with non-positive MassKg [{}] — "
                         "falling back to shape-calculated mass."), InHandle, InParams.Get_MassKg())
                { break; }

                Settings.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
                Settings.mMassPropertiesOverride.mMass = InParams.Get_MassKg();
                break;
            }
            case ECk_JoltBody_MassSource::FromStaticMesh:
            {
                CK_ENSURE_IF_NOT(InParams.Get_ShapeSource() == ECk_JoltBody_ShapeSource::StaticMeshAsset &&
                    ck::IsValid(MeshBodySetup),
                    TEXT("JoltBody on Entity [{}] uses MassSource FromStaticMesh but has no StaticMesh BodySetup — "
                         "falling back to shape-calculated mass."), InHandle)
                { break; }

                const auto MeshMass = MeshBodySetup->CalculateMass();

                // A degenerate BodySetup can compute a zero mass — same NaN hazard as an explicit zero.
                CK_ENSURE_IF_NOT(MeshMass > 0.0f,
                    TEXT("JoltBody on Entity [{}]: StaticMesh BodySetup computed a non-positive mass [{}] — "
                         "falling back to shape-calculated mass."), InHandle, MeshMass)
                { break; }

                Settings.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
                Settings.mMassPropertiesOverride.mMass = MeshMass;
                break;
            }
        }

        switch (InParams.Get_SurfaceSource())
        {
            case ECk_JoltBody_SurfaceSource::PhysicalMaterial:
            {
                const UPhysicalMaterial* PhysMat = InParams.Get_PhysicalMaterial().Get();
                if (ck::IsValid(PhysMat))
                {
                    Settings.mFriction = PhysMat->Friction;
                    Settings.mRestitution = PhysMat->Restitution;
                }
                // Null material -> Jolt defaults (mFriction 0.2, mRestitution 0.0).
                break;
            }
            case ECk_JoltBody_SurfaceSource::Explicit:
            {
                Settings.mFriction = InParams.Get_Friction();
                Settings.mRestitution = InParams.Get_Restitution();
                break;
            }
        }

        // ---- Create the body (NOT added — the batched AddBodies pass runs after the view loop) ----
        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();
        const auto Body = BodyInterface.CreateBody(Settings);

        CK_ENSURE_IF_NOT(ck::IsValid(Body, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("JoltBody on Entity [{}]: CreateBody FAILED (max body count reached?)."), InHandle)
        {
            // Transient failure (slots free up as other bodies die) — re-arm setup so the entity retries
            // instead of being left half-composed forever.
            InHandle.Add<MarkedDirtyBy>();
            return;
        }

        Body->SetUserData(static_cast<uint64>(InHandle.Get_Entity().Get_ID()));

        InCurrent._BodyId = Body->GetID();
        InCurrent._Shape = Shape;

        // Seed the step-pose buffer so the very first interpolation reads a valid prev==curr==spawn pose.
        auto& StepPose = InHandle.Get<ck::FFragment_JoltBody_StepPose>();
        StepPose.Set_PrevLocation(EntityLocation)
                .Set_PrevRotation(EntityRotation)
                .Set_CurrLocation(EntityLocation)
                .Set_CurrRotation(EntityRotation);

        // A body that never activates is invisible to the debug draw's per-frame active pass, so it has to
        // arrive through the revision-keyed full pass — statics always, and anything spawned already asleep.
        const auto BodyStartsInactive = InParams.Get_MotionType() == ECk_MotionType::Static ||
            InParams.Get_InitialSleepState() == ECk_Jolt_SleepState::Asleep;

        if (BodyStartsInactive && _JoltWorld != nullptr)
        { _JoltWorld->Request_NoteStaticSceneChanged(); }

        const auto Pending = FPendingBody{InHandle.Get_Entity(), Body->GetID()};
        if (InParams.Get_InitialSleepState() == ECk_Jolt_SleepState::Awake)
        { _PendingActivate.Emplace(Pending); }
        else
        { _PendingDontActivate.Emplace(Pending); }
    }

    auto
        FProcessor_JoltBody_Setup::
        DoBatchAdd(
            JPH::BodyInterface& InBodyInterface,
            TArray<FPendingBody>& InPending,
            JPH::EActivation InActivation)
        -> void
    {
        if (InPending.IsEmpty())
        { return; }

        // Deterministic batch order across runs.
        InPending.Sort([](const FPendingBody& InA, const FPendingBody& InB)
        {
            return InA._Entity.Get_ID() < InB._Entity.Get_ID();
        });

        auto BodyIds = TArray<JPH::BodyID>{};
        BodyIds.Reserve(InPending.Num());
        for (const auto& Pending : InPending)
        { BodyIds.Emplace(Pending._BodyId); }

        // AddBodiesPrepare may shuffle BodyIds; the same (shuffled) array must be handed to Finalize.
        const auto AddState = InBodyInterface.AddBodiesPrepare(BodyIds.GetData(), BodyIds.Num());
        InBodyInterface.AddBodiesFinalize(BodyIds.GetData(), BodyIds.Num(), AddState, InActivation);

        for (const auto& Pending : InPending)
        {
            auto Handle = _TransientEntity.Get_ValidHandle(Pending._Entity.Get_ID());
            if (ck::Is_NOT_Valid(Handle) || NOT Handle.Has<ck::FFragment_JoltBody_Current>())
            { continue; }

            Handle.Get<ck::FFragment_JoltBody_Current>()._BodyAdded = true;
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_JoltBody_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        const auto* LayerCtx = _TransientEntity.Get_RegistryView().TryGetContext<ck::jolt::FCk_Jolt_LayerContext>();

        _PhysicsSystem = ck_jolt_body_processor::TryResolve_PhysicsSystem(_TransientEntity);
        _LayerTable = LayerCtx != nullptr ? LayerCtx->_Table : nullptr;
        _JoltWorld = ck_jolt_body_processor::TryResolve_JoltWorld(_TransientEntity);
        if (ck::Is_NOT_Valid(_PhysicsSystem.Pin()))
        { return; }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_JoltBody_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_JoltBody_Params& InParams,
            FFragment_JoltBody_Current& InCurrent,
            FFragment_JoltBody_Requests& InRequestsComp) const
        -> void
    {
        const auto RequestsCopy = InRequestsComp._Requests;
        InRequestsComp._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            // Every DoHandleRequest overload below is void and has no rejection path, so reaching the
            // line after the call IS the success condition.
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

            // The two runtime mutators additionally rewrite Params (motion type / profile name) so later
            // Get_MotionType and the EndPlay static-scene check agree with the live body.
            using RequestType = std::decay_t<decltype(InRequest)>;
            if constexpr (std::is_same_v<RequestType, FCk_Request_JoltBody_SetMotionType> ||
                          std::is_same_v<RequestType, FCk_Request_JoltBody_SetCollisionProfile>)
            { DoHandleRequest(InHandle, InParams, InCurrent, InRequest); }
            else
            { DoHandleRequest(InHandle, InCurrent, InRequest); }

            if (InRequest.Get_IsRequestHandleValid())
            {
                InRequest.GetAndDestroyRequestHandle();
            }

            Result = ECk_Request_OperationResult::Succeeded;
        }), policy::DontResetContainer{});

        if (InRequestsComp._Requests.IsEmpty())
        {
            InHandle.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_JoltBody_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_SetSleepState& InRequest) const
        -> void
    {
        if (NOT InCurrent.Get_BodyAdded())
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        switch (InRequest.Get_SleepState())
        {
            case ECk_Jolt_SleepState::Awake:
            {
                BodyInterface.ActivateBody(InCurrent.Get_BodyId());
                break;
            }
            case ECk_Jolt_SleepState::Asleep:
            {
                BodyInterface.DeactivateBody(InCurrent.Get_BodyId());
                break;
            }
        }
    }

    auto
        FProcessor_JoltBody_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_AddForce& InRequest) const
        -> void
    {
        if (NOT InCurrent.Get_BodyAdded())
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        // AddForce defaults to EActivation::Activate — a settled body wakes to receive the force.
        BodyInterface.AddForce(InCurrent.Get_BodyId(), ck::jolt::Conv(InRequest.Get_Force()));
    }

    auto
        FProcessor_JoltBody_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_AddForceAtLocation& InRequest) const
        -> void
    {
        if (NOT InCurrent.Get_BodyAdded())
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        BodyInterface.AddForce(
            InCurrent.Get_BodyId(),
            ck::jolt::Conv(InRequest.Get_Force()),
            ck::jolt::Conv(InRequest.Get_WorldLocation()));
    }

    auto
        FProcessor_JoltBody_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_AddTorque& InRequest) const
        -> void
    {
        if (NOT InCurrent.Get_BodyAdded())
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        BodyInterface.AddTorque(InCurrent.Get_BodyId(), ck::jolt::Conv(InRequest.Get_Torque()));
    }

    auto
        FProcessor_JoltBody_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_AddImpulse& InRequest) const
        -> void
    {
        if (NOT InCurrent.Get_BodyAdded())
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        // BodyInterface::AddImpulse activates the body internally if it is asleep.
        BodyInterface.AddImpulse(InCurrent.Get_BodyId(), ck::jolt::Conv(InRequest.Get_Impulse()));
    }

    auto
        FProcessor_JoltBody_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_AddImpulseAtLocation& InRequest) const
        -> void
    {
        if (NOT InCurrent.Get_BodyAdded())
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        BodyInterface.AddImpulse(
            InCurrent.Get_BodyId(),
            ck::jolt::Conv(InRequest.Get_Impulse()),
            ck::jolt::Conv(InRequest.Get_WorldLocation()));
    }

    auto
        FProcessor_JoltBody_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_AddAngularImpulse& InRequest) const
        -> void
    {
        if (NOT InCurrent.Get_BodyAdded())
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        BodyInterface.AddAngularImpulse(InCurrent.Get_BodyId(), ck::jolt::Conv(InRequest.Get_AngularImpulse()));
    }

    auto
        FProcessor_JoltBody_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_SetLinearVelocity& InRequest) const
        -> void
    {
        if (NOT InCurrent.Get_BodyAdded())
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        // Set*Velocity activates the body if needed (per Jolt's BodyInterface contract).
        BodyInterface.SetLinearVelocity(InCurrent.Get_BodyId(), ck::jolt::Conv(InRequest.Get_LinearVelocity()));
    }

    auto
        FProcessor_JoltBody_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_SetAngularVelocity& InRequest) const
        -> void
    {
        if (NOT InCurrent.Get_BodyAdded())
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        BodyInterface.SetAngularVelocity(InCurrent.Get_BodyId(), ck::jolt::Conv(InRequest.Get_AngularVelocity()));
    }

    auto
        FProcessor_JoltBody_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_Teleport& InRequest) const
        -> void
    {
        if (NOT InCurrent.Get_BodyAdded())
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        const auto NewLocation = InRequest.Get_Location();
        const auto NewRotation = InRequest.Get_Rotation().Quaternion();

        // Activate: a settled body must wake, else the teleport is silently dropped on a sleeping body.
        BodyInterface.SetPositionAndRotation(
            InCurrent.Get_BodyId(),
            ck::jolt::Conv(NewLocation),
            ck::jolt::Conv(NewRotation),
            JPH::EActivation::Activate);

        switch (InRequest.Get_VelocityPolicy())
        {
            case ECk_Jolt_TeleportVelocityPolicy::ResetVelocity:
            {
                BodyInterface.SetLinearAndAngularVelocity(InCurrent.Get_BodyId(), JPH::Vec3::sZero(), JPH::Vec3::sZero());
                break;
            }
            case ECk_Jolt_TeleportVelocityPolicy::KeepVelocity:
            {
                break;
            }
        }

        // A Static body never activates, so the debug draw's per-frame active pass will not notice it moved —
        // only the revision-keyed full pass can.
        if (_JoltWorld != nullptr && InHandle.Has<ck::FTag_JoltBody_MotionType_Static>())
        { _JoltWorld->Request_NoteStaticSceneChanged(); }

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

        // The step-pose snap alone is not enough: the FJoltWorld pose buffer still holds the PRE-teleport Curr,
        // which the next capture+apply would sweep the entity back across. Reaping re-seeds it prev==curr.
        if (_JoltWorld != nullptr)
        {
            _JoltWorld->Remove_PoseBufferEntry(InCurrent.Get_BodyId().GetIndexAndSequenceNumber());
        }
    }

    auto
        FProcessor_JoltBody_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_JoltBody_Params& InParams,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_SetMotionType& InRequest) const
        -> void
    {
        if (NOT InCurrent.Get_BodyAdded())
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        const auto OldMotionType = InParams.Get_MotionType();
        const auto NewMotionType = InRequest.Get_MotionType();

        if (OldMotionType == NewMotionType)
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        // A Static body is inert by definition, so it is never activated; Dynamic and Kinematic both are,
        // otherwise a body switched out of Static while the simulation is settled would sit frozen until
        // something else happened to wake it.
        const auto Activation = NewMotionType == ECk_MotionType::Static
            ? JPH::EActivation::DontActivate
            : JPH::EActivation::Activate;

        BodyInterface.SetMotionType(InCurrent.Get_BodyId(), ck::jolt::Conv(NewMotionType), Activation);

        InParams.Set_MotionType(NewMotionType);

        // Re-stamp the ECS-side mirrors. FTag_JoltBody_KinematicFromECS is the SELECTOR that puts a body
        // into FProcessor_JoltBody_KinematicPush and excludes it from FProcessor_JoltBody_WritebackInterpolated;
        // leaving it stale would have the same body driven FROM the ECS transform and written back INTO it in
        // one frame. The MotionType_* tags feed the Teleport static-scene check and the debug draw's colouring.
        InHandle.Try_Remove<ck::FTag_JoltBody_MotionType_Static>();
        InHandle.Try_Remove<ck::FTag_JoltBody_MotionType_Kinematic>();
        InHandle.Try_Remove<ck::FTag_JoltBody_MotionType_Dynamic>();
        InHandle.Try_Remove<ck::FTag_JoltBody_KinematicFromECS>();

        switch (NewMotionType)
        {
            case ECk_MotionType::Static:
            {
                InHandle.AddOrGet<ck::FTag_JoltBody_MotionType_Static>();
                break;
            }
            case ECk_MotionType::Kinematic:
            {
                InHandle.AddOrGet<ck::FTag_JoltBody_MotionType_Kinematic>();
                InHandle.AddOrGet<ck::FTag_JoltBody_KinematicFromECS>();
                break;
            }
            case ECk_MotionType::Dynamic:
            {
                InHandle.AddOrGet<ck::FTag_JoltBody_MotionType_Dynamic>();
                break;
            }
        }

        // Sleep mirror: a body that just became Static or Kinematic will never fire OnBodyDeactivated, and one
        // that just became Dynamic was activated above. Either way the Sleeping tag from a previous life is now
        // wrong; drop it and let the activation-event mirror re-stamp it when the body actually settles.
        InHandle.Try_Remove<ck::FTag_JoltBody_Sleeping>();

        // Entering OR leaving Static changes what the revision-keyed full pass must reconcile: a body that just
        // became Static will never activate again (only the full pass can notice it), and one that just left
        // Static must be released from the inactive set.
        if (_JoltWorld != nullptr &&
            (OldMotionType == ECk_MotionType::Static || NewMotionType == ECk_MotionType::Static))
        { _JoltWorld->Request_NoteStaticSceneChanged(); }
    }

    auto
        FProcessor_JoltBody_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_JoltBody_Params& InParams,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_SetCollisionProfile& InRequest) const
        -> void
    {
        if (NOT InCurrent.Get_BodyAdded())
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        CK_ENSURE_IF_NOT(_LayerTable != nullptr,
            TEXT("JoltBody on Entity [{}]: no Jolt layer table context. Cannot resolve collision profile [{}]."),
            InHandle, InRequest.Get_CollisionProfileName())
        { return; }

        const auto NewProfileName = InRequest.Get_CollisionProfileName();

        if (InParams.Get_CollisionProfileName() == NewProfileName)
        { return; }

        // Same resolution path FProcessor_JoltBody_Setup uses — deliberately NOT a second implementation, so a
        // profile can never mean one thing at creation and another at runtime. The domain is derived from the
        // body's CURRENT motion type, which SetMotionType above keeps truthful.
        const auto Domain = InParams.Get_MotionType() == ECk_MotionType::Static
            ? ECk_Jolt_BodyDomain::Static
            : ECk_Jolt_BodyDomain::Dynamic;

        const auto MaybeSignature = ck::jolt::TryDerive_SignatureFromProfile(NewProfileName, Domain);

        CK_ENSURE_IF_NOT(MaybeSignature.IsSet(),
            TEXT("JoltBody on Entity [{}]: collision profile [{}] does not exist in UCollisionProfile (or has "
                 "collision disabled). Keeping the previous object layer."), InHandle, NewProfileName)
        { return; }

        const auto Layer = _LayerTable->Get_OrRegisterLayer(*MaybeSignature);

        // Table exhaustion already fired Get_OrRegisterLayer's own ensure; moving the body onto an invalid layer
        // would silently make it collide with nothing, which is worse than keeping the old one.
        if (Layer == JPH::cObjectLayerInvalid)
        { return; }

        PhysicsSystem->GetBodyInterface().SetObjectLayer(InCurrent.Get_BodyId(), Layer);

        InParams.Set_CollisionProfileName(NewProfileName);
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_JoltBody_SleepStateMirror::
        FProcessor_JoltBody_SleepStateMirror(
            const RegistryType& InRegistry)
        : Super(InRegistry)
    {
    }

    auto
        FProcessor_JoltBody_SleepStateMirror::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        auto* JoltWorld = ck_jolt_body_processor::TryResolve_JoltWorld(_TransientEntity);
        if (JoltWorld == nullptr)
        { return; }

        auto Events = TArray<FCk_Jolt_ActivationEvent>{};
        JoltWorld->DrainActivationEvents(Events);

        if (Events.IsEmpty())
        { return; }

        const auto RegView = _TransientEntity.Get_RegistryView();

        for (const auto& Event : Events)
        {
            // A snapshot load can leave a queued event pointing at a dead entity id — liveness check, no ensure.
            const auto Entity = FCk_Entity{FCk_Entity::IdType{static_cast<FCk_Entity::IdType>(Event.UserData)}};
            if (NOT RegView.IsValid(Entity))
            { continue; }

            auto Handle = _TransientEntity.Get_ValidHandle(Entity.Get_ID());
            if (ck::Is_NOT_Valid(Handle) || NOT Handle.Has<ck::FFragment_JoltBody_Current>())
            { continue; }

            // An entity may own more Jolt bodies than its JoltBody (e.g. a Probe), all sharing the entity id.
            if (Handle.Get<ck::FFragment_JoltBody_Current>().Get_BodyId().GetIndexAndSequenceNumber() != Event.BodyIndexAndSeq)
            { continue; }

            switch (Event.NewState)
            {
                case ECk_Jolt_SleepState::Asleep:
                {
                    Handle.AddOrGet<ck::FTag_JoltBody_Sleeping>();

                    // A sleeping body produces no new step poses — collapse prev onto curr so the interpolator
                    // holds it steady instead of blending a stale prev for a frame.
                    auto& StepPose = Handle.Get<ck::FFragment_JoltBody_StepPose>();
                    StepPose.Set_PrevLocation(StepPose.Get_CurrLocation())
                            .Set_PrevRotation(StepPose.Get_CurrRotation());
                    break;
                }
                case ECk_Jolt_SleepState::Awake:
                {
                    Handle.Try_Remove<ck::FTag_JoltBody_Sleeping>();
                    break;
                }
            }

            auto JoltBodyHandle = UCk_Utils_JoltBody_UE::Cast(Handle);
            UUtils_Signal_OnJoltBodySleepStateChanged::Broadcast(JoltBodyHandle, ck::MakePayload(JoltBodyHandle, Event.NewState));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_JoltBody_KinematicPush::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        auto* JoltWorld = ck_jolt_body_processor::TryResolve_JoltWorld(_TransientEntity);
        if (JoltWorld == nullptr)
        { return; }

        // Zero-step frame: no sim time to move a kinematic body across. The move is not lost — the push runs
        // for every added kinematic body on each stepping frame, not gated on a transform-dirty tag.
        if (JoltWorld->Get_PendingSimTime() <= 0.0f)
        { return; }

        _PhysicsSystem = ck_jolt_body_processor::TryResolve_PhysicsSystem(_TransientEntity);
        if (ck::Is_NOT_Valid(_PhysicsSystem.Pin()))
        { return; }

        SCOPE_CYCLE_COUNTER(STAT_CkJolt_KinematicPush);

        _PendingSimTime = JoltWorld->Get_PendingSimTime();

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_JoltBody_KinematicPush::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FFragment_Transform& InTransform) const
        -> void
    {
        if (NOT InCurrent.Get_BodyAdded())
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        const auto& Transform = InTransform.Get_Transform();

        BodyInterface.MoveKinematic(
            InCurrent.Get_BodyId(),
            ck::jolt::Conv(Transform.GetLocation()),
            ck::jolt::Conv(Transform.GetRotation()),
            _PendingSimTime);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_JoltBody_WritebackInterpolated::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        auto* JoltWorld = ck_jolt_body_processor::TryResolve_JoltWorld(_TransientEntity);
        if (JoltWorld == nullptr)
        { return; }

        SCOPE_CYCLE_COUNTER(STAT_CkJolt_Writeback);

        _Alpha = JoltWorld->Get_Alpha();

        TParallelProcessor::DoTick(InDeltaT);

        // Single-threaded post-pass: consuming the dirty tag here runs the writeback once per refreshed step pose.
        _TransientEntity.Clear<ck::FTag_JoltBody_TransformDirty>();
    }

    auto
        FProcessor_JoltBody_WritebackInterpolated::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InTransform,
            FFragment_Transform_Previous& InPrevTransform,
            const FFragment_JoltBody_StepPose& InStepPose) const
        -> void
    {
        const auto Location = FMath::Lerp(InStepPose.Get_PrevLocation(), InStepPose.Get_CurrLocation(), _Alpha);
        const auto Rotation = FQuat::Slerp(InStepPose.Get_PrevRotation(), InStepPose.Get_CurrRotation(), _Alpha).GetNormalized();
        const auto ExistingScale = InTransform.Get_Transform().GetScale3D();

        const auto ComponentsModified = UCk_Utils_Transform_UE::Apply_SetTransform_DirectWrite(
            InTransform, InPrevTransform, FTransform{Rotation, Location, ExistingScale});

        if (EnumHasAnyFlags(ComponentsModified,
            ECk_TransformComponents::Location |
            ECk_TransformComponents::Rotation |
            ECk_TransformComponents::Scale))
        {
            InHandle.template DeferAddOrGet<FTag_Transform_Updated>();
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_JoltBody_EndPlay::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _JoltWorld = ck_jolt_body_processor::TryResolve_JoltWorld(_TransientEntity);
        _PhysicsSystem = ck_jolt_body_processor::TryResolve_PhysicsSystem(_TransientEntity);

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_JoltBody_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltBody_Params& InParams,
            FFragment_JoltBody_Current& InCurrent) const
        -> void
    {
        // ASYNC GUARD: FGroup_EndPlay runs later in the SAME tick that kicked this frame's async step, and the
        // teardown below mutates the pose buffer + bodies the task-graph loop is still touching. Self-guarded
        // on future validity, so this is free in sync mode and after the first dying entity.
        if (_JoltWorld != nullptr && _JoltWorld->Get_AsyncFuture().IsValid())
        { _JoltWorld->WaitForAsyncStep(); }

        // Ownership releases unconditionally — it was claimed at Add, before any body existed.
        auto ReleaseHandle = InHandle;
        ck::physics_ownership::Release_Jolt(ReleaseHandle);

        const auto& BodyId = InCurrent.Get_BodyId();

        // Setup never created a body (ensure-skip path, or no Jolt subsystem): nothing Jolt-side to free, and
        // demanding a PhysicsSystem here would turn that legal state into an ensure.
        if (BodyId.IsInvalid())
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();

        CK_ENSURE_IF_NOT(ck::IsValid(PhysicsSystem),
            TEXT("PhysicsSystem is INVALID during JoltBody [{}] teardown — subsystem already deinitialized?"), InHandle)
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        // RemoveBody only detaches from the broadphase; DestroyBody frees the SLOT and must run for every body,
        // whether it was ever added or not.
        if (BodyInterface.IsAdded(BodyId))
        {
            BodyInterface.RemoveBody(BodyId);
        }

        BodyInterface.DestroyBody(BodyId);

        if (_JoltWorld != nullptr)
        {
            _JoltWorld->Remove_PoseBufferEntry(BodyId.GetIndexAndSequenceNumber());

            // Every motion type, not just Static: a SLEEPING dynamic body is invisible to both of the debug
            // draw's body passes, so its released slots would outlive it unless the sweep is told to run.
            _JoltWorld->Request_NoteBodyRemoved();

            if (InParams.Get_MotionType() == ECk_MotionType::Static)
            { _JoltWorld->Request_NoteStaticSceneChanged(); }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_JoltBody_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltBody_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
