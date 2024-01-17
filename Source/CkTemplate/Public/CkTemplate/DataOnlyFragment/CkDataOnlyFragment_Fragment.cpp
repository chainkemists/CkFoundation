#include "CkDataOnlyFragment_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Processor_DataOnlyFragment_UE::
    Tick(
        TimeType InTime)
    -> void
{
    CK_STAT(STAT_Tick);

    Super::Tick(InTime);
    _Registry->View<FCk_Fragment_DataOnlyFragment>().ForEach(
    [&](EntityType InEntity, FCk_Fragment_DataOnlyFragment& InFragment)
    {
        CK_STAT(STAT_ForEachEntity);

        const auto Handle = HandleType{InEntity, *_Registry};
        DoTick(InTime, Handle);
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DataOnlyFragment_UE::
    Add(
        FCk_Handle                           InHandle,
        const FCk_Fragment_DataOnlyFragment& InParams)
    -> void
{
}

auto
    UCk_Utils_DataOnlyFragment_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return false;
}

auto
    UCk_Utils_DataOnlyFragment_UE::
    Ensure(
        FCk_Handle InTimer)
    -> bool
{
    return false;
}
