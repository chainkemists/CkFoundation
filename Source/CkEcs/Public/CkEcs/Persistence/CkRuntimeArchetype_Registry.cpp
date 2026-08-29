#include "CkEcs/Persistence/CkRuntimeArchetype_Registry.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_runtime_archetype_registry
{
    struct FEntry
    {
        FName _ProviderId;
        ck::FCk_RuntimeArchetypeRegistry::FResolver _Resolver;
    };

    // Registration ORDER is preserved rather than hashed: two providers could in principle both claim a path,
    // and a resolution order that reshuffles between runs is one nobody can reproduce.
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
    ck::FCk_RuntimeArchetypeRegistry::
    Register(
        FName InProviderId,
        FResolver InResolver)
    -> void
{
    const auto ProviderIdIsUsable = NOT InProviderId.IsNone();
    CK_ENSURE_IF_NOT(ProviderIdIsUsable,
        TEXT("A runtime-archetype provider was registered with no id. The id is how a provider is REPLACED on "
             "re-registration and how it is named in a diagnostic — an unnamed one can be neither"))
    { return; }

    const auto ResolverIsBound = static_cast<bool>(InResolver);
    CK_ENSURE_IF_NOT(ResolverIsBound,
        TEXT("Runtime-archetype provider [{}] was registered unbound — it can never resolve a path, so every "
             "recipe that needed it would still fail, just later and with a provider registered"), InProviderId)
    { return; }

    auto& Entries = ck_runtime_archetype_registry::DoGet_Entries();

    if (auto* Existing = Entries.FindByPredicate([&](const ck_runtime_archetype_registry::FEntry& InEntry)
        { return InEntry._ProviderId == InProviderId; }))
    {
        Existing->_Resolver = MoveTemp(InResolver);
        return;
    }

    Entries.Emplace(ck_runtime_archetype_registry::FEntry{InProviderId, MoveTemp(InResolver)});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FCk_RuntimeArchetypeRegistry::
    Unregister(
        FName InProviderId)
    -> void
{
    ck_runtime_archetype_registry::DoGet_Entries().RemoveAll(
        [&](const ck_runtime_archetype_registry::FEntry& InEntry)
        { return InEntry._ProviderId == InProviderId; });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FCk_RuntimeArchetypeRegistry::
    TryResolve(
        const FSoftObjectPath& InPath)
    -> UCk_Entity_ConstructionScript_PDA*
{
    if (InPath.IsNull())
    { return nullptr; }

    for (const auto& Entry : ck_runtime_archetype_registry::DoGet_Entries())
    {
        if (auto* Resolved = Entry._Resolver(InPath);
            ck::IsValid(Resolved))
        { return Resolved; }
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FCk_RuntimeArchetypeRegistry::
    Get_HasAnyProvider()
    -> bool
{
    return ck_runtime_archetype_registry::DoGet_Entries().Num() > 0;
}

// --------------------------------------------------------------------------------------------------------------------
