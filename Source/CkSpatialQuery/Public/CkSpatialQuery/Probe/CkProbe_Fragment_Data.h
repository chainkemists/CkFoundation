#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkPhysics/Public/CkPhysics/CkPhysics_Common.h"

#include <GameplayTagContainer.h>

#include "CkProbe_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Probe_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSPATIALQUERY_API FCk_Handle_Probe : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Probe); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Probe);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Fragment_Probe_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Probe_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_ShapeType _Shape = ECk_ShapeType::Box;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector _HalfExtents = FVector(1.0f, 1.0f, 1.0f);

public:
    CK_PROPERTY_GET(_Shape);
    CK_PROPERTY_GET(_HalfExtents);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Fragment_MultipleProbe_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleProbe_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_Probe_ParamsData> _ProbeParams;

public:
    CK_PROPERTY_GET(_ProbeParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleProbe_ParamsData, _ProbeParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Request_Probe_ExampleRequest : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_Probe_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_Probe_ExampleRequest);
};

// --------------------------------------------------------------------------------------------------------------------