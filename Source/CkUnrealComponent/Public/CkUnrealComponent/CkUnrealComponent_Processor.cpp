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

#include "Components/ActorComponent.h"
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
    auto
        PushTransformIfChanged(
            USceneComponent* InSceneComponent,
            const FTransform& InWorldTransform) -> void
    {
        if (ck::Is_NOT_Valid(InSceneComponent))
        { return; }

        if (InSceneComponent->GetComponentTransform().Equals(InWorldTransform))
        { return; }

        InSceneComponent->SetWorldTransform(InWorldTransform);
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

            if (ck::Is_NOT_Valid(HostActor, ck::IsValid_Policy_NullptrOnly{}))
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
                ck_unreal_component_processor::PushTransformIfChanged(SceneComponent, CurrentTransform);
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
