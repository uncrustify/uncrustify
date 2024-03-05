/**
 * @file option.h
 * Enumerations and data types for options.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @author  Matthew Woehlke since version 0.67
 * @license GPL v2+
 */
#ifndef OPTION_H_INCLUDED
#define OPTION_H_INCLUDED

/* NOTE:
 * This file is processed by make_option_enum.py, which parses any 'enum class'
 * it finds, as well as the special macros UNC_OPTVAL_ALIAS and UNC_OPTVALS.
 *
 * The '// <PREFIX>' comment after an 'enum class' tells the script to generate
 * aliases for the enum values using the prefix that is given in the '<>'s.
 * Don't remove or alter these.
 */

#include "enum_flags.h"

#include <string>
#include <vector>

#include <cassert>

#ifdef IGNORE // WinBase.h
#undef IGNORE
#endif

namespace uncrustify
{

template<typename T> class Option;

//-----------------------------------------------------------------------------
// Option types
enum class option_type_e // <OT>
{
   // UNC_CONVERT_INTERNAL
   BOOL,
   IARF,
   LINEEND,
   TOKENPOS,
   NUM,
   UNUM,
   STRING,
};

#if 0 // Fake enumeration for make_option_enum.py
enum class bool
{
   true,
   false,
};
#endif

//-----------------------------------------------------------------------------
/// I/A/R/F values - these are bit fields
enum class iarf_e // <IARF>
{
   IGNORE = 0,                   //! option ignores a given feature
   ADD    = (1u << 0),           //! option adds a given feature
   REMOVE = (1u << 1),           //! option removes a given feature
   FORCE  = (ADD | REMOVE),      //! option forces the usage of a given feature
};

UNC_DECLARE_FLAGS(iarf_flags_t, iarf_e);
UNC_DECLARE_OPERATORS_FOR_FLAGS(iarf_flags_t);

//-----------------------------------------------------------------------------
/// Line endings
enum class line_end_e // <LE>
{
   LF,   //! "\n"   typically used on Unix/Linux system
   CRLF, //! "\r\n" typically used on Windows systems
   CR,   //! "\r"   carriage return without newline
   AUTO, //! keep last
};
constexpr auto line_end_styles = static_cast<size_t>(line_end_e::AUTO);

//-----------------------------------------------------------------------------
/// Token position - these are bit fields
enum class token_pos_e // <TP>
{
   IGNORE      = 0,               //! don't change it
   BREAK       = 1,               //! add a newline before or after the if not present
   FORCE       = 2,               //! force a newline on one side and not the other
   LEAD        = 4,               //! at the start of a line or leading if wrapped line
   TRAIL       = 8,               //! at the end of a line or trailing if wrapped line
   JOIN        = 16,              //! remove newlines on both sides
   LEAD_BREAK  = (LEAD | BREAK),  //  5
   LEAD_FORCE  = (LEAD | FORCE),  //  6
   TRAIL_BREAK = (TRAIL | BREAK), //  9
   TRAIL_FORCE = (TRAIL | FORCE), // 10
};

UNC_DECLARE_FLAGS(token_pos_flags_t, token_pos_e);
UNC_DECLARE_OPERATORS_FOR_FLAGS(token_pos_flags_t);

//-----------------------------------------------------------------------------
/// Abstract (untyped) interface for options
class GenericOption
{
public:
   GenericOption(const char *opt_name, const char *opt_desc)
      : m_name{opt_name}
      , m_desc{opt_desc}
   {}

   virtual ~GenericOption() = default;

   virtual option_type_e type() const = 0;
   const char *name() const { return(m_name); }
   const char *description() const { return(m_desc); }
   virtual const char *const *possibleValues() const = 0;

   virtual std::string defaultStr() const = 0;
   virtual std::string minStr() const { return(std::string{}); }
   virtual std::string maxStr() const { return(std::string{}); }

   virtual bool isDefault() const = 0;

   virtual void reset() = 0;
   virtual bool read(const char *s) = 0;
   virtual std::string str() const = 0;

protected:
   template<typename V> friend bool read_enum(const char *s, Option<V> &o);
   template<typename V> friend bool read_number(const char *s, Option<V> &o);

   void warnUnexpectedValue(const char *actual) const;
   void warnIncompatibleReference(const GenericOption *ref) const;

   const char *const m_name;
   const char *const m_desc;
};

//-----------------------------------------------------------------------------
// Helper class for reporting problems with options
class OptionWarning
{
public:
   enum class /* UNC_NO_META */ Severity
   {
      OS_CRITICAL,
      OS_MINOR,
   };

   constexpr static auto CRITICAL = Severity::OS_CRITICAL;
   constexpr static auto MINOR    = Severity::OS_MINOR;

   OptionWarning(const char *filename, Severity = CRITICAL);
   OptionWarning(const GenericOption *, Severity = CRITICAL);
   OptionWarning(const OptionWarning &) = delete;
   ~OptionWarning();

#ifdef __GNUC__
   [[gnu::format(printf, 2, 3)]]
#endif
   void operator()(const char *fmt, ...);
};

//-----------------------------------------------------------------------------
// Concrete (strongly typed) interface for options
template<typename T>
class Option : public GenericOption
{
public:
   Option(const char *opt_name, const char *opt_desc, T opt_val = T{})
      : GenericOption{opt_name, opt_desc}
      , m_val{opt_val}
      , m_default{opt_val}
   {}

   option_type_e type() const override;
   const char *const *possibleValues() const override;

   std::string defaultStr() const override;

