#include "CkUnrealComponent_Processor.h"

#include "CkUnrealComponent/CkUnrealComponent_Log.h"
#include "CkUnrealComponent/CkUnrealComponent_Utils.h"
#include "CkUnrealComponent/Host/CkComponentHost_Subsystem.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkJolt/StaticWorld/CkJoltBakeExtraction.h"
#include "CkJolt/StaticWorld/CkJoltStaticWorld_Subsystem.h"
#include "CkJolt/StaticWorld/CkJoltStaticWorld_Utils.h"

#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_UnrealComponent_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_UnrealComponent_PushTransform);
CK_REGISTER_PROCESSOR(ck::FProcessor_UnrealComponent_Tick);
CK_REGISTER_PROCESSOR(ck::FProcessor_UnrealComponent_EndPlay);

namespace ck_unreal_component_processor
{
    // Returns whether the transform actually changed — baked static-world bodies must follow.
    auto
        PushTransformIfChanged(
            USceneComponent* InSceneComponent,
            const FTransform& InWorldTransform) -> bool
    {
        if (ck::Is_NOT_Valid(InSceneComponent))
        { return false; }

        if (InSceneComponent->GetComponentTransform().Equals(InWorldTransform))
        { return false; }

        InSceneComponent->SetWorldTransform(InWorldTransform);
        return true;
    }
}

