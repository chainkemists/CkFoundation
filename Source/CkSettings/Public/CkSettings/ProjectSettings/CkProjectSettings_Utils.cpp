#include "CkProjectSettings_Utils.h"

#include <Engine/RendererSettings.h>
#include <HAL/IConsoleManager.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProjectSettings_UE::
    Get_RendererSettings_CustomDepthValue()
    -> ECk_RendererSettings_CustomDepth
{
    const auto* consoleVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.CustomDepth"));
    const auto& customDepth = consoleVar->GetInt();

    switch (static_cast<ECustomDepthStencil::Type>(customDepth))
    {
        case ECustomDepthStencil::Disabled:
        {
            return ECk_RendererSettings_CustomDepth::Disabled;
        }
        case ECustomDepthStencil::Enabled:
        {
            return ECk_RendererSettings_CustomDepth::Enabled;
        }
        case ECustomDepthStencil::EnabledOnDemand:
        {
            return ECk_RendererSettings_CustomDepth::EnabledOnDemand;
        }
        case ECustomDepthStencil::EnabledWithStencil:
        {
            return ECk_RendererSettings_CustomDepth::EnabledWithStencil;
        }
        default:
        {
            return ECk_RendererSettings_CustomDepth::Disabled;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
