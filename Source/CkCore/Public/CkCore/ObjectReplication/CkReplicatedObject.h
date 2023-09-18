#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

#include "CkReplicatedObject.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCORE_API UCk_ReplicatedObject_UE : public UObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ReplicatedObject_UE);

public:
    UCk_ReplicatedObject_UE();

public:
    auto GetOwningActor() const -> AActor*;

    virtual auto GetWorld() const -> UWorld* override;
    virtual auto CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack) -> bool override;
    virtual auto GetFunctionCallspace(UFunction* Function, FFrame* Stack) -> int32 override;
    virtual auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;
    virtual auto IsSupportedForNetworking() const -> bool override;
    virtual bool IsNameStableForNetworking() const override;
};

// --------------------------------------------------------------------------------------------------------------------
