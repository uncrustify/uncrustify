// should be ddd, eee, fff
import ddd;
import eee;
import fff;

// should be aaa, ccc
import aaa;
import ccc;
// should be just bbb
import bbb;

// should not change these, as it can't handle multi-line imports
private import mango.ccc;
private import mango.bbb,
               mango.aaa;

void foo();


