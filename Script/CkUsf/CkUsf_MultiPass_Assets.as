namespace CkUsf
{
    // Two-pass feedback effect: BufferA reads its own previous frame (diffuse + decay + emitter),
    // Image colorizes BufferA into the display render target. Proves render-to-texture / multi-pass.
    asset Smoke of UCkUsf_MultiPassDefinition
    {
        _Resolution = 512;

        FCkUsf_PassDef BufferAPass;
        BufferAPass._Id = ECk_Usf_PassId::BufferA;
        BufferAPass._Look = SmokeBuffer;
        FCkUsf_BufferChannelBinding SelfFeedback;
        SelfFeedback._ChannelIndex = 0;
        SelfFeedback._Buffer = ECk_Usf_PassId::BufferA;
        BufferAPass._BufferChannels.Add(SelfFeedback);
        _Passes.Add(BufferAPass);

        FCkUsf_PassDef ImagePass;
        ImagePass._Id = ECk_Usf_PassId::Image;
        ImagePass._Look = SmokeImage;
        FCkUsf_BufferChannelBinding ReadBufferA;
        ReadBufferA._ChannelIndex = 0;
        ReadBufferA._Buffer = ECk_Usf_PassId::BufferA;
        ImagePass._BufferChannels.Add(ReadBufferA);
        _Passes.Add(ImagePass);
    }
}
