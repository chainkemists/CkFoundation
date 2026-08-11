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

#include "CkProfile/Stats/CkCpuWork.h"

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
                const auto OwnerWorldTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(OwnerTransform);
                ck_unreal_component_processor::PushTransformIfChanged(
                    CastChecked<USceneComponent>(NewComponent), OwnerWorldTransform);

                // Seeded with the exact value just pushed so PushTransform's change detection starts
                // settled. The fragment lives on the OWNER, shared by all its components — seed only
                // when absent: resetting existing memory here would swallow an owner move that a
                // sibling component (added earlier, positioned at the old pose) still needs delivered.
                if (NOT OwnerTransform.Has<FFragment_UnrealComponent_LastPushedTransform>())
                {
                    OwnerTransform.Add<FFragment_UnrealComponent_LastPushedTransform>(OwnerWorldTransform);
                }
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
            const FFragment_RecordOfUnrealComponents&,
            FFragment_UnrealComponent_LastPushedTransform& InLastPushed)
        -> void
    {
        // PostTransform runs after root-to-ECS synchronization and transform requests. At this point the
        // fragment is the authoritative value for both root-driven and externally-driven owners.
        const auto& CurrentTransform = InTransform.Get_Transform();

        // The view is deliberately NOT tag-gated (a pump-drained tag is already dead by the next main
        // pass), so EVERY component-owning transform entity is visited each tick and this comparison is
        // what keeps the push owner-driven: an owner whose fragment still matches what was last
        // delivered has nothing to deliver, and re-pushing it would stomp externally-driven component
        // drift (TransformPropagation.DirtyOwnersOnly). Rationale on the fragment's declaration.
        const auto OwnerSettled = InLastPushed.Get_Transform().Equals(CurrentTransform);

        const auto Enabled = cpu_work::Get_Enabled();

        // A settled owner bails BEFORE the record walk, so the steady-state cost is one Equals. In an
        // enabled frame the walk still runs with the push suppressed, so those entries land in their real
        // buckets (Unchanged, or their eligibility rejection) rather than vanishing from the partition the
        // component counters are documented to form over valid Record callbacks. The extra traversal is
        // diagnostic-only by construction — CkProfile's contract is that this instrumentation is never
        // itself a gameplay optimization, and its overhead is characterized by an on/off capture.
        if (OwnerSettled && NOT Enabled)
        { return; }

        TRACE_CPUPROFILER_EVENT_SCOPE_CONDITIONAL(CkCpuWork_ComponentRecord, Enabled);

        auto Entries = int32{0};
        auto SetupRejected = int32{0};
        auto PushDisabled = int32{0};
        auto NonSceneRejected = int32{0};
        auto MissingTransform = int32{0};
        auto Invalid = int32{0};
        auto Unchanged = int32{0};
        auto Changed = int32{0};
        auto StaticRebakes = int32{0};

        if (NOT OwnerSettled)
        { InLastPushed._Transform = CurrentTransform; }

        RecordOfUnrealComponents_Utils::ForEach_ValidEntry(
            InHandle,
            [&](FCk_Handle_UnrealComponent InComponentHandle)
            {
                TRACE_CPUPROFILER_EVENT_SCOPE_CONDITIONAL(CkCpuWork_ComponentCallback, Enabled);
                if (Enabled)
                { ++Entries; }

                if (InComponentHandle.Has<FTag_UnrealComponent_NeedsSetup>())
                {
                    if (Enabled) { ++SetupRejected; }
                    return;
                }
                if (InComponentHandle.Has<FTag_UnrealComponent_TransformPushDisabled>())
                {
                    if (Enabled) { ++PushDisabled; }
                    return;
                }
                if (NOT InComponentHandle.Has<FTag_UnrealComponent_IsScene>())
                {
                    if (Enabled) { ++NonSceneRejected; }
                    return;
                }
                if (NOT InComponentHandle.Has<FFragment_UnrealComponent_Current>())
                {
                    if (Enabled) { ++MissingTransform; }
                    return;
                }

                auto* SceneComponent = Cast<USceneComponent>(
                    InComponentHandle.Get<FFragment_UnrealComponent_Current>().Get_Component().Get());
                if (Enabled && ck::Is_NOT_Valid(SceneComponent))
                {
                    ++Invalid;
                    return;
                }

                // A settled owner reaches here only to be counted; suppressing the push is the whole point
                // of the early-out, and "nothing was delivered to this component" is Unchanged.
                const auto TransformChanged = NOT OwnerSettled &&
                    ck_unreal_component_processor::PushTransformIfChanged(SceneComponent, CurrentTransform);
                if (Enabled)
                {
                    TransformChanged ? ++Changed : ++Unchanged;
                }

                // A baked static-world body is a snapshot — when the component actually moves,
                // re-bake at the new pose so queries stay correct (teleports, store rearrangement).
                // A CONTINUOUSLY moving blocker churns the broadphase every frame; that content
                // belongs on a kinematic CkJoltBody, not the static world.
                if (TransformChanged &&
                    InComponentHandle.Has<FTag_UnrealComponent_BakedIntoStaticWorld>())
                {
                    if (auto* PrimitiveComponent = Cast<UPrimitiveComponent>(SceneComponent);
                        ck::IsValid(PrimitiveComponent))
                    {
                        UCk_Utils_JoltStaticWorld_UE::Request_BakeComponent(PrimitiveComponent);
                        if (Enabled) { ++StaticRebakes; }
                    }
                }
            });

        if (Enabled)
        {
            cpu_work::Add(ECk_CpuWorkCounter::ComponentEntries, Entries);
            cpu_work::Add(ECk_CpuWorkCounter::ComponentSetupRejected, SetupRejected);
            cpu_work::Add(ECk_CpuWorkCounter::ComponentPushDisabled, PushDisabled);
            cpu_work::Add(ECk_CpuWorkCounter::ComponentNonSceneRejected, NonSceneRejected);
            cpu_work::Add(ECk_CpuWorkCounter::ComponentMissingTransform, MissingTransform);
            cpu_work::Add(ECk_CpuWorkCounter::ComponentInvalid, Invalid);
            cpu_work::Add(ECk_CpuWorkCounter::ComponentUnchanged, Unchanged);
            cpu_work::Add(ECk_CpuWorkCounter::ComponentChanged, Changed);
            cpu_work::Add(ECk_CpuWorkCounter::ComponentStaticRebakes, StaticRebakes);
        }
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
