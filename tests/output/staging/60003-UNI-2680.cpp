if (Application.platform == RuntimePlatform.LinuxEditor)
{
    return new ProcessStartInfo("smthg")
        {
            Arguments = string.Format ("-9 --ss -S aa \"{0}\"", file),
            WorkingDirectory = Directory.GetCurrentDirectory(),
            UseShellExecute = false,
            CreateNoWindow = true
        };
}
