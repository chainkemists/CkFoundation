#include "CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

ck::FProcessor::
    FProcessor(
        const RegistryType& InRegistry)
    : _Registry(InRegistry)
{
    _TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(_Registry);
}

// --------------------------------------------------------------------------------------------------------------------
