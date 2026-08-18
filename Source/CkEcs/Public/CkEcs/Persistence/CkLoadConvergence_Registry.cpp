#include "CkEcs/Persistence/CkLoadConvergence_Registry.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_load_convergence_registry
{
    struct FEntry
    {
        FName _Name;
        ck::FCk_LoadConvergenceRegistry::FPredicate _Predicate;
    };

    // Registration ORDER is preserved rather than hashed, so the pending list a lossy load reports reads the
    // same way every run — a report whose contents reshuffle between runs is one nobody can diff.
    auto
        DoGet_Entries()
        -> TArray<FEntry>&
    {
        static auto Entries = TArray<FEntry>{};
        return Entries;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FCk_LoadConvergenceRegistry::
    Register(
        FName InName,
        FPredicate InPredicate)
    -> void
{
    const auto NameIsUsable = NOT InName.IsNone();
    CK_ENSURE_IF_NOT(NameIsUsable,
        TEXT("A load-convergence predicate was registered with no name. The name is how an unmet fact is REPORTED "
             "when the convergence phase gives up — an unnamed one would burn the budget anonymously"))
    { return; }

    const auto PredicateIsBound = static_cast<bool>(InPredicate);
    CK_ENSURE_IF_NOT(PredicateIsBound,
        TEXT("Load-convergence predicate [{}] was registered unbound — it can never report Satisfied, so every "
             "load would burn the convergence budget and report it as a loss"), InName)
    { return; }

    auto& Entries = ck_load_convergence_registry::DoGet_Entries();

    if (auto* Existing = Entries.FindByPredicate([&](const ck_load_convergence_registry::FEntry& InEntry)
        { return InEntry._Name == InName; }))
    {
        Existing->_Predicate = MoveTemp(InPredicate);
        return;
    }

    Entries.Emplace(ck_load_convergence_registry::FEntry{InName, MoveTemp(InPredicate)});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FCk_LoadConvergenceRegistry::
    Unregister(
        FName InName)
    -> void
{
    ck_load_convergence_registry::DoGet_Entries().RemoveAll([&](const ck_load_convergence_registry::FEntry& InEntry)
    { return InEntry._Name == InName; });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FCk_LoadConvergenceRegistry::
    Get_Pending(
        const FCk_Registry& InRegistry)
    -> TArray<FName>
{
    auto Pending = TArray<FName>{};

    const auto& Entries = ck_load_convergence_registry::DoGet_Entries();

    const auto RegistryIsValid = ck::IsValid(InRegistry);
    CK_ENSURE_IF_NOT(RegistryIsValid,
        TEXT("Load convergence was asked about an invalid registry. Reporting every registered fact as PENDING: a "
             "registry that cannot be read cannot establish that anything converged, and answering otherwise would "
             "hand the world back on a guarantee nobody checked"))
    {
        for (const auto& Entry : Entries)
        { Pending.Emplace(Entry._Name); }

        return Pending;
    }

    for (const auto& Entry : Entries)
    {
        if (Entry._Predicate(InRegistry) == ECk_LoadConvergence::Pending)
        { Pending.Emplace(Entry._Name); }
    }

    return Pending;
}

// --------------------------------------------------------------------------------------------------------------------
