#pragma once

// Algorithm layer (pure C++ templates, no ECS)
#include "CkGoap/Algorithm/CkGoap_WorldState.h"
#include "CkGoap/Algorithm/CkGoap_Types.h"
#include "CkGoap/Algorithm/CkGoap_Graph.h"

// ECS layer (fragments, processors, signals)
#include "CkGoap/CkGoap_Fragment_Data.h"
#include "CkGoap/CkGoap_Fragment.h"
#include "CkGoap/CkGoap_Processor.h"

// WorldState sub-feature — owns the shared boolean world state planners read/write.
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment_Data.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment.h"
