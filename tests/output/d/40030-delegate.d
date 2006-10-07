module er;

void delegate(ubyte[] a) TSender;
bool delegate(ushort a)  TVerifier;
typedef ushort          TAddr;

public void delegate(ubyte[] a) TSender;
public bool delegate(ushort a)  TVerifier;
public typedef ushort   TAddr;

void delegate() dg;
dg = { int y; };

int opApply(int delegate(inout Type[, ...]) dg);
