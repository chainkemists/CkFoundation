#pragma once

#include "CkMacros/CkMacros.h"
#include "CkEcs/Entity/CkEntity.h"

#include "CkEntity_Utils.generated.h"

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_Entity_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Entity_UE);

private:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Entity",
        meta = (NativeBreakFunc))
    static void
    Break_Entity(FCk_Entity InEntity,
        int32& OutEntityID,
        int32& OutEntityNumber,
        int32& OutEntityVersion);

};
