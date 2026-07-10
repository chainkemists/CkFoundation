#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Archetype/CkArchetype_Data.h"
#include "CkEcs/Handle/CkHandle.h"

#include <CoreMinimal.h>

#include "CkArchetype_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// BP/AS surface over ck::archetype_registry (see CkArchetype_Registry.h for matching
// semantics and the resolution split).
// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_Archetype_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Archetype_UE);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck][Archetype] Request Register",
              Category = "Ck|Utils|Archetype")
    static void
    Request_Register(
        const FCk_ArchetypeDescriptor& InDescriptor);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck][Archetype] Request Register (From Definition)",
              Category = "Ck|Utils|Archetype")
    static void
    Request_Register_FromDefinition(
        const UCk_ArchetypeDefinition* InDefinition);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Archetype] Get Matches",
              Category = "Ck|Utils|Archetype")
    static bool
    Get_Matches(
        const FCk_Handle& InHandle,
        FName InArchetypeName);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Archetype] TryGet Best Match Name",
              Category = "Ck|Utils|Archetype")
    static FName
    TryGet_BestMatchName(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Archetype] Get All Archetype Names",
              Category = "Ck|Utils|Archetype")
    static TArray<FName>
    Get_AllArchetypeNames();
};
