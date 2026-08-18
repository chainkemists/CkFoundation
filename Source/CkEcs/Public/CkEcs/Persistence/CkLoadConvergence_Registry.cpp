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

    struct FAdvanceEntry
    {
        FName _Name;
        ck::FCk_LoadConvergenceRegistry::FAdvance _Advance;
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

    auto
        DoGet_AdvanceEntries()
        -> TArray<FAdvanceEntry>&
    {
        static auto Entries = TArray<FAdvanceEntry>{};
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

auto
    ck::FCk_LoadConvergenceRegistry::
    Register_Advance(
        FName InName,
        FAdvance InAdvance)
    -> void
{
    const auto NameIsUsable = NOT InName.IsNone();
    CK_ENSURE_IF_NOT(NameIsUsable,
        TEXT("A load-convergence advance was registered with no name. The name is its identity — an unnamed one "
             "cannot be replaced on re-registration or removed at module shutdown"))
    { return; }

    const auto AdvanceIsBound = static_cast<bool>(InAdvance);
    CK_ENSURE_IF_NOT(AdvanceIsBound,
        TEXT("Load-convergence advance [{}] was registered unbound — the work it exists to drive would never run, "
             "so whichever predicate waits on that work would burn the convergence budget"), InName)
    { return; }

    auto& Entries = ck_load_convergence_registry::DoGet_AdvanceEntries();

    if (auto* Existing = Entries.FindByPredicate([&](const ck_load_convergence_registry::FAdvanceEntry& InEntry)
        { return InEntry._Name == InName; }))
    {
        Existing->_Advance = MoveTemp(InAdvance);
        return;
    }

    Entries.Emplace(ck_load_convergence_registry::FAdvanceEntry{InName, MoveTemp(InAdvance)});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FCk_LoadConvergenceRegistry::
    Unregister_Advance(
        FName InName)
    -> void
{
    ck_load_convergence_registry::DoGet_AdvanceEntries().RemoveAll([&](const ck_load_convergence_registry::FAdvanceEntry& InEntry)
    { return InEntry._Name == InName; });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FCk_LoadConvergenceRegistry::
    Run_Advances(
        FCk_Registry& InRegistry)
    -> void
{
    const auto RegistryIsValid = ck::IsValid(InRegistry);
    CK_ENSURE_IF_NOT(RegistryIsValid,
        TEXT("Load convergence was asked to advance an invalid registry. Running nothing: an advance drives real "
             "work against a world, and there is no world here to drive"))
    { return; }

    // A copy, because an advance runs owner code that may register or unregister one.
    const auto EntriesCopy = ck_load_convergence_registry::DoGet_AdvanceEntries();

    for (const auto& Entry : EntriesCopy)
    { Entry._Advance(InRegistry); }
}

// --------------------------------------------------------------------------------------------------------------------
