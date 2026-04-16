#pragma once

#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"

#include "CkEcs/Request/CkRequest_Data.h"

// --------------------------------------------------------------------------------------------------------------------
// DEBUG REQUEST STRUCTS
// --------------------------------------------------------------------------------------------------------------------

struct CKSTATEMACHINE_API FCk_Request_SmDebug_RecordTransition
{
    CK_GENERATED_BODY(FCk_Request_SmDebug_RecordTransition);

private:
    TSubclassOf<UCk_SmState_EntityScript> _FromStateClass;
    TSubclassOf<UCk_SmState_EntityScript> _ToStateClass;
    TArray<FString> _ConditionNames;
    double _RealTimeSeconds = 0.0;
    int64 _FrameNumber = 0;

public:
    CK_PROPERTY_GET(_FromStateClass);
    CK_PROPERTY_GET(_ToStateClass);
    CK_PROPERTY_GET(_ConditionNames);
    CK_PROPERTY_GET(_RealTimeSeconds);
    CK_PROPERTY_GET(_FrameNumber);

    CK_PROPERTY_SET(_ConditionNames);
    CK_PROPERTY_SET(_RealTimeSeconds);
    CK_PROPERTY_SET(_FrameNumber);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_SmDebug_RecordTransition, _FromStateClass, _ToStateClass);
};

// --------------------------------------------------------------------------------------------------------------------
