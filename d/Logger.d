/**
 * @file Logger.d
 * Logger and LogMask classes
 *
 * $Id: log.d 153 2006-04-02 00:51:25Z bengardner $
 */

module uncrustify.Logger;

import uncrustify.LogMask;

import std.cstream;
import std.string;
import std.stdio;
import std.format;
import std.date;


/* These are the 'standard' named log severities */
enum LogSev : byte
{
   Sys = 0,
   Error,
   Warn,
   Note,
   Info,
   Data,
}

class Logger
{
   char []   name = "Nobody";
   LogMask   mask;
   char [][] sev_names;

   /*TODO: consolidate into one 'show_flags' variable */
   bool      show_name = false;
   bool      show_sev = true;
   bool      show_date = false;
   bool      show_time = false;

   bool      m_in_log;

   byte      m_last_sev;
   char []   m_str;
   int       m_str_len;

   this ()
   {
      m_str.length = 512;
      m_str_len = 0;
      mask = new LogMask();
      mask.FromString("0-2");
      sev_names.length = 256;

      InitSevNames();
   }
   this (char [] log_name)
   {
      this();
      name = log_name;
   }

   bool ShouldLog(byte sev)
   {
      return(mask[sev]);
   }

   /**
    * Overridable function that processes the log.
    * The default just prints it to the console.
    */
   void ProcessLog(char [] str)
   {
      writef("%s", str);
   }

private:
   void Flush(bool force_nl = false)
   {
      if (m_str_len > 0)
      {
         if (force_nl && (m_str[m_str_len - 1] != '\n'))
         {
            m_str[m_str_len++] = '\n';
         }
         ProcessLog(m_str[0 .. m_str_len]);
         m_str_len = 0;
      }
   }


   /**
    * Starts the log statment by flushing if needed and printing the header
    *
    * @param sev  The log severity
    */
   void Start(byte sev)
   {
      if (sev != m_last_sev)
      {
         if (m_str_len > 0)
         {
            Flush(true);
         }
         m_last_sev = sev;
         m_in_log   = false;
      }

      if (!m_in_log)
      {
         /* Add the header, if enabled */
         if (show_name)
         {
            Append(name);
            Append(":");
         }
         if (show_sev)
         {
            Append(GetSevName(sev));
            Append(":");
         }

         /* TODO: come up with a better-looking format */
         if (show_date || show_time)
         {
            char [] utc = std.date.toUTCString(std.date.getUTCtime());

            if (show_date && show_time)
            {
               Append(utc[5..25]);
            }
            else if (show_date)
            {
               Append(utc[5..16]);
            }
            else
            {
               Append(utc[18..25]);
            }
            Append(":");
         }
      }
   }


   /**
    * Cleans up after a log statement by detecting whether the log is done,
    * (it ends in a newline) and possibly flushing the log.
    */
   void End()
   {
      if (m_str_len > 0)
      {
         m_in_log = (m_str[m_str_len - 1] != '\n');
      }
      if (!m_in_log || (m_str_len > (m_str.length / 2)))
      {
         Flush(false);
      }
   }

   void Append(char [] str, bool allow_flush = false)
   {
      foreach (char c; str)
      {
         m_str[m_str_len++] = c;
         if (m_str_len > (m_str.length - 16))
         {
            Flush(false);
         }
      }
   }

public:
   /* Sets the basic Severity Names */
   void InitSevNames()
   {
      SetSevName(LogSev.Sys,   "Sys ");
      SetSevName(LogSev.Error, "Err ");
      SetSevName(LogSev.Warn,  "Warn");
      SetSevName(LogSev.Note,  "Note");
      SetSevName(LogSev.Info,  "Info");
      SetSevName(LogSev.Data,  "Data");
   }

   void SetSevName(byte sev, char [] str)
   {
      if (sev < sev_names.length)
      {
         char [] s = new char [4];
         if (str.length >= 4)
         {
            s[] = str[0 .. 4];
         }
         else
         {
            s[0 .. str.length] = str;
            s[str.length .. 4] = ' ';
         }
         sev_names[sev] = s;
      }
   }

   char [] GetSevName(byte sev)
   {
      if (sev < sev_names.length)
      {
         if (sev_names[sev] !is null)
         {
            return sev_names[sev];
         }
      }
      return ToSevNumber(sev);
   }

   char [] ToSevNumber(byte sev)
   {
      char [] ch;
      ch.length = 4;

      ch[0] = 's';
      ch[1] = '0' + (sev / 100);
      ch[2] = '0' + ((sev / 10) % 10);
      ch[3] = '0' + (sev % 10);

      return(ch);
   }

