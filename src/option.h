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

#include "enum_flags.h"

#include <vector>
#include <string>

#include <cassert>

#ifdef IGNORE // WinBase.h
#undef IGNORE
#endif

namespace uncrustify
{

//-----------------------------------------------------------------------------
// Option types
enum class option_type_e
{
   BOOL,
   IARF,
   LINEEND,
   TOKENPOS,
   NUM,
   UNUM,
   STRING,
};

//-----------------------------------------------------------------------------
/// I/A/R/F values - these are bit fields
enum class iarf_e
{
   IGNORE      = 0,              //! option ignores a given feature
   ADD         = (1u << 0),      //! option adds a given feature
   REMOVE      = (1u << 1),      //! option removes a given feature
   FORCE       = (ADD | REMOVE), //! option forces the usage of a given feature
   NOT_DEFINED = (1u << 2)       //! for debugging
};

UNC_DECLARE_FLAGS(iarf_flags_t, iarf_e);
UNC_DECLARE_OPERATORS_FOR_FLAGS(iarf_flags_t);

//-----------------------------------------------------------------------------
/// Line endings
enum class lineend_e
{
   LF,   //! "\n"   typically used on Unix/Linux system
   CRLF, //! "\r\n" typically used on Windows systems
   CR,   //! "\r"   carriage return without newline
   AUTO, //! keep last
};
constexpr auto lineend_styles = static_cast<size_t>(lineend_e::AUTO);

//-----------------------------------------------------------------------------
/// Token position - these are bit fields
enum class tokenpos_e
{
   IGNORE      = 0,  //! don't change it
   BREAK       = 1,  //! add a newline before or after the if not present
   FORCE       = 2,  //! force a newline on one side and not the other
   LEAD        = 4,  //! at the start of a line or leading if wrapped line
   TRAIL       = 8,  //! at the end of a line or trailing if wrapped line
   JOIN        = 16, //! remove newlines on both sides
   LEAD_BREAK  = (LEAD | BREAK),
   LEAD_FORCE  = (LEAD | FORCE),
   TRAIL_BREAK = (TRAIL | BREAK),
   TRAIL_FORCE = (TRAIL | FORCE),
};

UNC_DECLARE_FLAGS(tokenpos_flags_t, tokenpos_e);
UNC_DECLARE_OPERATORS_FOR_FLAGS(tokenpos_flags_t);

//-----------------------------------------------------------------------------
/// Abstract (untyped) interface for options
class GenericOption
{
public:
   GenericOption(const char *name, const char *desc)
      : m_name{name}
      , m_desc{desc} {}

   virtual ~GenericOption() = default;

   virtual option_type_e type() const = 0;
   const char *name() const { return(m_name); }
   const char *description() const { return(m_desc); }
   virtual const char *const *possibleValues() const = 0;
   virtual std::string defaultStr() const = 0;

   virtual bool read(const char *s) = 0;
   virtual std::string str() const = 0;

protected:
   void warnUnexpectedValue(const char *actual) const;

   const char *const m_name;
   const char *const m_desc;
};

//-----------------------------------------------------------------------------
// Concrete (strongly typed) interface for options
template<typename T>
class Option : public GenericOption
{
public:
   Option(const char *name, const char *desc, T val = T{})
      : GenericOption{name, desc}
      , m_val{val}
      , m_default{val} {}

   option_type_e type() const override;
   const char *const *possibleValues() const override;
   std::string defaultStr() const override;

   bool read(const char *s) override;
   std::string str() const override;

   T operator()() const { return(m_val); }
   Option &operator=(T val) { m_val = val; return(*this); }

protected:
   virtual bool validate(T val) { return(true); }

   T m_val     = T{};
   T m_default = T{};
};

//-----------------------------------------------------------------------------
// Concrete (strongly typed) interface for bounded numeric options
template<typename T, T min, T max>
class BoundedOption : public Option<T>
{
public:
   BoundedOption(const char *name, const char *desc, T val = T{})
      : Option<T>{name, desc, val}
   {
      assert(val >= min && val <= max);
   }

protected:
   bool validate(T val) override
   {
      // FIXME report errors
      return(val >= min && val <= max);
   }
};

///////////////////////////////////////////////////////////////////////////////

// Declaration of option types; implementations are in option.cpp
#define UNC_IMPLEMENT_OPTION(T)                                     \
   template<> option_type_e Option<T>::type() const;                \
   template<> const char *const *Option<T>::possibleValues() const; \
   template<> std::string Option<T>::defaultStr() const;            \
   template<> bool Option<T>::read(const char *s);                  \
   template<> std::string Option<T>::str() const

UNC_IMPLEMENT_OPTION(bool);
UNC_IMPLEMENT_OPTION(iarf_e);
UNC_IMPLEMENT_OPTION(lineend_e);
UNC_IMPLEMENT_OPTION(tokenpos_e);
UNC_IMPLEMENT_OPTION(signed);
UNC_IMPLEMENT_OPTION(unsigned);
UNC_IMPLEMENT_OPTION(std::string);

// Additional mappings for option values
#define UNC_OPTVAL_ALIAS(...) \
   static_assert(true, "This is just a tag for make_option_enum.py")

UNC_OPTVAL_ALIAS(bool, false, "0", "f", "n", "no");
UNC_OPTVAL_ALIAS(bool, true, "1", "t", "y", "yes");
UNC_OPTVAL_ALIAS(iarf_e, IGNORE, "i");
UNC_OPTVAL_ALIAS(iarf_e, ADD, "a");
UNC_OPTVAL_ALIAS(iarf_e, REMOVE, "r");
UNC_OPTVAL_ALIAS(iarf_e, FORCE, "f");

} // namespace uncrustify

