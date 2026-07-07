#pragma once

#include "CkCore/Entt/Entt.h"
#include "CkCore/Time/CkTime.h"

namespace ck::concepts
{
    struct FTickable_Concept : entt::type_list<void(FCk_Time), int32()>
    {
        template <typename Base>
        struct type : Base
        {
            auto Tick(FCk_Time InDeltaTime)
            {
                entt::poly_call<0>(*this, InDeltaTime);
            }

            // Returns the number of entities the pump visited so the scheduler can tell a no-op
            // pump (0 — provably produced no new work, must not force another pump pass) from a
            // productive one (>0) or an unknown one (-1 — custom DoTick bodies that don't report
            // a count; treated conservatively as having done work).
            auto Pump() -> int32
            {
                return entt::poly_call<1>(*this);
            }
        };

        template <typename Type>
        using impl = entt::value_list<&Type::Tick, &Type::Pump>;
    };

    using FTickableType = entt::poly<FTickable_Concept>;
}