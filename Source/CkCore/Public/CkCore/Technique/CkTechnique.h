#pragma once

#include <CoreMinimal.h>

#include "CkTechnique.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Technique_CompletionReason : uint8
{
    Completed,
    Aborted
};

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
     * Optional callbacks:
     *   - OnAborted(...):  fires when a step returns Abort or ShouldAbort() returns true
     *   - OnFinished(Reason, ...): fires once after the step loop exits, always last
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
        using DerivedType    = T_First;
        using StepType       = TFunction<EStepResult(DerivedType&, T_Rest&&...)>;
        using OnAbortedType  = TFunction<void(DerivedType&, T_Rest&&...)>;
        using OnFinishedType = TFunction<void(ECk_Technique_CompletionReason, DerivedType&, T_Rest&&...)>;

    public:
        auto AddStep(StepType InStep) -> DerivedType&
        {
            _Steps.Emplace(MoveTemp(InStep));
            return *This();
        }

        auto OnAborted(OnAbortedType InCallback) -> DerivedType&
        {
            _OnAborted = MoveTemp(InCallback);
            return *This();
        }

        auto OnFinished(OnFinishedType InCallback) -> DerivedType&
        {
            _OnFinished = MoveTemp(InCallback);
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

            auto Reason = ECk_Technique_CompletionReason::Completed;

            for (const auto& Step : _Steps)
            {
                if constexpr (detail::HasShouldAbort<DerivedType>::value)
                {
                    if (This()->ShouldAbort())
                    {
                        Reason = ECk_Technique_CompletionReason::Aborted;
                        break;
                    }
                }

                if (Step(*This(), std::forward<T_Rest>(InParams)...) == EStepResult::Abort)
                {
                    Reason = ECk_Technique_CompletionReason::Aborted;
                    break;
                }
            }

            if (Reason == ECk_Technique_CompletionReason::Aborted && _OnAborted)
            { _OnAborted(*This(), std::forward<T_Rest>(InParams)...); }

            if (_OnFinished)
            { _OnFinished(Reason, *This(), std::forward<T_Rest>(InParams)...); }
        }

        [[nodiscard]] auto IsEmpty() const -> bool { return _Steps.IsEmpty(); }
        [[nodiscard]] auto Num() const -> int32 { return _Steps.Num(); }
        auto Reserve(int32 InCount) -> void { _Steps.Reserve(InCount); }
        auto Reset() -> void { _Steps.Reset(); _OnAborted = {}; _OnFinished = {}; }

    private:
        auto This() -> DerivedType* { return static_cast<DerivedType*>(this); }
        auto This() const -> const DerivedType* { return static_cast<const DerivedType*>(this); }

    private:
        TArray<StepType> _Steps;
        OnAbortedType    _OnAborted;
        OnFinishedType   _OnFinished;
    };

    // ----

    /**
     * Pipeline form (non-CRTP): steps are simple callables with no self reference.
     * Selected automatically when the first template argument is a reference type.
     * Steps return EStepResult to control flow.
     *
     * Optional callbacks:
     *   - OnAborted(T_First&, T_Rest&&...): fires on abort
     *   - OnFinished(Reason, T_First&, T_Rest&&...): fires last, always
     *
     * Usage:
     *   auto Steps = ck::Technique<FContext&>{};
     *   Steps.AddStep([](FContext& Ctx) -> ck::EStepResult { ... });
     *   Steps.ProcessAllSteps(Context);
     *
     * @tparam T_First  First parameter type (reference)
     * @tparam T_Rest   Additional parameter types forwarded to each step
     */
    template<typename T_First, typename... T_Rest>
    class Technique<T_First&, T_Rest...>
    {
    public:
        using StepType       = TFunction<EStepResult(T_First&, T_Rest&&...)>;
        using OnAbortedType  = TFunction<void(T_First&, T_Rest&&...)>;
        using OnFinishedType = TFunction<void(ECk_Technique_CompletionReason, T_First&, T_Rest&&...)>;

    public:
        auto AddStep(StepType InStep) -> Technique&
        {
            _Steps.Emplace(MoveTemp(InStep));
            return *this;
        }

        auto OnAborted(OnAbortedType InCallback) -> Technique&
        {
            _OnAborted = MoveTemp(InCallback);
            return *this;
        }

        auto OnFinished(OnFinishedType InCallback) -> Technique&
        {
            _OnFinished = MoveTemp(InCallback);
            return *this;
        }

        template<typename... T_Args>
        auto ProcessAllSteps(T_Args&&... InParams) -> void
        {
            static_assert(
                sizeof...(T_Args) == sizeof...(T_Rest) + 1,
                "Argument count must match parameter count");

            auto Reason = ECk_Technique_CompletionReason::Completed;

            for (const auto& Step : _Steps)
            {
                if (Step(std::forward<T_Args>(InParams)...) == EStepResult::Abort)
                {
                    Reason = ECk_Technique_CompletionReason::Aborted;
                    break;
                }
            }

            if (Reason == ECk_Technique_CompletionReason::Aborted && _OnAborted)
            { _OnAborted(std::forward<T_Args>(InParams)...); }

            if (_OnFinished)
            { _OnFinished(Reason, std::forward<T_Args>(InParams)...); }
        }

        [[nodiscard]] auto IsEmpty() const -> bool { return _Steps.IsEmpty(); }
        [[nodiscard]] auto Num() const -> int32 { return _Steps.Num(); }
        auto Reserve(int32 InCount) -> void { _Steps.Reserve(InCount); }
        auto Reset() -> void { _Steps.Reset(); _OnAborted = {}; _OnFinished = {}; }

    private:
        TArray<StepType> _Steps;
        OnAbortedType    _OnAborted;
        OnFinishedType   _OnFinished;
    };

    // ----

    /**
     * Zero-parameter pipeline form: steps capture everything via [&].
     * Steps return EStepResult to control flow.
     * Selected when Technique<> is instantiated with default void argument.
     *
     * Optional callbacks:
     *   - OnAborted(): fires on abort
     *   - OnFinished(Reason): fires last, always
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
        using StepType       = TFunction<EStepResult()>;
        using OnAbortedType  = TFunction<void()>;
        using OnFinishedType = TFunction<void(ECk_Technique_CompletionReason)>;

    public:
        auto AddStep(StepType InStep) -> Technique&
        {
            _Steps.Emplace(MoveTemp(InStep));
            return *this;
        }

        auto OnAborted(OnAbortedType InCallback) -> Technique&
        {
            _OnAborted = MoveTemp(InCallback);
            return *this;
        }

        auto OnFinished(OnFinishedType InCallback) -> Technique&
        {
            _OnFinished = MoveTemp(InCallback);
            return *this;
        }

        auto ProcessAllSteps() -> void
        {
            auto Reason = ECk_Technique_CompletionReason::Completed;

            for (const auto& Step : _Steps)
            {
                if (Step() == EStepResult::Abort)
                {
                    Reason = ECk_Technique_CompletionReason::Aborted;
                    break;
                }
            }

            if (Reason == ECk_Technique_CompletionReason::Aborted && _OnAborted)
            { _OnAborted(); }

            if (_OnFinished)
            { _OnFinished(Reason); }
        }

        [[nodiscard]] auto IsEmpty() const -> bool { return _Steps.IsEmpty(); }
        [[nodiscard]] auto Num() const -> int32 { return _Steps.Num(); }
        auto Reserve(int32 InCount) -> void { _Steps.Reserve(InCount); }
        auto Reset() -> void { _Steps.Reset(); _OnAborted = {}; _OnFinished = {}; }

    private:
        TArray<StepType> _Steps;
        OnAbortedType    _OnAborted;
        OnFinishedType   _OnFinished;
    };
}

// --------------------------------------------------------------------------------------------------------------------
