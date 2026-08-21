#pragma once

#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"
#include "CkCrowd/Queue/CkCrowdQueueAdapter_Fragment.h"

#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkCrowdQueueAdapter_Utils.generated.h"

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_CrowdAgent"))
class CKCROWD_API UCk_Utils_CrowdQueueAdapter_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_CrowdQueueAdapter_UE);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdQueue",
              DisplayName = "[Ck][CrowdQueue] Request Join Queue",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_CrowdAgent
    Request_JoinQueue(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdQueue",
              DisplayName = "[Ck][CrowdQueue] Request Restore Join Queue",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_CrowdAgent
    Request_RestoreJoinQueue(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        int64 InRestoredTicket,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdQueue",
              DisplayName = "[Ck][CrowdQueue] Request Leave Queue",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_CrowdAgent
    Request_LeaveQueue(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        const FCk_Delegate_Request_OnCompleted& InDelegate);
};
