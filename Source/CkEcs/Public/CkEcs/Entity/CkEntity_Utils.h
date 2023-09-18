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
              Category = "Ck|Utils|Entity",
              meta = (CompactNodeTitle = "==", KeyWords = "==,equal"))
    static bool
    Get_Entity_IsEqual(
        const FCk_Entity& InEntityA,
        const FCk_Entity& InEntityB);

    UFUNCTION(BlueprintPure,
           Category = "Ck|Utils|Entity",
           meta = (CompactNodeTitle = "!=", KeyWords = "!=,not,equal"))
    static bool
    Get_Entity_IsNotEqual(
        const FCk_Entity& InEntityA,
        const FCk_Entity& InEntityB);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Entity",
              meta = (DisplayName = "Entity To Text", CompactNodeTitle = "->", BlueprintAutocast))
    static FText
    Conv_EntityToText(
        const FCk_Entity& InEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Entity",
              meta = (DisplayName = "Entity To String", CompactNodeTitle = "->", BlueprintAutocast))
    static FString
    Conv_EntityToString(
        const FCk_Entity& InEntity);

private:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Entity",
        meta = (NativeBreakFunc))
    static void
    Break_Entity(const FCk_Entity& InEntity,
        int32& OutEntityID,
        int32& OutEntityNumber,
        int32& OutEntityVersion);
};

// --------------------------------------------------------------------------------------------------------------------
