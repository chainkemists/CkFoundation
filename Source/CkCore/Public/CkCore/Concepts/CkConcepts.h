#pragma once

#include "CkCore/Entt/Entt.h"
#include "CkCore/Time/CkTime.h"

namespace ck::concepts
{
    struct FTickable_Concept : entt::type_list<void(FCk_Time)>
    {
        template <typename Base>
        struct type : Base
        {
            auto Tick(FCk_Time InDeltaTime)
            {
                entt::poly_call<0>(*this, InDeltaTime);
            }
        };

        template <typename Type>
        using impl = entt::value_list<&Type::Tick>;
    };

    using FTickableType = entt::poly<FTickable_Concept>;
}