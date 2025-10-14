namespace utils_prefab
{
    FCk_Handle
    Create_ProbeNode(
        FCk_Handle_Transform InOwner,
        FCk_AnyShape InShape,
        FCk_Fragment_Probe_ParamsData InProbeParams,
        FTransform InLocalTransform = FTransform(),
        FCk_Probe_DebugInfo InDebugInfo = FCk_Probe_DebugInfo())
    {
        auto SceneNode = utils_scene_node::Create(InOwner, InLocalTransform);
        auto Entity = SceneNode.H();

        utils_shapes::Add(Entity, InShape);

        auto TransformHandle = Entity.To_FCk_Handle_Transform();
        utils_probe::Add(TransformHandle, InProbeParams, InDebugInfo);

        return Entity;
    }
}