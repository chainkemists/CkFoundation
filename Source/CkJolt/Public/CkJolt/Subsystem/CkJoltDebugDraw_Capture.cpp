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
#include "CkJolt/StaticWorld/CkJoltStaticActor_Fragment.h"
#include "CkJolt/Subsystem/CkJolt_DebugDrawTarget_Impl.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Jolt_DebugDraw_Capture"), STAT_CkJolt_DebugDrawCapture, STATGROUP_CkJolt);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_debugdraw_capture
{
    // Bodies and characters share one slot map; a CharacterVirtual has no BodyID, so its key is its entity id
    // lifted clear of the 32-bit BodyID (index + sequence) keyspace.
    constexpr uint64 CharacterKeyBit = uint64{1} << 40;

    // ----------------------------------------------------------------------------------------------------------------

    struct FContext_Capture
    {
        FCk_Jolt_DebugRenderer* _Renderer = nullptr;
        FCk_Jolt_DebugDrawTarget::FImpl* _TargetImpl = nullptr;
        JPH::PhysicsSystem* _PhysicsSystem = nullptr;
        const JPH::BodyLockInterfaceNoLock* _LockInterface = nullptr;
        FCk_Handle _TransientEntity;
        uint64 _StaticSceneRevision = 0;
        TSet<uint64> _ActiveKeys;
        bool _FullPassRanThisCapture = false;
    };

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_BodyKey(
            const JPH::BodyID& InBodyId)
        -> uint64
    {
        return static_cast<uint64>(InBodyId.GetIndexAndSequenceNumber());
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
    // JoltStaticActor entity (CkJolt/CLAUDE.md § "ECS-first attribution").
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
        Get_ColorClass(
            const JPH::Body& InBody,
            const FCk_Handle& InTransientEntity)
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
                return Get_IsBakedStaticWorldBody(InBody, InTransientEntity)
                    ? ECk_Jolt_DebugDraw_ColorClass::BakedStatic
                    : ECk_Jolt_DebugDraw_ColorClass::Static;
            }
        }
    }

    auto
        Draw_Body(
            FContext_Capture& InCtx,
            const JPH::Body& InBody)
        -> void
    {
        const auto ColorClass = Get_ColorClass(InBody, InCtx._TransientEntity);
        const auto Color = ck::jolt::Conv(InCtx._TargetImpl->_Palette.Get_Color(ColorClass));

        InCtx._Renderer->BeginBody(Get_BodyKey(InBody.GetID()), ColorClass);

        constexpr auto UseMaterialColors = false;
        constexpr auto DrawWireframe = false;
        InBody.GetShape()->Draw(InCtx._Renderer, InBody.GetCenterOfMassTransform(), JPH::Vec3::sReplicate(1.0f),
            Color, UseMaterialColors, DrawWireframe);

        InCtx._Renderer->EndBody();

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

        const auto SceneChanged = NOT TargetImpl._FullPassEverRan ||
            TargetImpl._CapturedStaticSceneRevision != InCtx._StaticSceneRevision;

        if (NOT SceneChanged)
        { return ck::EStepResult::Continue; }

        auto PreviouslyRetainedKeys = MoveTemp(TargetImpl._StaticBodyKeys);
        PreviouslyRetainedKeys.Append(MoveTemp(TargetImpl._SleepingBodyKeys));

        TargetImpl._StaticBodyKeys.Reset();
        TargetImpl._SleepingBodyKeys.Reset();

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

            Draw_Body(InCtx, *Body);

            const auto Key = Get_BodyKey(BodyId);

            if (Body->GetMotionType() == JPH::EMotionType::Static)
            { TargetImpl._StaticBodyKeys.Emplace(Key); }
            else
            { TargetImpl._SleepingBodyKeys.Emplace(Key); }
        }

        for (const auto StaleKey : PreviouslyRetainedKeys)
        {
            if (TargetImpl._StaticBodyKeys.Contains(StaleKey) || TargetImpl._SleepingBodyKeys.Contains(StaleKey))
            { continue; }

            InCtx._Renderer->Release_BodySlots(StaleKey);
        }

        TargetImpl._CapturedStaticSceneRevision = InCtx._StaticSceneRevision;
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

            Draw_Body(InCtx, *Body);

            const auto Key = Get_BodyKey(BodyId);
            InCtx._ActiveKeys.Emplace(Key);
            TargetImpl._SleepingBodyKeys.Remove(Key);
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

            if (Body == nullptr)
            {
                InCtx._Renderer->Release_BodySlots(FellAsleepKey);
                continue;
            }

            Draw_Body(InCtx, *Body);
            TargetImpl._SleepingBodyKeys.Emplace(FellAsleepKey);
        }

        TargetImpl._PrevActiveBodyKeys = MoveTemp(InCtx._ActiveKeys);

        return ck::EStepResult::Continue;
    }

    // A sleeping body is invisible to both body passes, so its slots would outlive its destruction. The check
    // is a bounds-checked array read per sleeping body — no draw, no allocation.
    auto
        FTechnique_CaptureJoltWorld::
        ReleaseDestroyedSleepingBodies(
            FTechnique_CaptureJoltWorld& InSelf,
            FContext_Capture& InCtx)
        -> ck::EStepResult
    {
        auto& TargetImpl = *InCtx._TargetImpl;

        auto DeadSleepingKeys = TArray<uint64>{};

        for (const auto SleepingKey : TargetImpl._SleepingBodyKeys)
        {
            const auto BodyId = JPH::BodyID{static_cast<JPH::uint32>(SleepingKey)};

            if (InCtx._LockInterface->TryGetBody(BodyId) == nullptr)
            { DeadSleepingKeys.Emplace(SleepingKey); }
        }

        for (const auto DeadKey : DeadSleepingKeys)
        {
            InCtx._Renderer->Release_BodySlots(DeadKey);
            TargetImpl._SleepingBodyKeys.Remove(DeadKey);
        }

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
            const auto CharacterColor = ck::jolt::Conv(
                TargetImpl._Palette.Get_Color(ECk_Jolt_DebugDraw_ColorClass::Character));

            // A handle is a value over (entity id + registry), so the copy addresses the same entity. It exists
            // because FCk_Handle::View() has no usable const overload.
            auto TransientEntity = InCtx._TransientEntity;

            TransientEntity.View<ck::FFragment_JoltCharacter_Current>().ForEach(
            [&](FCk_Entity InEntity, const ck::FFragment_JoltCharacter_Current& InCurrent) -> void
            {
                const auto* Character = InCurrent.Get_Character().GetPtr();
                if (Character == nullptr)
                { return; }

                const auto Key = CharacterKeyBit | static_cast<uint64>(InEntity.Get_ID());
                LiveCharacterKeys.Emplace(Key);

                InCtx._Renderer->BeginBody(Key, ECk_Jolt_DebugDraw_ColorClass::Character);

                constexpr auto UseMaterialColors = false;
                constexpr auto DrawWireframe = false;
                Character->GetShape()->Draw(InCtx._Renderer, Character->GetCenterOfMassTransform(),
                    JPH::Vec3::sReplicate(1.0f), CharacterColor, UseMaterialColors, DrawWireframe);

                InCtx._Renderer->EndBody();

                ++TargetImpl._LastCaptureStats._BodiesCaptured;
            });
        }

        for (const auto GoneKey : TargetImpl._CharacterKeys)
        {
            if (NOT LiveCharacterKeys.Contains(GoneKey))
            { InCtx._Renderer->Release_BodySlots(GoneKey); }
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
        uint64 InStaticSceneRevision,
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
    Context._StaticSceneRevision = InStaticSceneRevision;

    static auto Technique = FTechnique_CaptureJoltWorld{};
    Technique.ProcessAllSteps(Context);

    EndCapture();
}

#endif

// --------------------------------------------------------------------------------------------------------------------
