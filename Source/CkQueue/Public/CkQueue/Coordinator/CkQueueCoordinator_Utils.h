#pragma once

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkQueue/Coordinator/CkQueueCoordinator_Fragment.h"

#include "CkQueueCoordinator_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_QueueCoordinator"))
class CKQUEUE_API UCk_Utils_QueueCoordinator_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_QueueCoordinator_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_QueueCoordinator);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|QueueCoordinator",
              DisplayName = "[Ck][QueueCoordinator] Add")
    static FCk_Handle_QueueCoordinator
    Add(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_QueueCoordinator_ParamsData& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|QueueCoordinator",
              DisplayName = "[Ck][QueueCoordinator] Has Any QueueCoordinator")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|QueueCoordinator",
              DisplayName = "[Ck][QueueCoordinator] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_QueueCoordinator
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|QueueCoordinator",
              DisplayName = "[Ck][QueueCoordinator] Handle -> QueueCoordinator Handle",
              meta = (CompactNodeTitle = "<AsQueueCoordinator>", BlueprintAutocast))
    static FCk_Handle_QueueCoordinator
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|QueueCoordinator",
              DisplayName = "[Ck] Get Invalid QueueCoordinator Handle",
              meta = (CompactNodeTitle = "INVALID_QueueCoordinatorHandle", Keywords = "make"))
    static FCk_Handle_QueueCoordinator
    Get_InvalidHandle()
    { return {}; }

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|QueueCoordinator",
              DisplayName = "[Ck][QueueCoordinator] Get Services")
    static TArray<FCk_QueueCoordinator_Service>
    Get_Services(
        const FCk_Handle_QueueCoordinator& InCoordinator);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|QueueCoordinator",
              DisplayName = "[Ck][QueueCoordinator] Get Revision")
    static int32
    Get_Revision(
        const FCk_Handle_QueueCoordinator& InCoordinator);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|QueueCoordinator",
              DisplayName = "[Ck][QueueCoordinator] Request Register Queue",
              meta = (AutoCreateRefTerm = "InCompletionDelegate"))
    static FCk_Handle_QueueCoordinator
    Request_RegisterQueue(
        UPARAM(ref) FCk_Handle_QueueCoordinator& InCoordinator,
        const FCk_Request_QueueCoordinator_RegisterQueue& InRequest,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|QueueCoordinator",
              DisplayName = "[Ck][QueueCoordinator] Request Unregister Queue",
              meta = (AutoCreateRefTerm = "InCompletionDelegate"))
    static FCk_Handle_QueueCoordinator
    Request_UnregisterQueue(
        UPARAM(ref) FCk_Handle_QueueCoordinator& InCoordinator,
        const FCk_Request_QueueCoordinator_UnregisterQueue& InRequest,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|QueueCoordinator",
              DisplayName = "[Ck][QueueCoordinator] Request Select Queue",
              meta = (AutoCreateRefTerm = "InCompletionDelegate"))
    static FCk_Handle_QueueCoordinator
    Request_SelectQueue(
        UPARAM(ref) FCk_Handle_QueueCoordinator& InCoordinator,
        const FCk_Request_QueueCoordinator_SelectQueue& InRequest,
        const FCk_Delegate_QueueCoordinator_OnSelected& InResultDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
