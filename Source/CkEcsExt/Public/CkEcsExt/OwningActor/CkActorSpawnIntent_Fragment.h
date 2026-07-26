#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkActorSpawnIntent_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Read by the CkSnapshot respawn pass. The class path is a plain FString, NOT a TSoftClassPtr — see
// CkEcsExt/CLAUDE.md § "Actor rebind after a snapshot restore".
USTRUCT(BlueprintType)
struct CKECSEXT_API FFragment_ActorSpawnIntent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FFragment_ActorSpawnIntent);

private:
    // Bare `SaveGame` specifier (sets CPF_SaveGame), NOT `meta=(SaveGame)`: a save archive gates SerializeItem
    // on CPF_SaveGame, which the meta tag does NOT set, so meta=(SaveGame) fields round-trip empty.
    UPROPERTY(SaveGame)
    FString _ActorClassPath;

public:
    CK_PROPERTY_GET(_ActorClassPath);
    CK_DEFINE_CONSTRUCTORS(FFragment_ActorSpawnIntent, _ActorClassPath);
};

// --------------------------------------------------------------------------------------------------------------------
