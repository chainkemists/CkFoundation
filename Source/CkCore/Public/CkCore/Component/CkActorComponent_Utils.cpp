#include "CkActorComponent_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorComponent_UE::
    Get_AllComponentsInHierarchy(
        const AActor* InActor)
    -> TArray<UActorComponent*>
{
    auto OutComponents = TArray<UActorComponent*>{};

    if (ck::Is_NOT_Valid(InActor))
    { return OutComponents; }

    auto Root = InActor->GetRootComponent();
    if (ck::Is_NOT_Valid(Root))
    { return OutComponents; }

    auto Stack = TArray<USceneComponent*>{};
    Stack.Add(Root);

    while (Stack.Num() > 0)
    {
        const auto Current = Stack.Pop();

        CK_ENSURE_IF_NOT(ck::IsValid(Current),
            TEXT("Current component [{}] was expected to be valid during hierarchical traversal of Actor [{}]"),
            InActor)
        { continue; }

        OutComponents.Add(Current);

        TArray<USceneComponent*> Children;

        Current->GetChildrenComponents(true, Children);

        for (auto* Child : Children)
        {
            if (ck::IsValid(Child))
            {
                Stack.Add(Child);
            }
        }
    }

    return OutComponents;
}

auto
    UCk_Utils_ActorComponent_UE::
    Get_AllowTickOnDedicatedServer(
        const UActorComponent* InActorComponent)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActorComponent), TEXT("Invalid Component supplied to Get_AllowTickOnDedicatedServer"))
    { return {}; }

    return InActorComponent->PrimaryComponentTick.bAllowTickOnDedicatedServer;
}

auto
    UCk_Utils_ActorComponent_UE::
    Set_AllowTickOnDedicatedServer(
        UActorComponent* InActorComponent,
        bool InServerTickEnabled)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActorComponent), TEXT("Invalid Component supplied to Set_AllowTickOnDedicatedServer"))
    { return; }

    InActorComponent->PrimaryComponentTick.bAllowTickOnDedicatedServer = InServerTickEnabled;
    InActorComponent->PrimaryComponentTick.UnRegisterTickFunction();
    InActorComponent->PrimaryComponentTick.RegisterTickFunction(InActorComponent->GetComponentLevel());
}

// --------------------------------------------------------------------------------------------------------------------
