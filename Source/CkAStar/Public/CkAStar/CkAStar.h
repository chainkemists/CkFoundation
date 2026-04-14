#pragma once

// Algorithm layer (pure C++ templates, no ECS)
#include "CkAStar/Algorithm/CkAStar_GraphConcept.h"
#include "CkAStar/Algorithm/CkAStar_Types.h"
#include "CkAStar/Algorithm/CkAStar_Search.h"
#include "CkAStar/Algorithm/CkAStar_PathCache.h"
#include "CkAStar/Algorithm/CkAStar_PrecomputedTable.h"

// ECS layer (template fragments and processors for consumers to inherit)
#include "CkAStar/CkAStar_Fragment_Data.h"
#include "CkAStar/CkAStar_Fragment.h"
#include "CkAStar/CkAStar_Processor.h"
