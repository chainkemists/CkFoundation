#pragma once

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkLabel/CkLabel_Utils.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkNet/CkNet_Utils.h"

#include "CkEcsBasics_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------



UCLASS(NotBlueprintable)
class CKECSBASICS_API UCk_Utils_Ecs_Base_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Ecs_Base_UE);

private:
    template <typename T_ParamType>
    struct MetaFragment_Params
    {
        TArray<T_ParamType> _Params;

        auto Add(const T_ParamType& InParam) -> void;
        auto Pop() -> TOptional<T_ParamType>;

        template <typename T_Pred>
        auto Pop(T_Pred InFunc) -> TOptional<T_ParamType>;
    };

protected:
    template <typename T_FragmentUtils, typename T_RecordUtils>
    static auto
    Get_EntityOrRecordEntry_WithFragmentAndLabel(
        FCk_Handle InHandle,
        FGameplayTag InLabelName) -> FCk_Handle;

public:
    /*
     *
     * Parameter storage is required for 'Meta Fragments' [1] so that the Construction Scripts (CS) on the client
     * can retrieve the Parameters when constructing the Meta Fragment. See the MeterAttribute for an example.
     *
     * An Entity may have several Meta Fragments (e.g. several MeterAttributes) and thus we need a way to
     * store all the incoming Parameters (from the parent Entity's CS) for use later in our Meta Fragment's CS.
     *
     * Why not pass the parameters directly to the CS? Because CS is a network-stable object and any runtime
     * modifications to its Parameters will not be reflected on the Clients when the Entity is replicated.
     * Moreover, runtime modifications will affect the 'default' instance of the CS which can affect any
     * subsequent code that is relying on the default instance of the CS asset to have default values.
     *
     * [1] Meta Fragment is a Fragment that is actually an Entity with a Construction Script and has its
     * own Fragments. The BFL of the Meta Fragment makes this fact transparent for multiple reasons:
     * - keep future refactors easy e.g. MeterAttribute uses FloatAttribute internally. For reasons such
     *   as performance, it may simply use 3 floats and stop being a Meta Fragment. If that is the case,
     *   none of the client code should break
     * - the notion of a Meta Fragment is not something we want to expose to clients as a first-class
     *   concept of the library
     */

    template <typename T_ParamType>
    static auto
    Store_Parameter(
        FCk_Handle InHandle,
        const T_ParamType& InParameter) -> void;

    template <typename T_ParamType>
    static auto
    Pop_Parameter(
        FCk_Handle InHandle) -> TOptional<T_ParamType>;

    template <typename T_ParamType, typename T_Predicate>
    static auto
    Pop_Parameter(
        FCk_Handle InHandle, T_Predicate InFunc) -> TOptional<T_ParamType>;

    template <typename T_ParamType>
    static auto
    Has_Parameter(
        FCk_Handle InHandle) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_ParamType>
auto
    UCk_Utils_Ecs_Base_UE::MetaFragment_Params<T_ParamType>::
    Add(
        const T_ParamType& InParam) -> void
{
    _Params.Emplace(InParam);
}

template <typename T_ParamType>
auto
    UCk_Utils_Ecs_Base_UE::MetaFragment_Params<T_ParamType>::
    Pop()
    -> TOptional<T_ParamType>
{
    if (_Params.IsEmpty())
    { return {}; }

    const auto Ret = _Params[0];
    _Params.RemoveAt(0);
    return Ret;
}

template <typename T_ParamType>
template <typename T_Pred>
auto
    UCk_Utils_Ecs_Base_UE::
    MetaFragment_Params<T_ParamType>::
    Pop(
        T_Pred InFunc)
    -> TOptional<T_ParamType>
{
    auto Index = _Params.IndexOfByPredicate(InFunc);

    if (Index == INDEX_NONE)
    { return {}; }

    const auto Ret = _Params[Index];
    _Params.RemoveAt(Index);
    return Ret;
}

// --------------------------------------------------------------------------------------------------------------------

template <typename T_FragmentUtils, typename T_RecordUtils>
auto
    UCk_Utils_Ecs_Base_UE::
    Get_EntityOrRecordEntry_WithFragmentAndLabel(
        FCk_Handle InHandle,
        FGameplayTag InLabelName)
    -> FCk_Handle
{
    const auto& Pred = [&InLabelName](FCk_Handle InEntity)
    {
        return T_FragmentUtils::Has(InEntity)
            && UCk_Utils_GameplayLabel_UE::Has(InEntity)
            && UCk_Utils_GameplayLabel_UE::MatchesExact(InEntity, InLabelName);
    };

    if (Pred(InHandle))
    { return InHandle; }

    if (NOT T_RecordUtils::Has(InHandle))
    { return {}; }

    return T_RecordUtils::Get_RecordEntryIf(InHandle, Pred);
}

template <typename T_ParamType>
auto
    UCk_Utils_Ecs_Base_UE::
    Store_Parameter(
        FCk_Handle InHandle,
        const T_ParamType& InParameter)
    -> void
{
    InHandle.AddOrGet<MetaFragment_Params<T_ParamType>>().Add(InParameter);
}

template <typename T_ParamType>
auto
    UCk_Utils_Ecs_Base_UE::
    Pop_Parameter(
        FCk_Handle InHandle)
    -> TOptional<T_ParamType>
{
    if (NOT Has_Parameter<T_ParamType>(InHandle))
    { return {}; }

    return InHandle.Get<MetaFragment_Params<T_ParamType>>().Pop();
}

template <typename T_ParamType, typename T_Predicate>
auto
    UCk_Utils_Ecs_Base_UE::
    Pop_Parameter(
        FCk_Handle InHandle,
        T_Predicate InFunc)
    -> TOptional<T_ParamType>
{
    if (NOT Has_Parameter<T_ParamType>(InHandle))
    { return {}; }

    return InHandle.Get<MetaFragment_Params<T_ParamType>>().Pop(InFunc);
}

template <typename T_ParamType>
auto
    UCk_Utils_Ecs_Base_UE::
    Has_Parameter(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<MetaFragment_Params<T_ParamType>>();
}

// --------------------------------------------------------------------------------------------------------------------
