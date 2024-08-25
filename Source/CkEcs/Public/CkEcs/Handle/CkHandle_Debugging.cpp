#include "CkHandle_Debugging.h"

// --------------------------------------------------------------------------------------------------------------------

FEntity_FragmentMapper::
    ~FEntity_FragmentMapper()
{
    for (const auto Pointer : _AllFragments) { delete Pointer; }
}
