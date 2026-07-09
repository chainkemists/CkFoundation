#include "CkHandle_Debugging.h"

// --------------------------------------------------------------------------------------------------------------------

FEntity_FragmentMapper::
    ~FEntity_FragmentMapper()
{
    // Ownership map (see Add_FragmentInfo): every wrapper new'd there is owned by exactly one
    // of _AllFragments, _AllTags, or _DebugNameFragment. _LifetimeTag is a non-owning ALIAS of
    // an _AllTags element — deleting it here would double-free.
    for (const auto Pointer : _AllFragments) { delete Pointer; }
    for (const auto Pointer : _AllTags)      { delete Pointer; }
    delete _DebugNameFragment;
}
