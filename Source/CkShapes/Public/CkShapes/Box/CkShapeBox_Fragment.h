#pragma once

#include "CkShapeBox_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkShapeBox_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ShapeBox_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_ShapeBox_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_ShapeBox_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeBox_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeBox_Params);

    public:
        using ParamsType = FCk_Fragment_ShapeBox_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ShapeBox_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeBox_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeBox_Current);

    public:
        friend class FProcessor_ShapeBox_Setup;
        friend class FProcessor_ShapeBox_HandleRequests;
        friend class FProcessor_ShapeBox_Teardown;
        friend class UCk_Utils_ShapeBox_UE;

    private:
        // Add your properties here
        int32 _DummyProperty = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeBox_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeBox_Requests);

    public:
        friend class FProcessor_ShapeBox_HandleRequests;
        friend class UCk_Utils_ShapeBox_UE;

    public:
    	using RequestType = std::variant
        <
            FCk_Request_ShapeBox_ExampleRequest
        >;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

	// --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_ShapeBox_Replicate; }

UCLASS(Blueprintable)
class CKSHAPES_API UCk_Fragment_ShapeBox_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_ShapeBox_Rep);

public:
    friend class ck::FProcessor_ShapeBox_Replicate;
};

// --------------------------------------------------------------------------------------------------------------------