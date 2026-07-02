#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment.h"

#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"
// Full UCk_IskmRenderer_Data definition — required when this TU compiles OUTSIDE the unity blob (the
// TWeakObjectPtr member ctor static_asserts on a complete UObject type).
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment_Data.h"

namespace ck
{
    FFragment_IskmRenderer_Params::FFragment_IskmRenderer_Params(UCk_IskmRenderer_Data* InRendererData)
        : _RendererData(InRendererData)
    {
    }
}
