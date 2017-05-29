#if UNITY_GFX_EXTERNAL_SELECT_RENDERING_API

    #if PLATFORM_ANDROID || PLATFORM_TIZEN // should it be ENABLE_EGL?
    extern "C" int UnityGetSelectedGLESVersion();
    #elif UNITY_APPLE_PVR
    extern "C" int UnityGetSelectedRenderingAPI();
    #endif
#endif