   char [] ToHexByte(byte val)
   {
      char [] ch;

      ch.length = 2;
      ch[0] = std.string.hexdigits[val >> 4];
      ch[1] = std.string.hexdigits[val & 0x0f];
      return(ch);
   }

   char ToHexNibble(byte val)
   {
      return(std.string.hexdigits[val & 0x0f]);
   }

   void LogLine(byte sev, char [] str)
   {
      if (str.length > 0)
      {
         Log(sev, str);
         if (str[str.length - 1] != '\n')
         {
            Log(sev, "\n");
         }
      }
   }

   /**
    * Logs a string
    *
    * @param sev  The severity
    * @param str  The string
    */
   void Log(byte sev, char [] str)
   {
      if (!ShouldLog(sev) || (str is null) || (str.length <= 0))
      {
         return;
      }

      Start(sev);
      Append(str, true);
      End();
   }

   void LogHex(byte sev, byte [] buf, char [] sep = "")
   {
      if (!ShouldLog(sev) || (buf is null) || (buf.length <= 0))
      {
         return;
      }

      Start(sev);

      bool did_one = false;
      foreach (byte tmp; buf)
      {
         if (did_one)
         {
            Append(sep);
         }
         did_one = true;
         Append(ToHexByte(tmp));
      }

      End();
   }

   /**
    * Logs a block of data in a pretty hex format
    * Numbers on the left, characters on the right, just like I like it.
    *
    * @param sev     The severity
    * @param data    The data to log
    * @param len     The number of bytes to log
    */
   void LogHexBlock(byte sev, byte[] buf)
   {

      if (!ShouldLog(sev))
      {
         return;
      }

      int            idx;
      int            count;
      int            str_idx = 0;
      int            chr_idx = 0;
      byte           tmp;
      int            total;

      if ((buf is null) || (buf.length <= 0))
      {
         return;
      }

      /* Populate static chars */
      char [] m_hexstr;

      //"nnn | XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX | cccccccccccccccc\n";
      // 0   4 6                                                 ^ 56
      m_hexstr.length = 73;
      m_hexstr[3 .. 6]   = " | ";
      m_hexstr[53 .. 56] = " | ";
      m_hexstr[72] = '\n';
      for (idx = 8; idx < 51; idx += 3)
      {
         m_hexstr[idx] = ' ';
      }

      /*
       * Dump the specified number of bytes in hex, 16 byte per line by
       * creating a string and then calling log_str()
       */

      /* Loop through the data */
      count = 0;
      total = 0;
      foreach (byte tmp; buf)
      {
         if (count == 0)
         {
            str_idx = 6;
            chr_idx = 56;

            m_hexstr[0] = ToHexNibble(total >> 12);
            m_hexstr[1] = ToHexNibble(total >> 8);
            m_hexstr[2] = ToHexNibble(total >> 4);
         }

         m_hexstr[str_idx]     = ToHexNibble(tmp >> 4);
         m_hexstr[str_idx + 1] = ToHexNibble(tmp);
         str_idx         += 3;

         m_hexstr[chr_idx++] = ((tmp >= 32) && (tmp <= 126)) ? cast(char)tmp : '.';

         total++;
         count++;
         if (count >= 16)
         {
            count = 0;
            Log(sev, m_hexstr);
         }
      }

      /*
      ** Print partial line if any
      */
      if (count != 0)
      {
         /* Clear out any junk */
         while (count < 16)
         {
            m_hexstr[str_idx]     = ' ';   /* MSB hex */
            m_hexstr[str_idx + 1] = ' ';   /* LSB hex */
            str_idx         += 3;

            m_hexstr[chr_idx++] = ' ';

            count++;
         }
         Log(sev, m_hexstr);
      }
   }

   private void add_char(dchar ch)
   {
      m_str[m_str_len++] = ch;
      if (m_str_len > (m_str.length - 16))
      {
         Flush(false);
      }
   }

   /**
    * Logs a string, with formatting options
    *
    * @param sev  The severity
    * @param str  The format string
    * @param p    The format parameters
    */
   void LogFmt(byte sev, ...)
   {
      if (!ShouldLog(sev))
      {
         return;
      }

      LogFmt(sev, _arguments, _argptr);
   }

   void LogFmt(byte sev, TypeInfo [] args, void *argptr)
   {
      Start(sev);
      std.format.doFormat(&add_char, args, argptr);
      End();
   }
}

