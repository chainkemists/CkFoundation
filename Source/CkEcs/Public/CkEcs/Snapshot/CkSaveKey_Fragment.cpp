#include "CkSaveKey_Fragment.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Engine/Level.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>
#include <UObject/Package.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::save_key
{
    namespace
    {
        auto
            DoAssign(
                FCk_Handle& InEntity,
                const FString& InStableIdentity,
                bool InIsSharedRendezvousGroup)
            -> void
        {
            const auto EntityIsValid = ck::IsValid(InEntity);
            CK_ENSURE_IF_NOT(EntityIsValid,
                TEXT("Cannot assign the SaveKey [{}] — the Entity Handle is invalid"), InStableIdentity)
            {}
            if (NOT EntityIsValid)
            { return; }

            // An empty identity hashes to a single shared GUID, so every empty-keyed entity would rendezvous onto the
            // same one and silently consolidate. Reject rather than key.
            const auto IdentityIsValid = NOT InStableIdentity.IsEmpty();
            CK_ENSURE_IF_NOT(IdentityIsValid,
                TEXT("Cannot assign a SaveKey to Entity [{}] — the stable identity is EMPTY"), InEntity)
            {}
            if (NOT IdentityIsValid)
            { return; }

            InEntity.AddOrReplace<FFragment_SaveKey>(FGuid::NewDeterministicGuid(InStableIdentity));
            if (InIsSharedRendezvousGroup)
            { InEntity.Get<FFragment_SaveKey>().MarkSharedRendezvousGroup(); }
        }
    }

    auto
        Assign(
            FCk_Handle& InEntity,
            const FString& InStableIdentity)
        -> void
    {
        DoAssign(InEntity, InStableIdentity, false);
    }

    auto
        AssignSharedRendezvousGroup(
            FCk_Handle& InEntity,
            const FString& InStableIdentity)
        -> void
    {
        DoAssign(InEntity, InStableIdentity, true);
    }

    auto
        AssignAlias(
            FCk_Handle& InEntity,
            const FString& InHistoricalIdentity)
        -> void
    {
        const auto EntityIsValid = ck::IsValid(InEntity);
        CK_ENSURE_IF_NOT(EntityIsValid,
            TEXT("Cannot assign the SaveKey alias [{}] — the Entity Handle is invalid"), InHistoricalIdentity)
        {}
        if (NOT EntityIsValid)
        { return; }

        const auto IdentityIsValid = NOT InHistoricalIdentity.IsEmpty();
        CK_ENSURE_IF_NOT(IdentityIsValid,
            TEXT("Cannot assign a SaveKey alias to Entity [{}] — the historical identity is EMPTY"), InEntity)
        {}
        if (NOT IdentityIsValid)
        { return; }

        const auto HasCanonicalKey = InEntity.Has<FFragment_SaveKey>();
        CK_ENSURE_IF_NOT(HasCanonicalKey,
            TEXT("Cannot assign SaveKey alias [{}] to Entity [{}] before its canonical SaveKey"),
            InHistoricalIdentity, InEntity)
        {}
        if (NOT HasCanonicalKey)
        { return; }

        const auto Alias = FGuid::NewDeterministicGuid(InHistoricalIdentity);
        const auto& SaveKey = InEntity.Get<FFragment_SaveKey>();
        const auto AliasDiffersFromCanonical = Alias != SaveKey.Get_Key();
        CK_ENSURE_IF_NOT(AliasDiffersFromCanonical,
            TEXT("Cannot assign SaveKey alias [{}] to Entity [{}] — it matches the canonical SaveKey"),
            InHistoricalIdentity, InEntity)
        {}
        if (NOT AliasDiffersFromCanonical)
        { return; }

        InEntity.Get<FFragment_SaveKey>().AddAlias(Alias);
    }

    auto
        Get_IsLevelPlaced(
            const AActor* InActor)
        -> bool
    {
        if (ck::Is_NOT_Valid(InActor, ck::IsValid_Policy_NullptrOnly{}))
        { return false; }

        // RF_WasLoaded is set by the linker on every object serialized out of a package and is propagated by
        // duplication (PIE duplicates the editor world with FlagMask = RF_AllFlags), so it answers the same in the
        // editor, in PIE and in a cooked build. SpawnActor constructs its actor instead of loading it, so a runtime
        // spawn never carries it. Preferred over bNetStartup, which is only set for actors with bNetLoadOnClient.
        return InActor->HasAnyFlags(RF_WasLoaded);
    }

    auto
        Get_LevelPlacedIdentity(
            const AActor* InActor)
        -> FString
    {
        if (NOT Get_IsLevelPlaced(InActor))
        { return {}; }

        const auto* Level = InActor->GetLevel();
        if (ck::Is_NOT_Valid(Level, ck::IsValid_Policy_NullptrOnly{}))
        { return {}; }

        // The LEVEL's package, never the actor's: a World-Partition actor lives in its own external package on disk
        // but loses that package when PIE duplicates it, so the actor package answers differently per environment.
        // The PIE prefix carries the instance index, which changes between sessions, hence the strip.
        return UWorld::RemovePIEPrefix(Level->GetPackage()->GetName()) + TEXT("/") + InActor->GetFName().ToString();
    }
}

// --------------------------------------------------------------------------------------------------------------------
