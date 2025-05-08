#include "CkWorld_Utils.h"

#include "CkCore/Format/CkFormat.h"

// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Utils_World_UE::
    TryGet_GameWorld(
        const UObject* InObject)
    -> TOptional<const UWorld*>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Object is [{}]"), InObject)
    { return {}; }

    const auto SuperWorld = TryGet_WorldFromOuterRecursively(InObject->GetOuter());
    if (ck::IsValid(SuperWorld))
    {
        return SuperWorld;
    }

    return {};
}

auto
    UCk_Utils_World_UE::
    TryGet_WorldFromOuterRecursively(
        const UObject* InOuter)
    -> TOptional<const UWorld*>
{
    if (ck::Is_NOT_Valid(InOuter))
    { return {}; }

    if (ck::IsValid(Cast<UWorld>(InOuter)))
    {
        return Cast<UWorld>(InOuter);
    }

    return TryGet_WorldFromOuterRecursively(InOuter->GetOuter());
}

auto
    UCk_Utils_World_UE::
    TryGet_FirstValidWorld(
        const UObject* InObject,
        std::function<bool(const UWorld*)> InPredicate)
    -> TOptional<const UWorld*>
{
    const auto& MaybeWorld = TryGet_GameWorld(InObject);
    if (ck::IsValid(MaybeWorld))
    { return MaybeWorld; }

    // The following will get the first valid world regardless of the _type_
    const auto& World = [&]() -> UWorld*
    {
        if (ck::Is_NOT_Valid(GEngine))
        { return {}; }

        for (auto& Context : GEngine->GetWorldContexts())
        {
            const auto& ContextWorld = Context.World();

            if (ck::Is_NOT_Valid(ContextWorld))
            { continue; }

            if (const auto& GameInstance = ContextWorld->GetGameInstance(); ck::IsValid(GameInstance))
            {
                const auto GameInstanceWorld = GameInstance->GetWorld();

                if (ck::Is_NOT_Valid(GameInstanceWorld))
                { continue; }

                if (InPredicate(GameInstanceWorld))
                { return GameInstanceWorld; }

                return GameInstance->GetWorld();
            }
        }

        return {};
    }();

    return World;
}

auto
    UCk_Utils_World_UE::
    TryGet_MutableGameWorld(
        const UObject* InObject)
    -> TOptional<UWorld*>
{
    const auto& MaybeWorld = TryGet_GameWorld(InObject);

    if (ck::Is_NOT_Valid(MaybeWorld))
    { return {}; }

    auto MutableWorld = const_cast<UWorld*>(*MaybeWorld);
    return MutableWorld;
}

auto
    UCk_Utils_World_UE::
    TryGet_MutableWorldFromOuterRecursively(
        const UObject* InOuter)
    -> TOptional<UWorld*>
{
    const auto& MaybeWorld = TryGet_WorldFromOuterRecursively(InOuter);

    if (ck::Is_NOT_Valid(MaybeWorld))
    { return {}; }

    auto MutableWorld = const_cast<UWorld*>(*MaybeWorld);
    return MutableWorld;
}

auto
    UCk_Utils_World_UE::
    TryGet_MutableFirstValidWorld(
        const UObject* InObject,
        std::function<bool(UWorld*)> InPredicate)
    -> TOptional<UWorld*>
{
    const auto& MaybeWorld = TryGet_FirstValidWorld(InObject, [&](const UWorld* InWorld)
    {
        auto MutableWorld = const_cast<UWorld*>(InWorld);
        return InPredicate(MutableWorld);
    });

    if (ck::Is_NOT_Valid(MaybeWorld))
    { return {}; }

    auto MutableWorld = const_cast<UWorld*>(*MaybeWorld);
    return MutableWorld;
}

auto
    UCk_Utils_World_UE::
    Get_AllAvailableGameWorlds()
    -> TArray<UWorld*>
{
    auto Worlds = TArray<UWorld*>{};

    if (ck::Is_NOT_Valid(GEngine))
    { return {}; }

    for (auto& Context : GEngine->GetWorldContexts())
    {
        const auto& ContextWorld = Context.World();

        if (ck::Is_NOT_Valid(ContextWorld))
        { continue; }

        auto GameInstance = ContextWorld->GetGameInstance();

        if (ck::Is_NOT_Valid(GameInstance))
        { continue; }

        Worlds.Add(ContextWorld);
    }

    return Worlds;
}
