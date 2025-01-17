#pragma once

#include "CkShapeCylinder_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkShapeCylinder_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ShapeCylinder_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_ShapeCylinder_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_ShapeCylinder_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeCylinder_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeCylinder_Params);

    public:
        using ParamsType = FCk_Fragment_ShapeCylinder_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ShapeCylinder_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeCylinder_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeCylinder_Current);

    public:
        friend class FProcessor_ShapeCylinder_Setup;
        friend class FProcessor_ShapeCylinder_HandleRequests;
        friend class FProcessor_ShapeCylinder_Teardown;
        friend class UCk_Utils_ShapeCylinder_UE;

    private:
        // Add your properties here
        int32 _DummyProperty = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeCylinder_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeCylinder_Requests);

    public:
        friend class FProcessor_ShapeCylinder_HandleRequests;
        friend class UCk_Utils_ShapeCylinder_UE;

    public:
    	using RequestType = std::variant
        <
            FCk_Request_ShapeCylinder_ExampleRequest
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

namespace ck { class FProcessor_ShapeCylinder_Replicate; }

UCLASS(Blueprintable)
class CKSHAPES_API UCk_Fragment_ShapeCylinder_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_ShapeCylinder_Rep);

public:
    friend class ck::FProcessor_ShapeCylinder_Replicate;
};

// --------------------------------------------------------------------------------------------------------------------