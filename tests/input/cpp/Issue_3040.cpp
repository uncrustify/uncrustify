void main()
{
  switch (opcode)
  {
    case LocaleCompare:
      return Number(localeCompare(s, a0.toString(exec)));

#ifndef KJS_PURE_ECMA
    case Big:
      result = String("<big>" + s + "</big>");
      break;
#endif
  }
}

