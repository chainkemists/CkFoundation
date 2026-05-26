#pragma once

#include "CoreMinimal.h"

#include "CkDependencyProvider_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_DependencyProvider_Scope : uint8
{
    // Scoped to the current UWorld. Provider dies with the world.
    World,

    // Singleton across map travel. Provider lives with the UGameInstance.
    GameInstance
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_DependencyProvider_OverwritePolicy : uint8
{
    // CK_ENSURE_IF_NOT on duplicate type registration. Default — matches
    // FCk_ReplicatedFragmentHandlerRegistry behaviour.
    EnsureOnDuplicate,

    // Silently replace the prior registration. Use only when re-registration
    // is part of the design (e.g. respawnable singletons).
    Overwrite
};

// --------------------------------------------------------------------------------------------------------------------

// Result of the per-Construct injection pass. AllResolved → proceed to Construct;
// Pending → defer (tag the entity, wait for provider callback or deadline ensure).
UENUM()
enum class ECk_DependencyProvider_Resolution : uint8
{
    AllResolved,
    Pending
};

// --------------------------------------------------------------------------------------------------------------------

// Internal — not Blueprint-exposed. Built by FCk_InjectionCache from
// reflection over an EntityScript UClass, consumed by the injection pass.
struct CKECS_API FCk_InjectionSite
{
    // The reflected property on the EntityScript whose value the injection
    // pass writes into. Stored as raw FProperty* because FStructProperty has
    // no GENERATED_BODY and is owned by the UClass.
    FStructProperty*                  _PropertyOnScript = nullptr;

    // The typed handle's UScriptStruct (e.g. FCk_Handle_DayCycle::StaticStruct()).
    // Used as the lookup key against the provider map.
    UScriptStruct*                    _HandleType       = nullptr;

    // Parsed from the `CkInject` meta value. Defaults to World when the meta
    // value is empty.
    ECk_DependencyProvider_Scope      _Scope            = ECk_DependencyProvider_Scope::World;
};
