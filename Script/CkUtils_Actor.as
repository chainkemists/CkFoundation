enum ECk_ActorSearchMethod
{
    SearchByName,
    SearchByTag
};

namespace utils_actor
{
    TArray<AActor> Get_AllActorsOfClass(TSubclassOf<AActor> InActorClass)
    {
        auto OutActors = TArray<AActor>();
        Gameplay::GetAllActorsOfClass(InActorClass, OutActors);
        return OutActors;
    }

    TArray<AActor> Get_AllActorsWithNameContaining(FString InNameContains, ECk_ActorSearchMethod InSearchMethod = ECk_ActorSearchMethod::SearchByTag)
    {
        auto AllActors = Get_AllActorsOfClass(AActor);
        auto MatchingActors = TArray<AActor>();

        for (auto Actor : AllActors)
        {
            if (!ck::IsValid(Actor))
            {
                continue;
            }

            bool Matches = false;

            if (InSearchMethod == ECk_ActorSearchMethod::SearchByName)
            {
                Matches = Actor.GetFullName().Contains(InNameContains);
            }
            else if (InSearchMethod == ECk_ActorSearchMethod::SearchByTag)
            {
                auto TagName = FName(InNameContains);
                Matches = Actor.Tags.Contains(TagName);
            }

            if (!Matches)
            {
                continue;
            }

            MatchingActors.Add(Actor);
        }

        return MatchingActors;
    }

    TArray<AActor> Get_AllActorsWithName(FString InExactName, ECk_ActorSearchMethod InSearchMethod = ECk_ActorSearchMethod::SearchByTag)
    {
        auto AllActors = Get_AllActorsOfClass(AActor);
        auto MatchingActors = TArray<AActor>();

        for (auto Actor : AllActors)
        {
            if (!ck::IsValid(Actor))
            {
                continue;
            }

            bool Matches = false;

            if (InSearchMethod == ECk_ActorSearchMethod::SearchByName)
            {
                Matches = Actor.GetName() == InExactName;
            }
            else if (InSearchMethod == ECk_ActorSearchMethod::SearchByTag)
            {
                auto TagName = FName(InExactName);
                Matches = Actor.Tags.Contains(TagName);
            }

            if (!Matches)
            {
                continue;
            }

            MatchingActors.Add(Actor);
        }

        return MatchingActors;
    }

    AActor Get_FirstActorWithNameContaining(FString InNameContains, ECk_ActorSearchMethod InSearchMethod = ECk_ActorSearchMethod::SearchByTag)
    {
        auto AllActors = Get_AllActorsOfClass(AActor);

        for (auto Actor : AllActors)
        {
            if (!ck::IsValid(Actor))
            {
                continue;
            }

            bool Matches = false;

            if (InSearchMethod == ECk_ActorSearchMethod::SearchByName)
            {
                Matches = Actor.GetFullName().Contains(InNameContains);
            }
            else if (InSearchMethod == ECk_ActorSearchMethod::SearchByTag)
            {
                auto TagName = FName(InNameContains);
                Matches = Actor.Tags.Contains(TagName);
            }

            if (!Matches)
            {
                continue;
            }

            return Actor;
        }

        return nullptr;
    }

    AActor Get_FirstActorWithName(FString InExactName, ECk_ActorSearchMethod InSearchMethod = ECk_ActorSearchMethod::SearchByTag)
    {
        auto AllActors = Get_AllActorsOfClass(AActor);

        for (auto Actor : AllActors)
        {
            if (!ck::IsValid(Actor))
            {
                continue;
            }

            bool Matches = false;

            if (InSearchMethod == ECk_ActorSearchMethod::SearchByName)
            {
                Matches = Actor.GetName() == InExactName;
            }
            else if (InSearchMethod == ECk_ActorSearchMethod::SearchByTag)
            {
                auto TagName = FName(InExactName);
                Matches = Actor.Tags.Contains(TagName);
            }

            if (!Matches)
            {
                continue;
            }

            return Actor;
        }

        return nullptr;
    }

    TArray<AActor> Get_AllActorsOfClassWithNameContaining(TSubclassOf<AActor> InActorClass, FString InNameContains, ECk_ActorSearchMethod InSearchMethod = ECk_ActorSearchMethod::SearchByTag)
    {
        auto AllActors = Get_AllActorsOfClass(InActorClass);
        auto MatchingActors = TArray<AActor>();

        for (auto Actor : AllActors)
        {
            if (!ck::IsValid(Actor))
            {
                continue;
            }

            bool Matches = false;

            if (InSearchMethod == ECk_ActorSearchMethod::SearchByName)
            {
                Matches = Actor.GetFullName().Contains(InNameContains);
            }
            else if (InSearchMethod == ECk_ActorSearchMethod::SearchByTag)
            {
                auto TagName = FName(InNameContains);
                Matches = Actor.Tags.Contains(TagName);
            }

            if (!Matches)
            {
                continue;
            }

            MatchingActors.Add(Actor);
        }

        return MatchingActors;
    }

    AActor Get_FirstActorOfClassWithNameContaining(TSubclassOf<AActor> InActorClass, FString InNameContains, ECk_ActorSearchMethod InSearchMethod = ECk_ActorSearchMethod::SearchByTag)
    {
        auto AllActors = Get_AllActorsOfClass(InActorClass);

        for (auto Actor : AllActors)
        {
            if (!ck::IsValid(Actor))
            {
                continue;
            }

            bool Matches = false;

            if (InSearchMethod == ECk_ActorSearchMethod::SearchByName)
            {
                Matches = Actor.GetFullName().Contains(InNameContains);
            }
            else if (InSearchMethod == ECk_ActorSearchMethod::SearchByTag)
            {
                auto TagName = FName(InNameContains);
                Matches = Actor.Tags.Contains(TagName);
            }

            if (!Matches)
            {
                continue;
            }

            return Actor;
        }

        return nullptr;
    }

    TArray<AActor> Get_AllActorsWithinDistance(FVector InLocation, float InMaxDistance, TSubclassOf<AActor> InActorClass = AActor)
    {
        auto AllActors = Get_AllActorsOfClass(InActorClass);
        auto NearbyActors = TArray<AActor>();
        auto MaxDistanceSquared = InMaxDistance * InMaxDistance;

        for (auto Actor : AllActors)
        {
            if (!ck::IsValid(Actor))
            {
                continue;
            }

            auto DistanceSquared = Actor.GetActorLocation().DistSquared(InLocation);
            if (DistanceSquared > MaxDistanceSquared)
            {
                continue;
            }

            NearbyActors.Add(Actor);
        }

        return NearbyActors;
    }

    AActor Get_ClosestActorToLocation(FVector InLocation, TSubclassOf<AActor> InActorClass = AActor)
    {
        auto AllActors = Get_AllActorsOfClass(InActorClass);
        auto ClosestActor = Cast<AActor>(nullptr);
        auto ClosestDistanceSquared = Math::Square(999999.0);

        for (auto Actor : AllActors)
        {
            if (!ck::IsValid(Actor))
            {
                continue;
            }

            auto DistanceSquared = Actor.GetActorLocation().DistSquared(InLocation);
            if (DistanceSquared >= ClosestDistanceSquared)
            {
                continue;
            }

            ClosestDistanceSquared = DistanceSquared;
            ClosestActor = Actor;
        }

        return ClosestActor;
    }
}