#pragma once

#include "CkCore/Entt/Entt.h"
#include "CkCore/Time/CkTime.h"

namespace ck::concepts
{
    struct FTickable_Concept : entt::type_list<void(FCk_Time), void()>
    {
        template <typename Base>
        struct type : Base
        {
            auto Tick(FCk_Time InDeltaTime)
            {
                entt::poly_call<0>(*this, InDeltaTime);
            }

            auto Pump()
            {
                entt::poly_call<1>(*this);
            }
        };

        template <typename Type>
        using impl = entt::value_list<&Type::Tick, &Type::Pump>;
    };

    using FTickableType = entt::poly<FTickable_Concept>;
}