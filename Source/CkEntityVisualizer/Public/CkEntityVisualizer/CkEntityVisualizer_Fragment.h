#pragma once

#include "CkEcs/Tag/CkTag.h"

namespace ck
{
    // Excludes visual children from source discovery. Transient because visualizer entities are
    // derived editor/runtime presentation state and must never enter persistence.
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_EntityVisualizer_Visual);
}
