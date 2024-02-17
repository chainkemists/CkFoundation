#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Object/CkWorldContextObject.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkProcessorScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ecs_ForEach_Policy : uint8
{
    OnlyValidEntities,
    // Include Entities that are pending kill
    AllEntities
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, EditInlineNew)
class CKECS_API UCk_Ecs_ProcessorScript_Base_UE : public UCk_GameWorldContextObject_UE
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(UCk_Ecs_ProcessorScript_Base_UE);

public:
    using TimeType = FCk_Time;
    using EntityType = FCk_Entity;
    using RegistryType = FCk_Registry;
    using HandleType = FCk_Handle;

private:
    UPROPERTY(EditDefaultsOnly,
              Category = "Config",
              meta = (AllowPrivateAccess = true))
    ECk_Ecs_ForEach_Policy _ForEachPolicy = ECk_Ecs_ForEach_Policy::OnlyValidEntities;

public:
    virtual auto
    Tick(
        TimeType InTime) -> void;

    UFUNCTION(BlueprintNativeEvent)
    bool
    ProcessEntity_If(
        FCk_Handle InEntity) const;

private:
    TOptional<RegistryType> _Registry;

protected:
    FCk_Handle _TransientEntity;

public:
    CK_PROPERTY_GET(_ForEachPolicy);
};

// --------------------------------------------------------------------------------------------------------------------
