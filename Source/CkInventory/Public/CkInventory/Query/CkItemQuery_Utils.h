#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkInventory/Query/CkItemQuery_Fragment_Data.h"

#include "CkItemQuery_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKINVENTORY_API UCk_Utils_ItemQuery_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ItemQuery_UE);

public:
    // Deferred query over item definitions — there is no synchronous getter. The delegate fires on
    // the ItemQuery processor's next tick once the definition index is ready (the very next tick
    // when already built). InAnyHandle only scopes the transient request entity — any valid entity.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ItemQuery",
              DisplayName = "[Ck][ItemQuery] Request Query Item Definitions")
    static void
    Request_QueryItemDefinitions(
        const FCk_Handle& InAnyHandle,
        const FCk_Request_ItemQuery_QueryDefinitions& InRequest,
        const FCk_Delegate_ItemQuery_OnQueried& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
