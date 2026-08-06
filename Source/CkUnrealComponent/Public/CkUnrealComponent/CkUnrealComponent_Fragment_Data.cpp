#include "CkUnrealComponent_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_UnrealComponent_Spec::
FCk_UnrealComponent_Spec(
    TSubclassOf<UActorComponent> InComponentClass)
    : _ComponentClass(InComponentClass)
{
}

FCk_UnrealComponent_Spec::
FCk_UnrealComponent_Spec(
    UActorComponent* InComponentArchetype)
    : _ComponentClass(InComponentArchetype != nullptr ? InComponentArchetype->GetClass() : nullptr)
    , _ComponentArchetype(InComponentArchetype)
{
}

// --------------------------------------------------------------------------------------------------------------------
