// This is accepted

extern "C" NSBundle*            UnityGetMetalBundle()       {
    return _MetalBundle;
}
extern "C" MTLDeviceRef         UnityGetMetalDevice()       { return _MetalDevice; }
extern "C" MTLCommandQueueRef   UnityGetMetalCommandQueue() { return ((UnityDisplaySurfaceMTL*)GetMainDisplaySurface())->commandQueue; }

extern "C" EAGLContext*         UnityGetDataContextEAGL()   {
    return _GlesContext;
}

// But if I want to have them on the same line, it fails

extern "C" NSBundle*            UnityGetMetalBundle()       { return _MetalBundle; }
extern "C" MTLDeviceRef         UnityGetMetalDevice()       { return _MetalDevice; }
extern "C" MTLCommandQueueRef   UnityGetMetalCommandQueue() { return ((UnityDisplaySurfaceMTL*)GetMainDisplaySurface())->commandQueue; }

extern "C" EAGLContext*         UnityGetDataContextEAGL()   { return _GlesContext; }
