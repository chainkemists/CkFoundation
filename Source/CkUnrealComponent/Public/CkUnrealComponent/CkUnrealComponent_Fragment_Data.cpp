#include "CkUnrealComponent_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_UnrealComponent_ParamsData::
FCk_Fragment_UnrealComponent_ParamsData(
    TSubclassOf<UActorComponent> InComponentClass)
    : _ComponentClass(InComponentClass)
{
}

FCk_Fragment_UnrealComponent_ParamsData::
FCk_Fragment_UnrealComponent_ParamsData(
    UActorComponent* InComponentArchetype)
    : _ComponentClass(InComponentArchetype != nullptr ? InComponentArchetype->GetClass() : nullptr)
    , _ComponentArchetype(InComponentArchetype)
{
}

// --------------------------------------------------------------------------------------------------------------------
