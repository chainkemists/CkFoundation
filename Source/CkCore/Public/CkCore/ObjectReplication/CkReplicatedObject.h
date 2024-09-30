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

    UCk_ReplicatedObject_UE(
        const FObjectInitializer& InObjInitializer);

public:
    auto GetOwningActor() const -> AActor*;

    auto
    CallRemoteFunction(
        UFunction* Function, 
        void* Parms, 
        FOutParmRec* OutParms, 
        FFrame* Stack) -> bool override;

    auto
    GetFunctionCallspace(
        UFunction* Function, 
        FFrame* Stack) -> int32 override;

    auto
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;

    auto
    IsSupportedForNetworking() const -> bool override;

#if UE_WITH_IRIS
	auto
    RegisterReplicationFragments(
        UE::Net::FFragmentRegistrationContext& Context, 
        UE::Net::EFragmentRegistrationFlags RegistrationFlags) -> void override;
#endif // UE_WITH_IRIS
};

// --------------------------------------------------------------------------------------------------------------------
