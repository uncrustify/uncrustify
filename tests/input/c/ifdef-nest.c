
namespace MonoTests.System.IO.IsolatedStorageTest {
   public class NonAbstractIsolatedStorage : IsolatedStorage {
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

