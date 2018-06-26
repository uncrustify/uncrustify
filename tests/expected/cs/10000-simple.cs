class X : Y {
        bool Method (int argument_1, int argument_2)
        {
                #region something
                int foo = 0;
                #endregion

                if (argument_1 == argument_2)
                        throw new Exception (Locale.GetText ("They are equal!"));

                if (argument_1 < argument_2) {
                        if (argument_1 * 3 > 4)
                                return true;
                        else
                                return false;
                }

//
// This sample helps keep your sanity while using 8-spaces for tabs
//
                VeryLongIdentifierWhichTakesManyArguments (
                        Argument1,
                        Argument2, Argument3,
                        NestedCallHere (
                                MoreNested));
        }

        bool MyProperty {
                get { return x; }

                set { x = value; }
        }

        void AnotherMethod ()
        {
                Logger log = new Logger ();

                log.foo.bar = 5;
                log.narf.sweat = "cat";

                if ((a + 5) != 4) {
                }

                while (blah) {
                        if (a)
                                continue;
                        b++;
                }
        }
}

object lockA;
object lockB;

void Foo ()
{
        lock (lockA) {
                lock (lockB) {
                }
        }
}

void Bar ()
{
        lock (lockB) {
                lock (lockA) {
                }
        }
}


// class library
class Blah {
        Hashtable ht;
        void Foo (int zzz, Entry blah)
        {
                lock (ht) {
                        ht.Add (zzz, blah);
                }
        }

        void Bar ()
        {
                lock (ht) {
                        foreach (Entry e in ht)
                                EachBar (e);
                }
        }

        virtual void EachBar (Entry e)
        {
        }
}

// User
class MyBlah {
        byte[] box = new byte [6];

        box [2] = 56;

        void DoStuff ()
        {
                lock (this) {
                        int i = GetNumber ();
                        Entry e = GetEntry ();

                        Foo (i, e);
                }
        }

        override void EachBar (Entry e)
        {
                lock (this) {
                        DoSomething (e);
                }
        }
}

