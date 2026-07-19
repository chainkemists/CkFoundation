#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkJoltStaticWorld_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class AActor;

// --------------------------------------------------------------------------------------------------------------------

/// BP/AS-facing ray hit against the Jolt static world (Phase-1 introspection surface; the general
/// channel-filtered query API arrives with the Phase-2 layer table).
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Jolt_StaticWorldRayHit_Result
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Jolt_StaticWorldRayHit_Result);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _HasHit = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _Position = FVector::ZeroVector;

    // The hit body's source-actor attribution entity (FCk_Handle_JoltStaticActor), resolved from the body's
    // Jolt user-data; INVALID when the body has no live entity. Cast it via UCk_Utils_JoltStaticActor_UE.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle _Entity;

public:
    CK_PROPERTY(_HasHit);
    CK_PROPERTY(_Position);
    CK_PROPERTY(_Entity);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKJOLT_API UCk_Utils_JoltStaticWorld_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_JoltStaticWorld_UE);

public:
    /// Extracts and adds static Jolt bodies for InActor (same extraction the level-streaming path
    /// and the cooker use). Returns the number of bodies added.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Jolt",
              DisplayName="[Ck][Jolt] Request Bake Actor Into Static World")
    static int32
    Request_BakeActor(
        AActor* InActor);

    /// Removes bodies previously added via Request_BakeActor.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Jolt",
              DisplayName="[Ck][Jolt] Request Remove Actor From Static World")
    static void
    Request_RemoveActor(
        AActor* InActor);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Jolt",
              meta = (WorldContext = "InWorldContextObject"),
              DisplayName="[Ck][Jolt] Get Num Static World Bodies")
    static int32
    Get_NumStaticBodies(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Jolt",
              meta = (WorldContext = "InWorldContextObject"),
              DisplayName="[Ck][Jolt] Get Num Unique Static World Shapes")
    static int32
    Get_NumUniqueShapes(
        const UObject* InWorldContextObject);

    /// Ray against static-world bodies ONLY (probes and future dynamic bodies are not hit).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Jolt",
              meta = (WorldContext = "InWorldContextObject"),
              DisplayName="[Ck][Jolt] Ray Cast Static World")
    static FCk_Jolt_StaticWorldRayHit_Result
    Get_RayCastStaticWorld(
        const UObject* InWorldContextObject,
        FVector InStart,
        FVector InEnd);
};

// --------------------------------------------------------------------------------------------------------------------