#ifdef EMSCRIPTEN
#define group_map_value_options_t    std::vector<uncrustify_options>
#else
#define group_map_value_options_t    std::list<uncrustify_options>
#endif

struct group_map_value
{
   uncrustify_groups         id;
   const char                *short_desc;
   const char                *long_desc;
   group_map_value_options_t options;
};

struct option_map_value
{
   uncrustify_options        id;
   uncrustify_groups         group_id;
   uncrustify::option_type_e type;
   int                       min_val;
   int                       max_val;
   const char                *name;
   const char                *short_desc;
   const char                *long_desc;
};


const char *get_argtype_name(uncrustify::option_type_e argtype);


/**
 * @brief defines a new group of uncrustify options
 *
 * The current group is stored as global variable which
 * will be used whenever a new option is added.
 */
void unc_begin_group(uncrustify_groups id, const char *short_desc, const char *long_desc = NULL);


const option_map_value *unc_find_option(const char *name);


//! Add all uncrustify options to the global option list
void register_options(void);


/**
 * Sets non-zero settings defaults
 *
 * TODO: select from various sets? - i.e., K&R, GNU, Linux, Ben
 */
void set_option_defaults(void);


/**
 * processes a single line string to extract configuration settings
 * increments cpd.line_number and cpd.error_count, modifies configLine parameter
 *
 * @param configLine  single line string that will be processed
 * @param filename    for log messages, file from which the configLine param
 *                    was extracted
 */
void process_option_line(char *configLine, const char *filename);


int load_option_file(const char *filename);


int save_option_file(FILE *pfile, bool withDoc);


/**
 * save the used options into a text file
 *
 * @param pfile             file to print into
 * @param withDoc           also print description
 * @param only_not_default  print only options with non default value
 */
int save_option_file_kernel(FILE *pfile, bool withDoc, bool only_not_default);


/**
 * @return >= 0  entry was found
 * @return   -1  entry was not found
 */
int set_option_value(const char *name, const char *value);

/**
 * get the marker that was selected for the end of line via the config file
 *
 * @return "\n"     if newlines was set to LE_LF in the config file
 * @return "\r\n"   if newlines was set to LE_CRLF in the config file
 * @return "\r"     if newlines was set to LE_CR in the config file
 * @return "\n"     if newlines was set to LE_AUTO in the config file
 * @return "\n"     if newlines was set to LE_AUTO in the config file
 */
const char *get_eol_marker();

/**
 * check if a path/filename uses a relative or absolute path
 *
 * @retval false path is an absolute one
 * @retval true  path is a  relative one
 */
bool is_path_relative(const char *path);


const group_map_value *get_group_name(size_t ug);


const option_map_value *get_option_name(uncrustify_options uo);


/**
 * convert a argument type to a string
 *
 * @param val  argument type to convert
 */
std::string argtype_to_string(uncrustify::option_type_e argtype);

/**
 * convert a boolean to a string
 *
 * @param val  boolean to convert
 */
std::string bool_to_string(bool val);

/**
 * convert an argument value to a string
 *
 * @param val  argument value to convert
 */
std::string argval_to_string(uncrustify::iarf_e argval);

/**
 * convert a line ending type to a string
 *
 * @param val  line ending type to convert
 */
std::string lineends_to_string(uncrustify::lineend_e linends);

/**
 * convert a token to a string
 *
 * @param val  token to convert
 */
std::string tokenpos_to_string(uncrustify::tokenpos_e tokenpos);

/**
 * convert an argument of a given type to a string
 *
 * @param argtype   type of argument
 * @param op_val_t  value of argument
 */
std::string op_val_to_string(uncrustify::option_type_e argtype, op_val_t op_val);


typedef std::map<uncrustify_options, option_map_value>::iterator   option_name_map_it;
typedef std::map<uncrustify_groups, group_map_value>::iterator     group_map_it;
typedef group_map_value_options_t::iterator                        option_list_it;
typedef group_map_value_options_t::const_iterator                  option_list_cit;


#endif /* OPTION_H_INCLUDED */
