#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkProfile/Stats/CkStats.h"

#include "CkProcessorScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType)
class CKECS_API UCk_Ecs_ProcessorScript_Base : public UObject
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(UCk_Ecs_ProcessorScript_Base);

public:
    using TimeType = FCk_Time;
    using EntityType = FCk_Entity;
    using RegistryType = FCk_Registry;
    using HandleType = FCk_Handle;

public:
    virtual auto
    Tick(TimeType InTime) -> void;

    UFUNCTION(BlueprintImplementableEvent)
    void
    DoTick(FCk_Time InTime, FCk_Handle InEntity);

protected:
    TOptional<RegistryType> _Registry;
};
