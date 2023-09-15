#include "CkOverlapBody_Common.h"

#include "CkCore/Enums/CkEnums.h"
#include "CkPhysics/CkPhysics_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_OverlapBody_ActorComponent_Box_UE::
    UCk_OverlapBody_ActorComponent_Box_UE()
{
    const auto DoSet_CanEverTick_ForCCD = [this]() -> void
    {
        this->PrimaryComponentTick.bCanEverTick = true;
    };

    this->bCanEverAffectNavigation  = false;
    this->bTraceComplexOnMove       = false;
    this->bMultiBodyOverlap         = false;
    this->bReturnMaterialOnMove     = false;
    this->bAlwaysCreatePhysicsState = false;
    this->bWantsInitializeComponent = true;

    DoSet_CanEverTick_ForCCD();

    UCk_Utils_Physics_UE::Request_SetGenerateOverlapEvents(this, ECk_EnableDisable::Disable, this);
}

// --------------------------------------------------------------------------------------------------------------------

UCk_OverlapBody_ActorComponent_Sphere_UE::
    UCk_OverlapBody_ActorComponent_Sphere_UE()
{
    const auto DoSet_CanEverTick_ForCCD = [this]() -> void
    {
        this->PrimaryComponentTick.bCanEverTick = true;
    };

    this->bCanEverAffectNavigation  = false;
    this->bTraceComplexOnMove       = false;
    this->bMultiBodyOverlap         = false;
    this->bReturnMaterialOnMove     = false;
    this->bAlwaysCreatePhysicsState = false;
    this->bWantsInitializeComponent = true;

    DoSet_CanEverTick_ForCCD();

    UCk_Utils_Physics_UE::Request_SetGenerateOverlapEvents(this, ECk_EnableDisable::Disable, this);
}

// --------------------------------------------------------------------------------------------------------------------

UCk_OverlapBody_ActorComponent_Capsule_UE::
    UCk_OverlapBody_ActorComponent_Capsule_UE()
{
    const auto DoSet_CanEverTick_ForCCD = [this]() -> void
    {
        this->PrimaryComponentTick.bCanEverTick = true;
    };

    this->bCanEverAffectNavigation  = false;
    this->bTraceComplexOnMove       = false;
    this->bMultiBodyOverlap         = false;
    this->bReturnMaterialOnMove     = false;
    this->bAlwaysCreatePhysicsState = false;
    this->bWantsInitializeComponent = true;

    DoSet_CanEverTick_ForCCD();

    UCk_Utils_Physics_UE::Request_SetGenerateOverlapEvents(this, ECk_EnableDisable::Disable, this);
}

// --------------------------------------------------------------------------------------------------------------------
