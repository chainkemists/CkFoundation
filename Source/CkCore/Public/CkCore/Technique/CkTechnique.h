#pragma once

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    /**
     * A composable step-based execution pipeline.
     * Derive from Technique, add steps via AddStep(), then call ProcessAllSteps() to run them in order.
     *
     * @tparam T_Derived CRTP derived type
     * @tparam T_Params  Parameter types forwarded to each step
     */
    template<typename T_Derived, typename... T_Params>
    class Technique
    {
    public:
        using DerivedType = T_Derived;
        using ThisType    = Technique<DerivedType, T_Params...>;
        using StepType    = TFunction<void(DerivedType&, T_Params&&...)>;

    public:
        auto AddStep(StepType InStep) -> DerivedType&
        {
            _Steps.Emplace(MoveTemp(InStep));
            return *This();
        }

		template <typename... T_Args>
		void ProcessAllSteps(T_Args&&... params) const
		{
			const_cast<DerivedType*>(This())->ProcessAllSteps(std::forward<T_Params>(params)...);
		}

        template<typename... T_Args>
        auto ProcessAllSteps(T_Args&&... InParams) -> void
        {
            static_assert(
                (std::is_same_v<std::decay_t<T_Args>, std::decay_t<T_Params>> && ...) ||
                (std::is_assignable_v<T_Args, T_Params> && ...) ||
                (std::is_convertible_v<T_Args, T_Params> && ...),
                "T_Args and T_Params must be compatible");

            for (const auto& Step : _Steps)
            {
                // Cannot use T_Args directly due to MSVC v19.28 bug:
                // https://developercommunity.visualstudio.com/t/MSVC-fails-to-compile-correct-example-of/1359579
                Step(*This(), std::forward<T_Params>(InParams)...);
            }
        }

        [[nodiscard]] auto IsEmpty() const -> bool
        {
            return _Steps.IsEmpty();
        }

        [[nodiscard]] auto Num() const -> int32
        {
            return _Steps.Num();
        }

        auto Reserve(int32 InCount) -> void
        {
            _Steps.Reserve(InCount);
        }

        auto Reset() -> void
        {
            _Steps.Reset();
        }

    private:
        auto This() -> DerivedType*
        {
            return static_cast<DerivedType*>(this);
        }

    private:
        TArray<StepType> _Steps;
    };
}

// --------------------------------------------------------------------------------------------------------------------
