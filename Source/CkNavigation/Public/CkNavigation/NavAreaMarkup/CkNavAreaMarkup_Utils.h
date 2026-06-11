#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"

#include <AI/Navigation/NavRelevantInterface.h>
#include <Kismet/BlueprintFunctionLibrary.h>
#include <Templates/SubclassOf.h>

#include "CkNavAreaMarkup_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UNavArea;

// Pure-UObject nav-area painter: registers an oriented box of the given UNavArea
// class with the nav octree (RegisterNavRelevantObjectStatic — no Actor involved).
// With dynamic runtime navmesh generation the affected tiles rebuild on
// register/unregister. Lifetime: the caller owns it (keep a UPROPERTY ref so GC
// doesn't collect it); call Request_Destroy to unpaint.
UCLASS()
class CKNAVIGATION_API UCk_NavAreaMarkup_UE : public UObject, public INavRelevantInterface
{
    GENERATED_BODY()

public:
    friend class UCk_Utils_NavAreaMarkup_UE;

public:
    auto GetNavigationBounds() const -> FBox override;
    auto GetNavigationData(FNavigationRelevantData& Data) const -> void override;
    auto IsNavigationRelevant() const -> bool override;

private:
    UPROPERTY()
    FTransform _Transform;

    UPROPERTY()
    FVector _HalfExtents = FVector::ZeroVector;

    UPROPERTY()
    TSubclassOf<UNavArea> _AreaClass;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKNAVIGATION_API UCk_Utils_NavAreaMarkup_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_NavAreaMarkup_UE);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|NavAreaMarkup",
              DisplayName = "[Ck][NavAreaMarkup] Request Create")
    static UCk_NavAreaMarkup_UE*
    Request_Create(
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        const FTransform& InWorldTransform,
        const FVector& InHalfExtents,
        TSubclassOf<UNavArea> InAreaClass);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|NavAreaMarkup",
              DisplayName = "[Ck][NavAreaMarkup] Request Destroy")
    static void
    Request_Destroy(
        UCk_NavAreaMarkup_UE* InMarkup);
};
