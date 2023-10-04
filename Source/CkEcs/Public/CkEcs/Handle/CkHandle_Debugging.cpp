#include "CkHandle_Debugging.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FEntity_FragmentMapper::
    ProcessAll(
        const FCk_Handle& InHandle) const
    -> TArray<FName>
{
    TArray<FName> FragmentNames;

    _AllFragments.Reset();
    ck::algo::ForEachIsValid(_GetFragments.begin(), _GetFragments.end(),
    [&](Concept_GetFragment_PolyType FragmentGetter)
    {
        FragmentNames.Emplace(FragmentGetter->Get_FragmentName());
        if (const auto Fragment = FragmentGetter->Get_Fragment(InHandle))
        {
            _AllFragments.Emplace(Fragment);
        }
    },
    [&](const Concept_GetFragment_PolyType& FragmentGetter) -> bool
    {
        CK_ENSURE_IF_NOT(FragmentGetter,
            TEXT("Encountered a Concept that is not valid for Entity [{}]. "
            "This is unexpected unless the Handle Debugging logic has changed."),
            InHandle)
        { return false; }

        return true;
    });

    return FragmentNames;
}

// --------------------------------------------------------------------------------------------------------------------
