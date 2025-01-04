#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "Flow/Public/FlowAsset.h"

#include <GameplayTagContainer.h>

#include "CkFlowGraph_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKFLOWGRAPH_API FCk_Handle_FlowGraph : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_FlowGraph); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_FlowGraph);

//--------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_FlowGraph_Status : uint8
{
    Running,
    NotRunning
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_FlowGraph_Status);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFLOWGRAPH_API FCk_Fragment_FlowGraph_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_FlowGraph_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "FlowGraph"))
    FGameplayTag _FlowGraphName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UFlowAsset> _RootFlow;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    EFlowNetMode _FlowGraphNetMode = EFlowNetMode::Authority;

public:
    CK_PROPERTY_GET(_FlowGraphName);
    CK_PROPERTY_GET(_RootFlow);
    CK_PROPERTY_GET(_FlowGraphNetMode);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFLOWGRAPH_API FCk_Fragment_MultipleFlowGraph_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleFlowGraph_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_FlowGraphName"))
    TArray<FCk_Fragment_FlowGraph_ParamsData> _FlowGraphParams;

public:
    CK_PROPERTY_GET(_FlowGraphParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleFlowGraph_ParamsData, _FlowGraphParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFLOWGRAPH_API FCk_Request_FlowGraph_Stop : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_FlowGraph_Stop);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKFLOWGRAPH_API FCk_Request_FlowGraph_Start : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_FlowGraph_Start);
};

// --------------------------------------------------------------------------------------------------------------------