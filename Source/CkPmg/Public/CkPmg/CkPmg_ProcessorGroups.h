#pragma once

#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------
// PMG-local scheduler group: runs every per-debug-shape Setup processor in a
// dedicated slot before FGroup_Gameplay_Rendering's regular contents. The
// barrier guarantees FProcessor_Pmg_DebugShape_UpdateTransform (and the
// duration/lifetime processors that sit in FGroup_Gameplay_Rendering) never
// race with an in-flight Setup. Without this, the DebugShape UpdateTransform
// could iterate an entity whose FTag_Pmg_DebugShape_NeedsSetup had already
// been stripped by a shape Setup that had not yet assigned its mesh component.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FGroup_Pmg_DebugShape_Setup
    {
        using RunAfter = TDepList<FGroup_Gameplay_Audio>;
    };
}
