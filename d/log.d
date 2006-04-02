/**
 * @file log.d
 * Global logging utilities
 *
 * $Id$
 */

module uncrustify.log;

import uncrustify.Logger;
import uncrustify.LogSev;


/**
 * Global functions:
 *   LogInit();
 *   LogSetMask(LogMask);
 *   Log(byte, ...);
 *   LogHex(byte, byte []);
 *   LogHexBlock(byte, byte []);
 */

public Logger StaticLog;

void LogInit()
{
   StaticLog = new Logger();
}

void LogSetMask(LogMask mask)
{
   StaticLog.mask = mask;
}
void LogSetMask(char [] str)
{
   StaticLog.mask.FromString(str);
}

void Log(byte sev, char [] str)
{
   if (StaticLog.ShouldLog(sev))
   {
      StaticLog.Log(sev, str);
   }
}

void Log(byte sev, ...)
{
   if (StaticLog.ShouldLog(sev))
   {
      StaticLog.LogFmt(sev, _arguments, _argptr);
   }
}

void LogHex(byte sev, byte [] buf, char [] sep = "")
{
   if (StaticLog.ShouldLog(sev))
   {
      StaticLog.LogHex(sev, buf, sep);
   }
}

void LogHexBlock(byte sev, byte [] buf)
{
   if (StaticLog.ShouldLog(sev))
   {
      StaticLog.LogHexBlock(sev, buf);
   }
}

