#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Ticker/CkTicker.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKECS_API FEcsWorld : public FTicker
    {
        CK_GENERATED_BODY(FEcsWorld);

    private:
        RegistryType _Registry;

    public:
        CK_PROPERTY_GET(_Registry);
        CK_PROPERTY_GET_NON_CONST(_Registry);

        CK_DEFINE_CONSTRUCTORS(FEcsWorld, _Registry);

    public:
        template <typename T_Tickable, typename... T_Args>
        auto Add(T_Args&&... InArgs) -> HandleType;
    };

    // ----

    template <typename T_Tickable, typename... T_Args>
    auto
        FEcsWorld::
        Add(
            T_Args&&... InArgs)
        -> HandleType
    {
        if constexpr (requires { T_Tickable::NetModeRequirement; })
        {
            const auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(_Registry);
            if (NOT ShouldCreateProcessorForNetMode(T_Tickable::NetModeRequirement, TransientEntity))
            { return {}; }
        }

        return FTicker::Add<T_Tickable>(std::forward<T_Args>(InArgs)...);
    }
}

// --------------------------------------------------------------------------------------------------------------------