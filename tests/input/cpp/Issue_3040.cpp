int main()
{
  switch (opcode)
  {
    case 1:
      return Number(localeCompare(s, a0.toString(exec)));

#ifndef A
    case 2:
      result = String("<big>" + s + "</big>");
      break;
#endif

  }

  switch (ev->command)
  {
    case (3):
      ev->note = *ptrdata; ptrdata++; currentpos++;
      ev->vel  = *ptrdata; ptrdata++; currentpos++;
      if (ev->vel==0)
	note[ev->chn][ev->note]=FALSE;
      else
	note[ev->chn][ev->note]=TRUE;

#ifdef B
      if (ev->chn==6) {
	if (ev->vel==0) printfdebug("Note Onf\n");
	else printfdebug("Note On\n");
      };
#endif
      break;
    case (4) :
#ifdef C
      if (ev->chn==6) printfdebug("Note Off\n");
#endif
      ev->note = *ptrdata;ptrdata++;currentpos++; 
      ev->vel  = *ptrdata;ptrdata++;currentpos++;
      note[ev->chn][ev->note]=FALSE;

      break;
    case (5) :
#ifdef D
      if (ev->chn==6) printfdebug ("Key press\n");
#endif
      ev->note = *ptrdata;ptrdata++;currentpos++; 
      ev->vel  = *ptrdata;ptrdata++;currentpos++;
      break;

#ifndef E
    case 6:
      result = String("<big>" + s + "</big>");
      break;
#endif
		}
}

