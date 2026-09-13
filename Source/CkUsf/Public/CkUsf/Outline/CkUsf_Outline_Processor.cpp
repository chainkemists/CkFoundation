#include "CkUsf_Outline_Processor.h"

#include "CkUsf/Outline/CkUsf_Outline_ProjectSettings.h"
#include "CkUsf/Outline/CkUsf_OutlinePreset.h"
#include "CkUsf/Outline/CkUsf_OutlineSubsystem.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "GameFramework/Actor.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_OutlineResolved_Clear);
CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_OutlineClaims_Resolve);
CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_OutlineActor_Sync);
CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_OutlineActor_Remove);
CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_OutlineActor_EndPlay);

namespace ck_usf_outline_processor
{
    auto CandidateWins(const ck::FFragment_Usf_OutlineResolved& InCurrent,
                       const FCk_Usf_OutlineRuntimeDefinition& InDefinition,
                       const ck::FUsf_OutlineClaim& InClaim,
                       int32 InOwnershipDistance) -> bool
    {
        if (InDefinition.LayerIndex != InCurrent.Get_LayerIndex())
        { return InDefinition.LayerIndex < InCurrent.Get_LayerIndex(); }
        if (InOwnershipDistance != InCurrent.Get_OwnershipDistance())
        { return InOwnershipDistance < InCurrent.Get_OwnershipDistance(); }
        if (InClaim.Source != InCurrent.Get_Source())
        { return InClaim.Source < InCurrent.Get_Source(); }
        return InClaim.OutlineTag.ToString() < InCurrent.Get_OutlineTag().ToString();
    }

    auto ApplyCandidate(FCk_Handle& InTarget, const ck::FUsf_OutlineClaim& InClaim,
                        const FCk_Usf_OutlineRuntimeDefinition& InDefinition,
                        int32 InOwnershipDistance) -> void
    {
        if (InTarget.Has<ck::FFragment_Usf_OutlineResolved>())
        {
            const auto& Current = InTarget.Get<ck::FFragment_Usf_OutlineResolved>();
            if (NOT CandidateWins(Current, InDefinition, InClaim, InOwnershipDistance))
            { return; }
        }

        InTarget.AddOrGet<ck::FFragment_Usf_OutlineResolved>() = ck::FFragment_Usf_OutlineResolved{
            InClaim.Source, InClaim.OutlineTag, InDefinition.LayerTag, InDefinition.Preset,
            InDefinition.LayerIndex, InOwnershipDistance};
    }

    auto ApplyClaimToLiveSubtree(FCk_Handle& InTarget, const ck::FUsf_OutlineClaim& InClaim,
                                 const FCk_Usf_OutlineRuntimeDefinition& InDefinition,
                                 int32 InOwnershipDistance, TSet<FCk_Handle>& InVisited) -> void
    {
        const auto WasAlreadyVisited = InVisited.Contains(InTarget);
        CK_ENSURE_IF_NOT(NOT WasAlreadyVisited,
            TEXT("Outline live-subtree traversal found a lifetime ownership cycle at [{}]"), InTarget) {}
        if (WasAlreadyVisited) { return; }
        InVisited.Add(InTarget);

        ApplyCandidate(InTarget, InClaim, InDefinition, InOwnershipDistance);
        for (auto Dependent : UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(InTarget))
        {
            if (ck::Is_NOT_Valid(Dependent)) { continue; }
            ApplyClaimToLiveSubtree(
                Dependent, InClaim, InDefinition, InOwnershipDistance + 1, InVisited);
        }
    }

    auto ClearActorOwners(const FCk_Handle& InHandle,
                          const ck::FFragment_Usf_OutlineApplied_Actor& InApplied) -> void
    {
        for (const auto& WeakComponent : InApplied.Get_Components())
        {
            auto* Component = WeakComponent.Get();
            if (ck::Is_NOT_Valid(Component)) { continue; }
            if (auto* Subsystem = UCkUsf_OutlineSubsystem::Get_OutlineSubsystem(Component))
            { Subsystem->Clear_ResolvedOutline(Component, InHandle); }
        }
    }
}

