#pragma once

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork_editor::designer
{
    /**
     * Activate the Ck Path Network editor mode and, when the interactive Level Editor has
     * registered it, open or focus the docked toolbox. The operation is ignored outside a
     * usable non-PIE editor world.
     */
    CKPATHNETWORKEDITOR_API auto
    Open_Designer() -> void;

    /** Whether Open_Designer can operate on the current editor state. */
    CKPATHNETWORKEDITOR_API auto
    Can_OpenDesigner() -> bool;
}

// --------------------------------------------------------------------------------------------------------------------
