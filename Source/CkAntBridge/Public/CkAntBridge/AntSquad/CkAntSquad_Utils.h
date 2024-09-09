#pragma once

#include "CkAntSquad_Fragment_Data.h"
#include "CkHandle_TypeSafe.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkNet/CkNet_Utils.h"

#include "CkAntSquad_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKANTBRIDGE_API UCk_Utils_AntSquad_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AntSquad_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_AntSquad);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AntSquad",
              DisplayName="[Ck][AntSquad] Add Feature")
    static FCk_Handle_AntSquad
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_AntSquad_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AntSquad",
              DisplayName="[Ck][AntSquad] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AntSquad",
              DisplayName="[Ck][AntSquad] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_AntSquad
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AntSquad",
              DisplayName="[Ck][AntSquad] Handle -> AntSquad Handle",
              meta = (CompactNodeTitle = "<AsAntSquad>", BlueprintAutocast))
    static FCk_Handle_AntSquad
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AntSquad",
              DisplayName="[Ck][AntSquad] Request Add Agent",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_AntSquad
    Request_AddAgent(
        UPARAM(ref) FCk_Handle_AntSquad& InAntSquadHandle,
        const FCk_Request_AntSquad_AddAgent& InRequest);

public:
    // TODO: Add Bind/Unbind
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKANTBRIDGE_API UCk_Utils_AntAgent_Renderer_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AntAgent_Renderer_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_AntAgent_Renderer);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AntAgentRenderer",
              DisplayName="[Ck][AntAgentRenderer] Add Feature")
    static FCk_Handle_AntAgent_Renderer
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_AntAgent_Renderer_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AntAgentRenderer",
              DisplayName="[Ck][AntSquad] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AntAgentRenderer",
              DisplayName="[Ck][AntSquad] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_AntAgent_Renderer
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AntAgentRenderer",
              DisplayName="[Ck][AntAgentRenderer] Handle -> AntAgentRenderer Handle",
              meta = (CompactNodeTitle = "<AsAntAgentRenderer>", BlueprintAutocast))
    static FCk_Handle_AntAgent_Renderer
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Renderer|ISM",
              DisplayName="[Ck][IsmRenderer] Request Render Instance",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_AntAgent_Renderer
    Request_RenderInstance(
        UPARAM(ref) FCk_Handle_AntAgent_Renderer& InHandle,
        const FCk_Request_InstancedStaticMeshRenderer_NewInstance& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------
