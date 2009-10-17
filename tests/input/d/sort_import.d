// should be ddd, eee, fff
import ddd;
import fff;
import eee;

// should be aaa, ccc
import ccc;
import aaa;
// should be just bbb
import bbb;

// should not change these, as it can't handle multi-line imports
private import mango.ccc;
private import mango.bbb,
               mango.aaa;

void foo();


