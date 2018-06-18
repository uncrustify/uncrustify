private void Foo()
{
    Action<UnityPlayerBaseStartInfo, GraphicsTestRunConfiguration, Action<string, Bitmap, long>, RenderingBackend, DX11FeatureLevel? , string> playerRunnerImageCallback
        = (playerStartInfo, description, incomingScreenshotCallback, configuration, dx11Featurelevel, graphicsDriverType) => incomingScreenshotCallback(filename, new Bitmap(1, 1), 42);
}
