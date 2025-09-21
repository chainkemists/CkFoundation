#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include <Engine/LevelStreamingDynamic.h>
#include <Kismet/BlueprintFunctionLibrary.h>
#include <CoreMinimal.h>

#include "CkLevelStreaming_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_TransformOriginPolicy : uint8
{
    WorldOrigin,
    CustomOrigin
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_LevelTransformParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_LevelTransformParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FRotator _Rotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Scale = FVector::OneVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ToolTip = "If true, transforms are calculated relative to custom origin"))
    bool _UseCustomOrigin = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ToolTip = "Custom origin point for transform calculations",
                  EditCondition = "_UseCustomOrigin", EditConditionHides))
    FVector _CustomOrigin = FVector::ZeroVector;

public:
    CK_PROPERTY(_Location);
    CK_PROPERTY(_Rotation);
    CK_PROPERTY(_Scale);
    CK_PROPERTY(_UseCustomOrigin);
    CK_PROPERTY(_CustomOrigin);

public:
    auto Get_Transform(ECk_TransformOriginPolicy InPolicy = ECk_TransformOriginPolicy::WorldOrigin) const -> FTransform;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_LoadLevelParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_LoadLevelParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FString _LevelPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_LevelTransformParams _TransformParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                    ToolTip = "If true, blocks until level is fully loaded"))
    bool _BlockOnLoad = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _InitiallyVisible = true;

public:
    CK_PROPERTY(_LevelPath);
    CK_PROPERTY(_TransformParams);
    CK_PROPERTY(_BlockOnLoad);
    CK_PROPERTY(_InitiallyVisible);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_LoadLevelParams, _LevelPath);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_LevelStreaming_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_LevelStreaming_UE);

private:
    static inline int32 UniqueInstanceIdCounter = 10000;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|LevelStreaming",
              DisplayName = "[Ck] Load Streaming Level (Advanced)",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_LoadStreamingLevel(
        const UObject* InWorldContextObject,
        const FCk_LoadLevelParams& InParams,
        ULevelStreamingDynamic*& OutStreamingLevel);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|LevelStreaming",
              DisplayName = "[Ck] Update Level Transform")
    static void
    Request_UpdateLevelTransform(
        ULevelStreaming* InStreamingLevel,
        const FCk_LevelTransformParams& InTransformParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|LevelStreaming",
              DisplayName = "[Ck] Update Level Transform And Origin")
    static void
    Request_UpdateLevelTransformAndOrigin(
        ULevelStreaming* InStreamingLevel,
        const FCk_LevelTransformParams& InTransformParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|LevelStreaming",
              DisplayName = "[Ck] Unload Streaming Level")
    static void
    Request_UnloadStreamingLevel(
        ULevelStreamingDynamic* InStreamingLevel);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|LevelStreaming",
              DisplayName = "[Ck] Apply Transform To Level Actors")
    static void
    Request_ApplyTransformToLevelActors(
        ULevel* InLevel,
        const FTransform& InTransform,
        bool InPreserveComponentMobility = true);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|LevelStreaming",
              DisplayName = "[Ck] Get All Streaming Levels",
              meta = (WorldContext = "InWorldContextObject"))
    static TArray<ULevelStreaming*>
    Get_AllStreamingLevels(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|LevelStreaming",
              DisplayName = "[Ck] Get Level Transform")
    static FTransform
    Get_LevelTransform(
        const ULevelStreaming* InStreamingLevel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|LevelStreaming",
              DisplayName = "[Ck] Is Level Loaded")
    static bool
    Get_IsLevelLoaded(
        const ULevelStreaming* InStreamingLevel);

private:
    static auto
    Internal_RemoveLevelTransform(
        const ULevelStreaming* InStreamingLevel) -> void;

    static auto
    Internal_ApplyLevelTransform(
        const ULevelStreaming* InStreamingLevel) -> void;

    static auto
    Internal_CreateUniquePackageName(
        const UWorld* InWorld,
        const FString& InLongPackageName) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
