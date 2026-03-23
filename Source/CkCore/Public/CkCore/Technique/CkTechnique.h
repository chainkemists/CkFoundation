#pragma once

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    /**
     * Return type for technique steps. Controls whether the pipeline continues or stops.
     */
    enum class EStepResult : uint8
    {
        Continue,
        Abort
    };

    // ----

    namespace detail
    {
        template<typename T, typename = void>
        struct HasShouldAbort : std::false_type {};

        template<typename T>
        struct HasShouldAbort<T, std::void_t<decltype(std::declval<const T&>().ShouldAbort())>> : std::true_type {};
    }

    // ----

    /**
     * CRTP form: derive from Technique, add steps, call ProcessAllSteps().
     * Steps receive (DerivedType&, T_Params&&...) and return EStepResult.
     *
     * If the derived type defines `auto ShouldAbort() const -> bool`, the pipeline
     * will also check it between steps and stop early if it returns true.
     *
     * Usage:
     *   struct FMyTechnique : ck::Technique<FMyTechnique, FContext&>
     *   {
     *       FMyTechnique() { AddStep(&FMyTechnique::Validate); }
     *       static auto Validate(FMyTechnique& Self, FContext& Ctx) -> EStepResult;
     *   };
     *
     * @tparam T_Derived CRTP derived type (must be a non-reference class)
     * @tparam T_Params  Parameter types forwarded to each step
     */
    template<typename T_First = void, typename... T_Rest>
    class Technique
    {
    public:
        using DerivedType = T_First;
        using StepType    = TFunction<EStepResult(DerivedType&, T_Rest&&...)>;

    public:
        auto AddStep(StepType InStep) -> DerivedType&
        {
            _Steps.Emplace(MoveTemp(InStep));
            return *This();
        }

        template<typename... T_Args>
        auto ProcessAllSteps(T_Args&&... InParams) -> void
        {
            static_assert(
                (std::is_same_v<std::decay_t<T_Args>, std::decay_t<T_Rest>> && ...) ||
                (std::is_assignable_v<T_Args, T_Rest> && ...) ||
                (std::is_convertible_v<T_Args, T_Rest> && ...),
                "T_Args and T_Params must be compatible");

            for (const auto& Step : _Steps)
            {
                if constexpr (detail::HasShouldAbort<DerivedType>::value)
                {
                    if (This()->ShouldAbort())
                    { break; }
                }

                if (Step(*This(), std::forward<T_Rest>(InParams)...) == EStepResult::Abort)
                { break; }
            }
        }

        [[nodiscard]] auto IsEmpty() const -> bool { return _Steps.IsEmpty(); }
        [[nodiscard]] auto Num() const -> int32 { return _Steps.Num(); }
        auto Reserve(int32 InCount) -> void { _Steps.Reserve(InCount); }
        auto Reset() -> void { _Steps.Reset(); }

    private:
        auto This() -> DerivedType* { return static_cast<DerivedType*>(this); }
        auto This() const -> const DerivedType* { return static_cast<const DerivedType*>(this); }

    private:
        TArray<StepType> _Steps;
    };

    // ----

    /**
     * Pipeline form (non-CRTP): steps are simple callables with no self reference.
     * Selected automatically when the first template argument is a reference type.
     * Steps return EStepResult to control flow.
     *
     * Usage:
     *   auto Steps = ck::Technique<FContext&>{};
     *   Steps.AddStep([](FContext& Ctx) -> ck::EStepResult { ... });
     *   Steps.ProcessAllSteps(Context);
     *
     * Or with zero parameters (steps capture everything via [&]):
     *   auto Steps = ck::Technique<>{};
     *   Steps.AddStep([&]() -> ck::EStepResult { ... });
     *   Steps.ProcessAllSteps();
     *
     * @tparam T_First  First parameter type (reference)
     * @tparam T_Rest   Additional parameter types forwarded to each step
     */
    template<typename T_First, typename... T_Rest>
    class Technique<T_First&, T_Rest...>
    {
    public:
        using StepType = TFunction<EStepResult(T_First&, T_Rest&&...)>;

    public:
        auto AddStep(StepType InStep) -> Technique&
        {
            _Steps.Emplace(MoveTemp(InStep));
            return *this;
        }

        template<typename... T_Args>
        auto ProcessAllSteps(T_Args&&... InParams) -> void
        {
            static_assert(
                sizeof...(T_Args) == sizeof...(T_Rest) + 1,
                "Argument count must match parameter count");

            for (const auto& Step : _Steps)
            {
                if (Step(std::forward<T_Args>(InParams)...) == EStepResult::Abort)
                { break; }
            }
        }

        [[nodiscard]] auto IsEmpty() const -> bool { return _Steps.IsEmpty(); }
        [[nodiscard]] auto Num() const -> int32 { return _Steps.Num(); }
        auto Reserve(int32 InCount) -> void { _Steps.Reserve(InCount); }
        auto Reset() -> void { _Steps.Reset(); }

    private:
        TArray<StepType> _Steps;
    };

    // ----

    /**
     * Zero-parameter pipeline form: steps capture everything via [&].
     * Steps return EStepResult to control flow.
     * Selected when Technique<> is instantiated with default void argument.
     *
     * Usage:
     *   auto Steps = ck::Technique<>{};
     *   Steps.AddStep([&]() -> ck::EStepResult { ... });
     *   Steps.ProcessAllSteps();
     */
    template<>
    class Technique<void>
    {
    public:
        using StepType = TFunction<EStepResult()>;

    public:
        auto AddStep(StepType InStep) -> Technique&
        {
            _Steps.Emplace(MoveTemp(InStep));
            return *this;
        }

        auto ProcessAllSteps() -> void
        {
            for (const auto& Step : _Steps)
            {
                if (Step() == EStepResult::Abort)
                { break; }
            }
        }

        [[nodiscard]] auto IsEmpty() const -> bool { return _Steps.IsEmpty(); }
        [[nodiscard]] auto Num() const -> int32 { return _Steps.Num(); }
        auto Reserve(int32 InCount) -> void { _Steps.Reserve(InCount); }
        auto Reset() -> void { _Steps.Reset(); }

    private:
        TArray<StepType> _Steps;
    };
}

// --------------------------------------------------------------------------------------------------------------------
