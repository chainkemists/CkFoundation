#include "CkUsf/Outline/CkUsf_Outline_Utils.h"

#include "CkUsf/Outline/CkUsf_Outline_Fragment.h"
#include "CkUsf/Outline/CkUsf_Outline_ProjectSettings.h"
#include "CkCore/Validation/CkIsValid.h"

namespace ck_usf_outline_utils
{
    auto Complete(FCk_Handle& InTarget, const FCk_Delegate_Request_OnCompleted& InDelegate,
                  ECk_Request_OperationResult InResult) -> FCk_Handle
    {
        InDelegate.ExecuteIfBound(InTarget, InResult);
        return InTarget;
    }

    auto TryValidateRequest(const FCk_Handle& InTarget, const FCk_Handle& InSource,
                            const FGameplayTag& InOutlineTag,
                            TOptional<ECk_Usf_OutlineScope> InScope) -> bool
    {
        const auto TargetIsValid = ck::IsValid(InTarget);
        CK_ENSURE_IF_NOT(TargetIsValid, TEXT("Outline claim target is INVALID")) {}
        if (NOT TargetIsValid) { return false; }

        const auto SourceIsValid = ck::IsValid(InSource);
        CK_ENSURE_IF_NOT(SourceIsValid, TEXT("Outline claim source is INVALID for target [{}]"), InTarget) {}
        if (NOT SourceIsValid) { return false; }

        auto RuntimeConfig = FCk_Usf_OutlineRuntimeConfig{};
        const auto ConfigIsValid = UCk_Utils_Usf_Outline_Settings_UE::TryGet_RuntimeConfig(RuntimeConfig);
        if (NOT ConfigIsValid) { return false; }

        const auto OutlineTagIsConfigured = RuntimeConfig.TryGet(InOutlineTag) != nullptr;
        CK_ENSURE_IF_NOT(OutlineTagIsConfigured,
            TEXT("Outline claim tag [{}] is invalid or unconfigured"), InOutlineTag) {}
        if (NOT OutlineTagIsConfigured) { return false; }

        if (InScope.IsSet())
        {
            const auto Scope = InScope.GetValue();
            const auto ScopeIsValid = Scope == ECk_Usf_OutlineScope::EntityOnly ||
                                      Scope == ECk_Usf_OutlineScope::EntityAndDependents;
            CK_ENSURE_IF_NOT(ScopeIsValid,
                TEXT("Outline claim scope [{}] is invalid for target [{}]"), Scope, InTarget) {}
            if (NOT ScopeIsValid) { return false; }
        }
        return true;
    }
}

auto UCk_Utils_Usf_Outline_UE::Set_OutlineClaim(
    FCk_Handle& InTarget, const FCk_Handle& InSource, FGameplayTag InOutlineTag,
    ECk_Usf_OutlineScope InScope, const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle
{
    if (NOT ck_usf_outline_utils::TryValidateRequest(InTarget, InSource, InOutlineTag, InScope))
    {
        return ck_usf_outline_utils::Complete(
            InTarget, InDelegate, ECk_Request_OperationResult::Failed_NotEnqueued);
    }

    auto& Claims = InTarget.AddOrGet<ck::FFragment_Usf_OutlineClaims>()._Claims;
    auto* Existing = Claims.FindByPredicate(
        [&InSource, &InOutlineTag](const auto& InClaim)
        { return InClaim.Source == InSource && InClaim.OutlineTag.MatchesTagExact(InOutlineTag); });

    if (Existing != nullptr) { Existing->Scope = InScope; }
    else { Claims.Add(ck::FUsf_OutlineClaim{InSource, InOutlineTag, InScope}); }

    return ck_usf_outline_utils::Complete(InTarget, InDelegate, ECk_Request_OperationResult::Succeeded);
}

auto UCk_Utils_Usf_Outline_UE::Clear_OutlineClaim(
    FCk_Handle& InTarget, const FCk_Handle& InSource, FGameplayTag InOutlineTag,
    const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle
{
    if (NOT ck_usf_outline_utils::TryValidateRequest(InTarget, InSource, InOutlineTag, {}))
    {
        return ck_usf_outline_utils::Complete(
            InTarget, InDelegate, ECk_Request_OperationResult::Failed_NotEnqueued);
    }

    const auto HasClaims = InTarget.Has<ck::FFragment_Usf_OutlineClaims>();
    auto* Claims = HasClaims ? &InTarget.Get<ck::FFragment_Usf_OutlineClaims>()._Claims : nullptr;
    const auto ClaimIndex = Claims == nullptr ? INDEX_NONE : Claims->IndexOfByPredicate(
        [&InSource, &InOutlineTag](const auto& InClaim)
        { return InClaim.Source == InSource && InClaim.OutlineTag.MatchesTagExact(InOutlineTag); });
    const auto ClaimExists = ClaimIndex != INDEX_NONE;
    CK_ENSURE_IF_NOT(ClaimExists,
        TEXT("Cannot clear unowned outline claim [{}] from source [{}] on target [{}]"),
        InOutlineTag, InSource, InTarget) {}
    if (NOT ClaimExists)
    {
        return ck_usf_outline_utils::Complete(
            InTarget, InDelegate, ECk_Request_OperationResult::Failed_NotEnqueued);
    }

    Claims->RemoveAt(ClaimIndex);
    if (Claims->IsEmpty()) { InTarget.Remove<ck::FFragment_Usf_OutlineClaims>(); }
    return ck_usf_outline_utils::Complete(InTarget, InDelegate, ECk_Request_OperationResult::Succeeded);
}

auto UCk_Utils_Usf_Outline_UE::Has_Outline(const FCk_Handle& InHandle) -> bool
{
    return ck::IsValid(InHandle) &&
           (InHandle.Has<ck::FFragment_Usf_OutlineClaims>() ||
            InHandle.Has<ck::FFragment_Usf_OutlineResolved>());
}

auto UCk_Utils_Usf_Outline_UE::Has_OutlineClaim(
    const FCk_Handle& InTarget, const FCk_Handle& InSource, FGameplayTag InOutlineTag) -> bool
{
    if (ck::Is_NOT_Valid(InTarget) || NOT InTarget.Has<ck::FFragment_Usf_OutlineClaims>())
    { return false; }
    return InTarget.Get<ck::FFragment_Usf_OutlineClaims>()._Claims.ContainsByPredicate(
        [&InSource, &InOutlineTag](const auto& InClaim)
        { return InClaim.Source == InSource && InClaim.OutlineTag.MatchesTagExact(InOutlineTag); });
}

auto UCk_Utils_Usf_Outline_UE::TryGet_OutlinePreset(const FCk_Handle& InHandle) -> UCkUsf_OutlinePreset*
{
    if (ck::Is_NOT_Valid(InHandle) || NOT InHandle.Has<ck::FFragment_Usf_OutlineResolved>())
    { return nullptr; }
    return InHandle.Get<ck::FFragment_Usf_OutlineResolved>().Get_Preset().Get();
}
