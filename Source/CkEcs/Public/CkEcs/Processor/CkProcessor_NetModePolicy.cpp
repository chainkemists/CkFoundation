#include "CkProcessor_NetModePolicy.h"

#include "CkEcs/Net/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        ShouldCreateProcessorForNetMode(
            const ECk_ProcessorNetModeRequirement InRequirement,
            const FCk_Handle& InTransientEntity)
        -> bool
    {
        switch (InRequirement)
        {
            case ECk_ProcessorNetModeRequirement::All:
            {
                return true;
            }
            case ECk_ProcessorNetModeRequirement::CosmeticOnly:
            {
                return UCk_Utils_Net_UE::Get_CanExecuteCosmeticEvents(InTransientEntity);
            }
            case ECk_ProcessorNetModeRequirement::AuthorityOnly:
            {
                // NOT Get_HasAuthority: the transient entity is locally owned in every world, so
                // its authority tag is present on pure clients too. The Host net-mode tag is the
                // world-level signal the enum documents.
                return UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InTransientEntity);
            }
            case ECk_ProcessorNetModeRequirement::ClientOnly:
            {
                return UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(InTransientEntity)
                    && NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InTransientEntity);
            }
            default:
            {
                return true;
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
