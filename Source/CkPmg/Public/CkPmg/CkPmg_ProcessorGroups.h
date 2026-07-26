#pragma once

#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FGroup_Pmg_DebugShape_Setup
    {
        using RunAfter = TDepList<FGroup_Gameplay_Audio>;
    };
}
