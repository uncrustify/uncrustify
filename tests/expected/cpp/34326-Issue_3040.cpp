void main()
{
  switch (opcode ) {

    case LocaleCompare:
    {
      return Number(localeCompare(s, a0.toString(exec)));
    }

#ifndef KJS_PURE_ECMA
    case Big:
    {
      result = String("<big>" + s + "</big>");
      break;
    }
#endif
  }

  switch (ev->command)
  {
    case (MIDI_NOTEON):
    {
      ev->note = *ptrdata; ptrdata++; currentpos++;
      ev->vel  = *ptrdata; ptrdata++; currentpos++;
      if (ev->vel==0)
	note[ev->chn][ev->note]=FALSE;
      else
	note[ev->chn][ev->note]=TRUE;

#ifdef TRACKDEBUG2
      if (ev->chn==6) {
	if (ev->vel==0) printfdebug("Note Onf\n");
	else printfdebug("Note On\n");
      };
#endif
      break;
    }

    case (MIDI_NOTEOFF):
    {
#ifdef TRACKDEBUG2
      if (ev->chn==6) printfdebug("Note Off\n");
#endif
      ev->note = *ptrdata; ptrdata++; currentpos++;
      ev->vel  = *ptrdata; ptrdata++; currentpos++;
      note[ev->chn][ev->note]=FALSE;

      break;
    }

    case (MIDI_KEY_PRESSURE):
    {
#ifdef TRACKDEBUG2
      if (ev->chn==6) printfdebug ("Key press\n");
#endif
      ev->note = *ptrdata; ptrdata++; currentpos++;
      ev->vel  = *ptrdata; ptrdata++; currentpos++;
      break;
    }
  }
}

