#pragma once

// Resolution of a construction-recipe ARCHETYPE whose object path a fresh process cannot load.
//
// A build recipe records its archetype as an object PATH (CkSnapshot_CaptureV3.cpp, the DefinitionBuilt case).
// That is durable identity for an on-disk asset and NOT durable identity for an archetype minted at runtime:
// UCk_Utils_Item_UE::GetOrCreate_TransientItemDefinition creates RF_Transient definitions under a rooted outer,
// so the path resolves in the process that minted it and in no other. A save/load inside one session therefore
// works — FindObject still answers — while the same save on a cold boot resolves to nothing.
//
// A provider closes that gap by MINTING the archetype from the path, which it can do because the path's leaf is
// the deterministic identity the feature derives the archetype from. Nothing here knows how; a feature that mints
// runtime archetypes registers a resolver and owns the parsing.
//
// Deliberately NOT a fallback that hides the problem: the loader consults this registry, and if no provider
// answers it fails LOUDLY and records a named loss. The registry exists so the failure has a legitimate cure,
// not so it can be swallowed.

#include "CkCore/Macros/CkMacros.h"

#include <Templates/Function.h>
#include <UObject/NameTypes.h>
#include <UObject/SoftObjectPath.h>

class UCk_Entity_ConstructionScript_PDA;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKECS_API FCk_RuntimeArchetypeRegistry
    {
        // Returns the archetype for InPath, or nullptr when this provider does not own that path. Returning
        // nullptr is the NORMAL answer for a provider asked about someone else's path — the loader walks every
        // provider — so a resolver must decide ownership from the path itself and never ensure on a miss.
        //
        // A resolver may CREATE the archetype it returns (that is the point), but it must be idempotent: the
        // loader can ask for the same path more than once within a load, and every ask must yield the same object.
        using FResolver = TFunction<UCk_Entity_ConstructionScript_PDA*(const FSoftObjectPath&)>;

        // Registering the same id twice REPLACES, so a re-registration (hot reload, a test installing its own
        // provider) does not accumulate duplicates under one id.
        static auto
        Register(
            FName InProviderId,
            FResolver InResolver) -> void;

        static auto
        Unregister(
            FName InProviderId) -> void;

        // The first provider that claims InPath, in registration order; nullptr when none does. The caller
        // decides what an unresolved path means — this never ensures, because "no provider owns this path" is
        // also the answer for every ordinary on-disk archetype that simply failed to load.
        static auto
        TryResolve(
            const FSoftObjectPath& InPath) -> UCk_Entity_ConstructionScript_PDA*;

        // Whether anything is registered at all. Lets a diagnostic distinguish "no provider owns this path" from
        // "this build registered no providers", which are very different bugs to chase.
        static auto
        Get_HasAnyProvider() -> bool;
    };
}

// --------------------------------------------------------------------------------------------------------------------
