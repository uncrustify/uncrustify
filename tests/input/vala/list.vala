

using GLib;

public class GListTest : Object
{
 public GListTest { }

    static int main (string[] args) {
     List<string> list;
            list.append("TestString1");
  list.append("myTest");
        message ("list.length()=%d", list.length());

	for ( int i = 0; i < list.length(); i++) {
                    string list2 = list.nth_data(i);
                    message ("%s", list2);
                }
    }
}
