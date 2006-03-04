
namespace MonoTests.System.IO.IsolatedStorageTest {

#if NET_2_0
#if __MonoCS__
        public class NonAbstractIsolatedStorage : IsolatedStorage {
#else
        public class NonAbstractIsolatedStorage : global::System.IO.IsolatedStorage.IsolatedStorage {
#endif
#else
        public class NonAbstractIsolatedStorage : IsolatedStorage {
#endif
                public NonAbstractIsolatedStorage () {
                   string s = String.Format ("{0} {1}",
                                             Environment.NewLine,
 #if NET_2_0
                                             String.Empty);
 #else
                                             "            ");
 #endif
                }

        }
}

