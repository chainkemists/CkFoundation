#pragma once

#include "CkSharedValues.h"

#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkSharedValues_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedBool_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedBool_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get SharedValue [Bool]",
              Category = "Ck|Utils|SharedBool",
              meta     = (CompactNodeTitle = "Get"))
    static bool
    Get(
        const FCk_SharedBool& InShared);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Set SharedValue [Bool]",
              Category = "Ck|Utils|SharedBool",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedBool& InShared,
        bool InValue);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Make SharedValue [Bool]",
              Category = "Ck|Utils|SharedBool",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedBool
    Make(
        bool InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedInt_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedInt_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get SharedValue [Int32]",
              Category = "Ck|Utils|SharedInt",
              meta     = (CompactNodeTitle = "Get"))
    static int32
    Get(
        const FCk_SharedInt& InShared);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Set SharedValue [Int32]",
              Category = "Ck|Utils|SharedInt",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedInt& InShared,
        int32                           InValue);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Make SharedValue [Int32]",
              Category = "Ck|Utils|SharedInt",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedInt
    Make(
        int32 InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedFloat_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedFloat_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get SharedValue [Float]",
              Category = "Ck|Utils|SharedFloat",
              meta     = (CompactNodeTitle = "Get"))
    static float
    Get(
        const FCk_SharedFloat& InShared);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Set SharedValue [Float]",
              Category = "Ck|Utils|SharedFloat",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedFloat& InShared,
        float InValue);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Make SharedValue [Float]",
              Category = "Ck|Utils|SharedFloat",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedFloat
    Make(
        float InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedVec3_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedVec3_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get SharedValue [Vec3]",
              Category = "Ck|Utils|SharedVector",
              meta     = (CompactNodeTitle = "Get"))
    static FVector
    Get(
        const FCk_SharedVec3& InShared);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Set SharedValue [Vec3]",
              Category = "Ck|Utils|SharedVector",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedVec3& InShared,
        FVector InValue);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Make SharedValue [Vec3]",
              Category = "Ck|Utils|SharedVector",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedVec3
    Make(
        FVector InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedVec2_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedVec2_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get SharedValue [Vec2]",
              Category = "Ck|Utils|SharedVector",
              meta     = (CompactNodeTitle = "Get"))
    static FVector2D
    Get(
        const FCk_SharedVec2& InShared);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Set SharedValue [Vec2]",
              Category = "Ck|Utils|SharedVector",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedVec2& InShared,
        FVector2D InValue);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Make SharedValue [Vec2]",
              Category = "Ck|Utils|SharedVector",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedVec2
    Make(
        FVector2D InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedString_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedString_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get SharedValue [String]",
              Category = "Ck|Utils|SharedString",
              meta     = (CompactNodeTitle = "Get"))
    static FString
    Get(
        const FCk_SharedString& InShared);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Set SharedValue [String]",
              Category = "Ck|Utils|SharedString",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedString& InShared,
        FString InValue);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Make SharedValue [String]",
              Category = "Ck|Utils|SharedString",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedString
    Make(
        FString InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedName_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedName_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get SharedValue [Name]",
              Category = "Ck|Utils|SharedName",
              meta     = (CompactNodeTitle = "Get"))
    static FName
    Get(
        const FCk_SharedName& InShared);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Set SharedValue [Name]",
              Category = "Ck|Utils|SharedName",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedName& InShared,
        FName InValue);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Make SharedValue [Name]",
              Category = "Ck|Utils|SharedName",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedName
    Make(
        FName InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedText_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedText_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get SharedValue [Text]",
              Category = "Ck|Utils|SharedText",
              meta     = (CompactNodeTitle = "Get"))
    static FText
    Get(
        const FCk_SharedText& InShared);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Set SharedValue [Text]",
              Category = "Ck|Utils|SharedText",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedText& InShared,
        FText InValue);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Make SharedValue [Text]",
              Category = "Ck|Utils|SharedText",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedText
    Make(
        FText InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedRotator_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedRotator_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get SharedValue [Rotator]",
              Category = "Ck|Utils|SharedRotator",
              meta     = (CompactNodeTitle = "Get"))
    static FRotator
    Get(
        const FCk_SharedRotator& InShared);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Set SharedValue [Rotator]",
              Category = "Ck|Utils|SharedRotator",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedRotator& InShared,
        FRotator InValue);
public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Make SharedValue [Rotator]",
              Category = "Ck|Utils|SharedRotator",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedRotator
    Make(
        FRotator InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedTransform_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedTransform_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get SharedValue [Transform]",
              Category = "Ck|Utils|SharedTransform",
              meta     = (CompactNodeTitle = "Get"))
    static FTransform
    Get(
        const FCk_SharedTransform& InShared);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Set SharedValue [Transform]",
              Category = "Ck|Utils|SharedTransform",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedTransform& InShared,
        FTransform InValue);
public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Make SharedValue [Transform]",
              Category = "Ck|Utils|SharedTransform",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedTransform
    Make(
        FTransform InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedInstancedStruct_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedInstancedStruct_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get SharedValue [InstancedStruct]",
              Category = "Ck|Utils|SharedInstancedStruct",
              meta     = (CompactNodeTitle = "Get"))
    static FInstancedStruct
    Get(
        const FCk_SharedInstancedStruct& InShared);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Set SharedValue [InstancedStruct]",
              Category = "Ck|Utils|SharedInstancedStruct",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedInstancedStruct& InShared,
        const FInstancedStruct& InValue);
public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Make SharedValue [InstancedStruct]",
              Category = "Ck|Utils|SharedInstancedStruct",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedInstancedStruct
    Make(
        const FInstancedStruct& InValue);
};

// --------------------------------------------------------------------------------------------------------------------