namespace ck
{
    auto
        FProcessor_UnrealComponent_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_UnrealComponent_Params& InParams,
            FFragment_UnrealComponent_Current& InCurrent)
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        const auto ComponentClass = InParams.Get_ComponentClass();
        CK_ENSURE_IF_NOT(ck::IsValid(ComponentClass),
            TEXT("UnrealComponent [{}] has invalid ComponentClass"), InHandle)
        { return; }

        const auto ComponentArchetype = InParams.Get_ComponentArchetype().Get();

        if (ck::IsValid(ComponentArchetype))
        {
            CK_ENSURE_IF_NOT(ComponentArchetype->IsA(ComponentClass),
            TEXT("UnrealComponent [{}] has a non-null Archetype [{}] that is NOT of class [{}]"), InHandle, ComponentArchetype, ComponentClass)
            { return; }
        }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(World),
            TEXT("UnrealComponent [{}] could not resolve World"), InHandle)
        { return; }

        auto Host = UCk_ComponentHost_Subsystem_UE::Get(World);
        CK_ENSURE_IF_NOT(ck::IsValid(Host),
            TEXT("UnrealComponent [{}] could not resolve ComponentHost subsystem"), InHandle)
        { return; }

        const auto IsSceneComponent = ComponentClass->IsChildOf(USceneComponent::StaticClass());

        if (IsSceneComponent)
        {
            CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InCurrent._OwningEntity),
                TEXT("UnrealComponent [{}] is a SceneComponent but its OwningEntity [{}] has no Transform fragment"),
                InHandle, InCurrent._OwningEntity)
            { return; }
        }

        // Non-scene components have no nav relevance and stay World-hosted; scene components need an
        // owning Actor — see UCk_ComponentHost_Subsystem_UE::Get_HostActor.
        UObject* ComponentOuter = World;
        if (IsSceneComponent)
        {
            auto HostActor = static_cast<AActor*>(nullptr);

#if WITH_EDITOR
            HostActor = UCk_Utils_EditorSelectionOwner_UE::TryGet_SelectionProxyHostActor(World, InHandle);
#endif

            if (ck::Is_NOT_Valid(HostActor))
            { HostActor = Host->Get_HostActor(); }

            if (ck::IsValid(HostActor))
            { ComponentOuter = HostActor; }
        }

        // DestroyOnRelease — the subsystem pins it so the fragment can hold a weak ptr
        const auto PoolParams = FCk_ObjectPooling_PoolParams{}
            .Set_RecyclePolicy(ECk_ObjectPooling_RecyclePolicy::DestroyOnRelease);

        auto NewComponent = UCk_Utils_Object_UE::Request_CreateNewObject<UActorComponent>(ComponentOuter,
            ComponentClass, ComponentArchetype, PoolParams, nullptr);

        CK_ENSURE_IF_NOT(ck::IsValid(NewComponent),
            TEXT("UnrealComponent [{}] failed to instantiate component of class [{}]"),
            InHandle, ComponentClass->GetName())
        { return; }

        NewComponent->RegisterComponentWithWorld(World);

        InCurrent._Component = NewComponent;

        if (IsSceneComponent)
        {
            InHandle.AddOrGet<FTag_UnrealComponent_IsScene>();

            if (NOT InHandle.Has<FTag_UnrealComponent_TransformPushDisabled>())
            {
                auto OwnerTransform = UCk_Utils_Transform_UE::CastChecked(InCurrent.Get_OwningEntity());
                ck_unreal_component_processor::PushTransformIfChanged(
                    CastChecked<USceneComponent>(NewComponent),
                    UCk_Utils_Transform_UE::Get_EntityCurrentTransform(OwnerTransform));
            }
        }

        if (InParams.Get_TickPolicy() == ECk_UnrealComponent_TickPolicy::TickViaProcessor)
        {
            InHandle.AddOrGet<FTag_UnrealComponent_TickViaProcessor>();
        }

        // Editor/preview ECS worlds have no Jolt static world (game worlds only) — every bake policy
        // is a quiet skip there, not an ensure. The utils-level ensure stays for EXPLICIT callers.
        auto* StaticWorldSubsystem = World->GetSubsystem<UCk_JoltStaticWorld_Subsystem_UE>();

        if (ck::IsValid(StaticWorldSubsystem))
        {
            switch (InParams.Get_StaticWorldBakePolicy())
            {
                case ECk_UnrealComponent_StaticWorldBakePolicy::Automatic:
                {
                    // Default-on with the designer opt-outs: a collision-bearing primitive bakes unless
                    // the Jolt bake-filter's component exclusions say otherwise. Zero bodies is a QUIET
                    // skip — NoCollision content and an ISM whose instances arrive after Add are legal
                    // here (the latter opts in via Request_BakeIntoJoltStaticWorld once configured).
                    auto* PrimitiveComponent = Cast<UPrimitiveComponent>(NewComponent);
                    if (ck::Is_NOT_Valid(PrimitiveComponent))
                    { break; }

                    if (PrimitiveComponent->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
                    { break; }

                    const auto BakeFilter = ck::jolt::bake::FCk_Jolt_BakeFilter::Make_FromProjectSettings();
                    if (ck::jolt::bake::Get_IsComponentExcludedByBakeFilter(*PrimitiveComponent, BakeFilter))
                    { break; }

                    if (StaticWorldSubsystem->Request_BakeComponent(*PrimitiveComponent) > 0)
                    { InHandle.AddOrGet<FTag_UnrealComponent_BakedIntoStaticWorld>(); }
                    break;
                }
                case ECk_UnrealComponent_StaticWorldBakePolicy::BakeOnSetup:
                {
                    // BakeOnSetup declares the archetype carried complete collision — a primitive
                    // component with zero extracted bodies means the policy was set on unbakeable content.
                    auto* PrimitiveComponent = Cast<UPrimitiveComponent>(NewComponent);

                    CK_ENSURE_IF_NOT(ck::IsValid(PrimitiveComponent),
                        TEXT("UnrealComponent [{}] has StaticWorldBakePolicy BakeOnSetup but hosts a NON-PRIMITIVE "
                             "class [{}] — nothing can bake."), InHandle, ComponentClass->GetName())
                    {}

                    if (ck::IsValid(PrimitiveComponent))
                    {
                        const auto NumBodies = StaticWorldSubsystem->Request_BakeComponent(*PrimitiveComponent);

                        CK_ENSURE_IF_NOT(NumBodies > 0,
                            TEXT("UnrealComponent [{}] has StaticWorldBakePolicy BakeOnSetup but its archetype produced "
                                 "ZERO static bodies — the archetype's collision is disabled or invalid. Author the "
                                 "collision on the archetype, or use Automatic/DoNotBake + "
                                 "Request_BakeIntoJoltStaticWorld for components configured after Add."), InHandle)
                        {}

                        if (NumBodies > 0)
                        { InHandle.AddOrGet<FTag_UnrealComponent_BakedIntoStaticWorld>(); }
                    }
                    break;
                }
                case ECk_UnrealComponent_StaticWorldBakePolicy::DoNotBake:
                { break; }
            }
        }

        UCk_Utils_UnrealComponent_UE::DoRegisterBridge(NewComponent, InHandle);

        ck::unreal_component::Verbose(TEXT("UnrealComponent [{}] registered component of class [{}]"),
            InHandle, ComponentClass->GetName());

        UUtils_Signal_UnrealComponent_OnAdded::Broadcast(InHandle, MakePayload(InHandle));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_UnrealComponent_PushTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_RecordOfUnrealComponents&)
        -> void
    {
        // PostTransform runs after root-to-ECS synchronization and transform requests. At this point the
        // fragment is the authoritative value for both root-driven and externally-driven owners.
        const auto& CurrentTransform = InTransform.Get_Transform();
        RecordOfUnrealComponents_Utils::ForEach_ValidEntry(
            InHandle,
            [&CurrentTransform](FCk_Handle_UnrealComponent InComponentHandle)
            {
                if (InComponentHandle.Has<FTag_UnrealComponent_NeedsSetup>() ||
                    InComponentHandle.Has<FTag_UnrealComponent_TransformPushDisabled>() ||
                    NOT InComponentHandle.Has<FTag_UnrealComponent_IsScene>() ||
                    NOT InComponentHandle.Has<FFragment_UnrealComponent_Current>())
                { return; }

                auto* SceneComponent = Cast<USceneComponent>(
                    InComponentHandle.Get<FFragment_UnrealComponent_Current>().Get_Component().Get());
                const auto TransformChanged =
                    ck_unreal_component_processor::PushTransformIfChanged(SceneComponent, CurrentTransform);

                // A baked static-world body is a snapshot — when the component actually moves,
                // re-bake at the new pose so queries stay correct (teleports, store rearrangement).
                // A CONTINUOUSLY moving blocker churns the broadphase every frame; that content
                // belongs on a kinematic CkJoltBody, not the static world.
                if (TransformChanged &&
                    InComponentHandle.Has<FTag_UnrealComponent_BakedIntoStaticWorld>())
                {
                    if (auto* PrimitiveComponent = Cast<UPrimitiveComponent>(SceneComponent);
                        ck::IsValid(PrimitiveComponent))
                    { UCk_Utils_JoltStaticWorld_UE::Request_BakeComponent(PrimitiveComponent); }
                }
            });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_UnrealComponent_Tick::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_UnrealComponent_Current& InCurrent)
        -> void
    {
        auto Component = InCurrent.Get_Component().Get();
        if (ck::Is_NOT_Valid(Component))
        { return; }

        Component->TickComponent(
            InDeltaT.Get_Seconds(),
            LEVELTICK_All,
            &Component->PrimaryComponentTick);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_UnrealComponent_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_UnrealComponent_Current& InCurrent)
        -> void
    {
        ck::unreal_component::Verbose(TEXT("Tearing down UnrealComponent [{}]"), InHandle);

        UUtils_Signal_UnrealComponent_OnRemoved::Broadcast(InHandle, MakePayload(InHandle));

        auto Component = InCurrent._Component.Get();
        if (ck::IsValid(Component))
        {
            // Baked static-world bodies must go BEFORE the component: the subsystem's removal map is
            // keyed by the component pointer.
            if (InHandle.Has<FTag_UnrealComponent_BakedIntoStaticWorld>())
            {
                if (auto* PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
                    ck::IsValid(PrimitiveComponent))
                { UCk_Utils_JoltStaticWorld_UE::Request_RemoveComponent(PrimitiveComponent); }
            }

            UCk_Utils_UnrealComponent_UE::DoUnregisterBridge(Component);
            Component->UnregisterComponent();

            // unpin before DestroyComponent (destroy garbage-marks the object, failing release's validity check)
            UCk_Utils_Object_UE::TryReleaseToPool(Component);
            Component->DestroyComponent();
        }

        InCurrent._Component.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------
