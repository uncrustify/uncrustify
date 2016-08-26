// Why are we getting doubling indentation on multiline new[] initializer?

// See GetAdditionalReferences

// Note that in C++ it seems to work ok

namespace Namespace
{
    public class Class
    {
        private string[] GetAdditionalReferences()
        {
            return new []
                {
                    "System.dll",
                };
        }
    }
}
