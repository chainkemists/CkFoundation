#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <NavAreas/NavArea.h>

#include "CkNavArea_Restricted.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Generic "restricted" nav area. Traversal semantics are defined entirely by each
// agent's UNavigationQueryFilter: filters that mark this area bIsExcluded cannot
// path through it; filters that don't, traverse it at normal cost.
UCLASS()
class CKNAVIGATION_API UCk_NavArea_Restricted : public UNavArea
{
    GENERATED_BODY()

public:
    UCk_NavArea_Restricted();
};
