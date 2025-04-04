#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>
#include <CoreMinimal.h>
#include <Engine/StaticMesh.h>
#include <Engine/SkeletalMesh.h>

#include "CkMesh_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_StaticMesh_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_StaticMesh_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|StaticMesh",
              DisplayName = "[Ck] Get Triangle Count (Static Mesh)")
    static int32
    Get_TriangleCount(
        UStaticMesh* InStaticMesh,
        int32 InLODIndex = 0);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SkeletalMesh_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SkeletalMesh_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SkeletalMesh",
              DisplayName = "[Ck] Get Triangle Count (Skeletal Mesh)")
    static int32
    Get_TriangleCount(
        USkeletalMesh* InSkeletalMesh,
        int32 InLODIndex = 0);
};

// --------------------------------------------------------------------------------------------------------------------
