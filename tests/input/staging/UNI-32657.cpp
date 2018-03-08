void UNITY_INTERFACE_API XREnvironment::DepthSetNumberOfPointsImpl(
    IUnityXRDepthDataAllocator* allocator,
    size_t numPoints)
{
}

UnityXRRaycastHit* (UNITY_INTERFACE_API* Raycast_SetNumberOfHits)(
        IUnityXRRaycastAllocator* allocator,
        size_t numHits);
		