// Extern "C" blocks need an alignment option somehow. I can do a "set NAMESPACE extern" in the cfg but that will probably screw other stuff up.

// See External\Audio\NativePluginDemo\NativeCode\TeleportLib.h for an example. Yeah it's in external (so have to force-format it) but it's a good case.

// (Actually it's in https://bitbucket.org/Unity-Technologies/nativeaudioplugins, but just published here to external)

extern "C"
{
	typedef EXPORT_API int (*Foo)(int arg);
};