   bool isDefault() const override { return(m_val == m_default); }

   //! resets option to its default value
   //- currently only used by the emscripten interface
   virtual void reset() override;

   bool read(const char *s) override;
   std::string str() const override;

   T operator()() const { return(m_val); }
   Option &operator=(T val) { m_val = val; return(*this); }

protected:
   template<typename V> friend bool read_enum(const char *s, Option<V> &o);
   template<typename V> friend bool read_number(const char *s, Option<V> &o);

   virtual bool validate(long) { return(true); }

   T m_val     = T{};
   T m_default = T{};
};

//-----------------------------------------------------------------------------
// Concrete (strongly typed) interface for bounded numeric options
template<typename T, T min, T max>
class BoundedOption : public Option<T>
{
public:
   BoundedOption(const char *opt_name, const char *opt_desc, T opt_val = T{})
      : Option<T>{opt_name, opt_desc, opt_val}
   {
      assert(  opt_val >= min
            && opt_val <= max);
   }

   std::string minStr() const override { return(std::to_string(min)); }
   std::string maxStr() const override { return(std::to_string(max)); }

protected:
   bool validate(long val) override
   {
      if (val < static_cast<long>(min))
      {
         OptionWarning w{ this };
         w("requested value %ld for option '%s' "
           "is less than the minimum value %ld",
           val, this->name(), static_cast<long>(min));
         return(false);
      }

      if (val > static_cast<long>(max))
      {
         OptionWarning w{ this };
         w("requested value %ld for option '%s' "
           "is greater than the maximum value %ld",
           val, this->name(), static_cast<long>(max));
         return(false);
      }
      return(true);
   }
};

///////////////////////////////////////////////////////////////////////////////

// Declaration of option types; implementations are in option.cpp
#define UNC_IMPLEMENT_OPTION(T)                                     \
   template<> option_type_e Option<T>::type() const;                \
   template<> const char *const *Option<T>::possibleValues() const; \
   template<> bool Option<T>::read(const char *s);                  \
   extern template class Option<T>

UNC_IMPLEMENT_OPTION(bool);
UNC_IMPLEMENT_OPTION(iarf_e);
UNC_IMPLEMENT_OPTION(line_end_e);
UNC_IMPLEMENT_OPTION(token_pos_e);
UNC_IMPLEMENT_OPTION(signed);
UNC_IMPLEMENT_OPTION(unsigned);
UNC_IMPLEMENT_OPTION(std::string);

// Additional mappings for option values
#define UNC_OPTVAL_ALIAS(...) \
   static_assert(true, "This is just a tag for make_option_enum.py")

UNC_OPTVAL_ALIAS(bool, false, "0", "f", "n", "no");
UNC_OPTVAL_ALIAS(bool, true, "1", "t", "y", "yes");
UNC_OPTVAL_ALIAS(iarf_e, IGNORE, "i");
UNC_OPTVAL_ALIAS(iarf_e, ADD, "a", "2", "t", "true", "y", "yes");
UNC_OPTVAL_ALIAS(iarf_e, REMOVE, "r", "0", "f", "false", "n", "no");
UNC_OPTVAL_ALIAS(iarf_e, FORCE, "f", "1");

// Possible values for options, by type
#define UNC_OPTVALS(e)    extern const char *const e ## _values[]
UNC_OPTVALS(iarf);
UNC_OPTVALS(line_end);
UNC_OPTVALS(token_pos);

extern bool convert_string(const char *, bool &);
extern bool convert_string(const char *, iarf_e &);
extern bool convert_string(const char *, line_end_e &);
extern bool convert_string(const char *, token_pos_e &);

extern const char *to_string(bool);
extern const char *to_string(iarf_e);
extern const char *to_string(line_end_e);
extern const char *to_string(token_pos_e);
extern const char *to_string(option_type_e);

struct OptionGroup
{
   const char                   *description;
   std::vector<GenericOption *> options;
};


/**
 * @brief Defines a new group of uncrustify options.
 *
 * New options are always added to the most recently defined group.
 */
void begin_option_group(const char *description);


/**
 * @brief Adds an uncrustify option to the global option registry.
 *
 * The option is added to the most recently defined option group.
 */
void register_option(GenericOption *);


GenericOption *find_option(const char *name);


//! Add all uncrustify options to the global option registry
void register_options();


OptionGroup *get_option_group(size_t);


size_t get_option_count();


/**
 * processes a single line string to extract configuration settings
 * increments cpd.line_number and cpd.error_count
 *
 * @param config_line  single line string that will be processed
 * @param filename     for log messages, file from which the \p config_line
 *                     param was extracted
 * @param compat_level version of Uncrustify with which to be compatible
 */
void process_option_line(const std::string &config_line, const char *filename, int &compat_level);


bool load_option_file(const char *filename, int compat_level = 0);


/**
 * save the used options into a text file
 *
 * @param pfile     file to print into
 * @param with_doc  also print description
 * @param minimal   print only options with non default value
 */
void save_option_file(FILE *pfile, bool with_doc = false, bool minimal = false);


/**
 * get the marker that was selected for the end of line via the config file
 *
 * @return "\n"     if newlines was set to LE_LF in the config file
 * @return "\r\n"   if newlines was set to LE_CRLF in the config file
 * @return "\r"     if newlines was set to LE_CR in the config file
 * @return "\n"     if newlines was set to LE_AUTO in the config file
 */
const char *get_eol_marker();

} // namespace uncrustify

#endif /* OPTION_H_INCLUDED */
