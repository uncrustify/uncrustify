/* Advanced Vala Sample Code */
using GLib;
public class Sample : Object {
   public string name { get; set; }
      public signal void foo ();
     public Sample (construct string! name) {
  }
public void run () {
          foo += s => {
                      stdout.printf ("Lambda expression %s!\n", name);
  };

         /* Calling lambda expression */
            foo ();
    }
    static int main (string[] args) {
           foreach (string arg in args) {
                       var sample = new Sample (arg);
                      sample.run ();
                     /* Object will automatically be freed
                      * at the end of the block */
       }
      return 0;
    }
}

