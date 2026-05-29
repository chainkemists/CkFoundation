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
    // Deferred query over item definitions. The completion delegate fires on the
    // ItemQuery processor's next tick once the definition index is ready: as soon
    // as the next tick if the index is already built, or once the async index
    // build finishes otherwise. Mirrors
    // UCk_Utils_RenderStatus_UE::Request_QueryRenderedActors.
    //
    // Querying is ALWAYS deferred through this request — there is no synchronous
    // getter. Design-time / non-ECS callers cannot run this request.
    //
    // InAnyHandle only scopes the transient request entity; it carries no meaning
    // beyond "any valid entity in this world" — ck::TransientEntity() is fine.
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