namespace ck
{
    auto FProcessor_Usf_OutlineResolved_Clear::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle, const FFragment_Usf_OutlineResolved& InResolved) -> void
    { InHandle.Remove<FFragment_Usf_OutlineResolved>(); }

    auto FProcessor_Usf_OutlineClaims_Resolve::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle, FFragment_Usf_OutlineClaims& InClaims) -> void
    {
        InClaims._Claims.RemoveAll(
            [&InHandle](const auto& InClaim)
            {
                const auto SourceIsValid = ck::IsValid(InClaim.Source);
                CK_ENSURE_IF_NOT(SourceIsValid,
                    TEXT("Reaping outline claim with invalid source on target [{}]"), InHandle) {}
                return NOT SourceIsValid;
            });

        if (InClaims._Claims.IsEmpty())
        {
            InHandle.Remove<FFragment_Usf_OutlineClaims>();
            return;
        }

        auto RuntimeConfig = FCk_Usf_OutlineRuntimeConfig{};
        if (NOT UCk_Utils_Usf_Outline_Settings_UE::TryGet_RuntimeConfig(RuntimeConfig)) { return; }

        InClaims._Claims.RemoveAll(
            [&InHandle, &RuntimeConfig](const auto& InClaim)
            {
                const auto DefinitionIsConfigured = RuntimeConfig.TryGet(InClaim.OutlineTag) != nullptr;
                CK_ENSURE_IF_NOT(DefinitionIsConfigured,
                    TEXT("Reaping active outline claim [{}] on target [{}]: tag is no longer configured"),
                    InClaim.OutlineTag, InHandle) {}

                const auto ScopeIsValid = InClaim.Scope == ECk_Usf_OutlineScope::EntityOnly ||
                                          InClaim.Scope == ECk_Usf_OutlineScope::EntityAndDependents;
                CK_ENSURE_IF_NOT(ScopeIsValid,
                    TEXT("Reaping active outline claim [{}] on target [{}]: scope [{}] is invalid"),
                    InClaim.OutlineTag, InHandle, InClaim.Scope) {}
                return NOT DefinitionIsConfigured || NOT ScopeIsValid;
            });

        if (InClaims._Claims.IsEmpty())
        {
            InHandle.Remove<FFragment_Usf_OutlineClaims>();
            return;
        }

        for (const auto& Claim : InClaims._Claims)
        {
            const auto* Definition = RuntimeConfig.TryGet(Claim.OutlineTag);
            check(Definition != nullptr);

            if (Claim.Scope == ECk_Usf_OutlineScope::EntityOnly)
            { ck_usf_outline_processor::ApplyCandidate(InHandle, Claim, *Definition, 0); }
            else
            {
                auto Visited = TSet<FCk_Handle>{};
                ck_usf_outline_processor::ApplyClaimToLiveSubtree(
                    InHandle, Claim, *Definition, 0, Visited);
            }
        }
    }

    auto FProcessor_Usf_OutlineActor_Sync::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle, const FFragment_Usf_OutlineResolved& InResolved,
        const FFragment_OwningActor_Current& InOwningActor) -> void
    {
        auto* Actor = InOwningActor.Get_EntityOwningActor().Get();
        auto* Preset = InResolved.Get_Preset().Get();
        if (ck::Is_NOT_Valid(Actor) || ck::Is_NOT_Valid(Preset)) { return; }

        auto* Subsystem = UCkUsf_OutlineSubsystem::Get_OutlineSubsystem(Actor);
        if (ck::Is_NOT_Valid(Subsystem)) { return; }

        auto CurrentComponents = TArray<UPrimitiveComponent*>{};
        Actor->GetComponents(CurrentComponents);
        auto WeakCurrentComponents = TArray<TWeakObjectPtr<UPrimitiveComponent>>{};
        WeakCurrentComponents.Reserve(CurrentComponents.Num());

        if (InHandle.Has<FFragment_Usf_OutlineApplied_Actor>())
        {
            const auto& Applied = InHandle.Get<FFragment_Usf_OutlineApplied_Actor>();
            if (Applied.Get_Actor() != Actor)
            { ck_usf_outline_processor::ClearActorOwners(InHandle, Applied); }
            else
            {
                for (const auto& Previous : Applied.Get_Components())
                {
                    auto* PreviousComponent = Previous.Get();
                    if (ck::IsValid(PreviousComponent) && NOT CurrentComponents.Contains(PreviousComponent))
                    { Subsystem->Clear_ResolvedOutline(PreviousComponent, InHandle); }
                }
            }
        }

        for (auto* Component : CurrentComponents)
        {
            if (ck::Is_NOT_Valid(Component)) { continue; }
            Subsystem->Set_ResolvedOutline(Component, InHandle, InResolved);
            WeakCurrentComponents.Add(Component);
        }
        InHandle.AddOrGet<FFragment_Usf_OutlineApplied_Actor>() =
            FFragment_Usf_OutlineApplied_Actor{Actor, MoveTemp(WeakCurrentComponents)};
    }

    auto FProcessor_Usf_OutlineActor_Remove::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle, const FFragment_Usf_OutlineApplied_Actor& InApplied) -> void
    {
        ck_usf_outline_processor::ClearActorOwners(InHandle, InApplied);
        InHandle.Remove<FFragment_Usf_OutlineApplied_Actor>();
    }

    auto FProcessor_Usf_OutlineActor_EndPlay::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle, const FFragment_Usf_OutlineApplied_Actor& InApplied) -> void
    {
        ck_usf_outline_processor::ClearActorOwners(InHandle, InApplied);
        InHandle.Remove<FFragment_Usf_OutlineApplied_Actor>();
    }
}
