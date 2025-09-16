namespace utils_entity_collection
{
    FCk_Handle_EntityCollection Request_AddSingleEntity(FCk_Handle_EntityCollection InEntityCollection, FCk_Handle InEntityToAdd)
    {
        auto EntitiesToAdd = TArray<FCk_Handle>();
        EntitiesToAdd.Add(InEntityToAdd);

        utils_entity_collection::Request_AddEntities(InEntityCollection, FCk_Request_EntityCollection_AddEntities(EntitiesToAdd));

        return InEntityCollection;
    }

    FCk_Handle_EntityCollection Request_RemoveSingleEntity(FCk_Handle_EntityCollection InEntityCollection, FCk_Handle InEntityToRemove)
    {
        auto EntitiesToRemove = TArray<FCk_Handle>();
        EntitiesToRemove.Add(InEntityToRemove);

        utils_entity_collection::Request_RemoveEntities(InEntityCollection, FCk_Request_EntityCollection_RemoveEntities(EntitiesToRemove));

        return InEntityCollection;
    }

    FCk_Handle_EntityCollection Request_ClearEntities(FCk_Handle_EntityCollection InEntityCollection)
    {
        auto EntitiesToRemove = InEntityCollection.Get_EntitiesInCollection().Get_EntitiesInCollection();

        if (EntitiesToRemove.IsEmpty() == false)
        {
            utils_entity_collection::Request_RemoveEntities(InEntityCollection, FCk_Request_EntityCollection_RemoveEntities(EntitiesToRemove));
        }

        return InEntityCollection;
    }
}