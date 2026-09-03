#include "CkNavigation/NavSurface/CkNavSurface_AreaPolicy.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_nav_surface_area_policy
{
    auto Get_Policies() -> TMap<FGameplayTag, FCk_NavSurface_AreaPolicy>&
    {
        static auto Policies = TMap<FGameplayTag, FCk_NavSurface_AreaPolicy>{};
        return Policies;
    }

    auto Get_PendingRegistrations() -> TArray<TFunction<void()>>&
    {
        static auto Pending = TArray<TFunction<void()>>{};
        return Pending;
    }

    auto DoFlushPendingRegistrations() -> void
    {
        auto& Pending = Get_PendingRegistrations();
        if (Pending.IsEmpty())
        { return; }

        auto Running = MoveTemp(Pending);
        Pending.Reset();

        for (const auto& Registration : Running)
        { Registration(); }
    }

    auto Get_SeededPolicies() -> TMap<FGameplayTag, FCk_NavSurface_AreaPolicy>&
    {
        DoFlushPendingRegistrations();
        return Get_Policies();
    }

    auto Get_IsSamePolicy(
        const FCk_NavSurface_AreaPolicy& InA,
        const FCk_NavSurface_AreaPolicy& InB) -> bool
    {
        return InA.Get_Kind() == InB.Get_Kind() &&
               FMath::IsNearlyEqual(InA.Get_CostMultiplier(), InB.Get_CostMultiplier());
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::nav_surface
{
    FRegistrar::
        FRegistrar(
            TFunction<void()> InRegistration)
    {
        ck_nav_surface_area_policy::Get_PendingRegistrations().Add(MoveTemp(InRegistration));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Register_AreaPolicy(
            const FGameplayTag&              InAreaTag,
            const FCk_NavSurface_AreaPolicy& InPolicy)
        -> void
    {
        const auto RegistrationIsValid = InAreaTag.IsValid();
        CK_ENSURE_IF_NOT(RegistrationIsValid,
            TEXT("Rejected nav area policy registration: invalid tag"))
        { return; }

        auto& Policies = ck_nav_surface_area_policy::Get_Policies();

        const auto* Existing = Policies.Find(InAreaTag);

        if (Existing == nullptr)
        {
            Policies.Add(InAreaTag, InPolicy);
            return;
        }

        const auto PolicyAgrees = ck_nav_surface_area_policy::Get_IsSamePolicy(*Existing, InPolicy);
        CK_ENSURE_IF_NOT(PolicyAgrees,
            TEXT("Nav area tag [{}] is already registered as [{}] x[{}] and was re-registered as [{}] x[{}]. The FIRST policy stands."),
            InAreaTag,
            Existing->Get_Kind(), Existing->Get_CostMultiplier(),
            InPolicy.Get_Kind(), InPolicy.Get_CostMultiplier())
        { return; }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        TryGet_AreaPolicy(
            const FGameplayTag& InAreaTag)
        -> TOptional<FCk_NavSurface_AreaPolicy>
    {
        if (NOT InAreaTag.IsValid())
        { return {}; }

        const auto* Found = ck_nav_surface_area_policy::Get_SeededPolicies().Find(InAreaTag);

        if (Found == nullptr)
        { return {}; }

        return *Found;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_RegisteredAreaTags()
        -> TArray<FGameplayTag>
    {
        auto AreaTags = TArray<FGameplayTag>{};
        ck_nav_surface_area_policy::Get_SeededPolicies().GetKeys(AreaTags);
        return AreaTags;
    }
}

// --------------------------------------------------------------------------------------------------------------------
