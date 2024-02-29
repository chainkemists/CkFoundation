#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Entity/CkEntity.h"

#include "CkEntity_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_Entity_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Entity_UE);

private:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Entity == Entity",
              Category = "Ck|Utils|Entity",
              meta = (CompactNodeTitle = "==", KeyWords = "==,equal"))
    static bool
    IsEqual(
        const FCk_Entity& InEntityA,
        const FCk_Entity& InEntityB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Entity != Entity",
              Category = "Ck|Utils|Entity",
              meta = (CompactNodeTitle = "!=", KeyWords = "!=,not,equal"))
    static bool
    IsNotEqual(
        const FCk_Entity& InEntityA,
        const FCk_Entity& InEntityB);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Entity -> Text",
              Category = "Ck|Utils|Entity",
              meta = (CompactNodeTitle = "->", BlueprintAutocast))
    static FText
    Conv_EntityToText(
        const FCk_Entity& InEntity);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Entity -> String",
              Category = "Ck|Utils|Entity",
              meta = (CompactNodeTitle = "->", BlueprintAutocast))
    static FString
    Conv_EntityToString(
        const FCk_Entity& InEntity);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Tombstone Entity",
              Category = "Ck|Utils|Entity",
              meta = (CompactNodeTitle = "TOMBSTONE_Entity"))
    static const FCk_Entity&
    Get_TombstoneEntity();

private:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Break Entity",
              Category = "Ck|Utils|Entity",
              meta = (NativeBreakFunc))
    static void
    Break_Entity(const FCk_Entity& InEntity,
        int32& OutEntityID,
        int32& OutEntityNumber,
        int32& OutEntityVersion);
};

// --------------------------------------------------------------------------------------------------------------------
