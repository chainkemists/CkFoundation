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

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|StaticMesh",
              DisplayName = "[Ck] Get Relative Transform (Static Mesh)")
    static FTransform
    Get_RelativeSocketTransform(
        UStaticMesh* InMesh,
        FName InSocketName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|StaticMesh",
              DisplayName = "[Ck] Get Relative Socket Location (Static Mesh)")
    static FVector
    Get_RelativeSocketLocation(
        UStaticMesh* InMesh,
        FName InSocketName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|StaticMesh",
              DisplayName = "[Ck] Get Relative Socket Rotation (Static Mesh)")
    static FRotator
    Get_RelativeSocketRotation(
        UStaticMesh* InMesh,
        FName InSocketName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|StaticMesh",
              DisplayName = "[Ck] Get Relative Socket Scale (Static Mesh)")
    static FVector
    Get_RelativeSocketScale(
        UStaticMesh* InMesh,
        FName InSocketName);
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

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SkeletalMesh",
              DisplayName = "[Ck] Get Relative Socket Transform (Skeletal Mesh)")
    static FTransform
    Get_RelativeSocketTransform(
        USkeletalMesh* InMesh,
        FName InSocketName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SkeletalMesh",
              DisplayName = "[Ck] Get Relative Socket Location (Skeletal Mesh)")
    static FVector
    Get_RelativeSocketLocation(
        USkeletalMesh* InMesh,
        FName InSocketName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SkeletalMesh",
              DisplayName = "[Ck] Get Relative Socket Rotation (Skeletal Mesh)")
    static FRotator
    Get_RelativeSocketRotation(
        USkeletalMesh* InMesh,
        FName InSocketName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SkeletalMesh",
              DisplayName = "[Ck] Get Relative Socket Scale (Skeletal Mesh)")
    static FVector
    Get_RelativeSocketScale(
        USkeletalMesh* InMesh,
        FName InSocketName);
};

// --------------------------------------------------------------------------------------------------------------------
