/**
 * @file combine_fix_mark_enum_struct_union.h
 *
 * @author  Joshua Parker
 * @license GPL v2+
 */

#ifndef TOKENIZER_ENUM_STRUCT_UNION_PARSER_H_INCLUDED
#define TOKENIZER_ENUM_STRUCT_UNION_PARSER_H_INCLUDED

#include "pcf_flags.h"
#include "token_enum.h"

#include <map>

class Chunk;


/**
 * Class EnumStructUnionParser : This class facilitates the parsing and interpretation
 *                               of ALL instances of the class, enum, union, and
 *                               struct keywords, including user-defined types with
 *                               a body {} and any trailing inline variable declarations
 *                               that may follow the definition (as permitted by
 *                               the coding language in question). The class also
 *                               interprets variable declarations preceded by one
 *                               of those keywords, as well as any C/C++ forward
 *                               declarations
 */
class EnumStructUnionParser
{
public:
   /**
    * Constructor
    */
   EnumStructUnionParser();


   /**
    * Destructor
    */
   ~EnumStructUnionParser();


private:
   /**
    * Analyzes all identifiers (marked as CT_WORD) between the starting and
    * ending chunks and changes CT_WORD to one of CT_TYPE, CT_MACRO_FUNC_CALL,
    * etc. and sets flags (PCF_VAR_1ST, PCF_VAR_1ST_DEF, PCF_VAR_INLINE, etc.)
    * for variable identifiers accordingly. Flags C++ forward declarations as
    * PCF_INCOMPLETE
    */
   void analyze_identifiers();


   /**
    * Returns true if a pair of braces were both detected AND determined to be
    * part of a class/enum/struct/union body
    */
   bool body_detected() const;


   /**
    * Returns true if comma-separated values were detected during parsing
    */
   bool comma_separated_values_detected() const;


   /**
    * Returns true if an enumerated integral type was detected during parsing
    */
   bool enum_base_detected() const;


   /**
    * Returns the end chunk of a class/enum/struct/union body, if detected
    * during parsing
    */
   Chunk *get_body_end() const;


   /**
    * Returns the starting chunk of a class/enum/struct/union body, if detected
    * during parsing
    */
   Chunk *get_body_start() const;


   /**
    * Returns the starting chunk associated with an enumerated type's base
    * specifier statement, if detected during parsing
    */
   Chunk *get_enum_base_start() const;


   /**
    * Returns the first comma encountered at the level of the starting chunk,
    * if detected during parsing
    */
   Chunk *get_first_top_level_comma() const;


   /**
    * Returns the ending chunk associated with an class/struct inheritance
    * list, if detected during parsing
    */
   Chunk *get_inheritance_end() const;


   /**
    * Returns the starting chunk associated with an class/struct inheritance
    * list, if detected during parsing
    */
   Chunk *get_inheritance_start() const;


   /**
    * Returns a numerically-indexed map of all question operators encountered
    * during parsing
    */
   std::map<std::size_t, Chunk *> get_question_operators() const;


   /**
    * Returns the end chunk associated with a template parameter list, if
    * detected during parsing
    */
   Chunk *get_template_end() const;


   /**
    * Return the starting chunk associated with a template parameter list, if
    * detected during parsing
    */
   Chunk *get_template_start() const;


   /**
    * Returns a numerically-indexed map of all top-level commas encountered
    * during parsing
    */
   std::map<std::size_t, Chunk *> get_top_level_commas() const;


   /**
    * Return the starting chunk associated with a where clause, if
    * detected during parsing
    */
   Chunk *get_where_end() const;


   /**
    * Return the starting chunk associated with a where clause, if
    * detected during parsing
    */
   Chunk *get_where_start() const;


   /**
    * Returns true if an inheritance list associated with a class or struct was
    * discovered during parsing
    */
   bool inheritance_detected() const;


public:
   /**
    * Performs object initialization prior to parsing
    */
   void initialize(Chunk *pc);


private:
   /**
    * Returns true if the chunk under test represents a potential end chunk past
    * which further parsing is not likely warranted
    */
   bool is_potential_end_chunk(Chunk *pc) const;


   /**
    * Returns true if the chunk under test is deemed to be located within a
    * conditional/ternary statement
    */
   bool is_within_conditional(Chunk *pc) const;


   /**
    * Returns true if the chunk under test is deemed to be located within an
    * inheritance list
    */
   bool is_within_inheritance_list(Chunk *pc) const;


   /**
    * Returns true if the chunk under test is deemed to be located within a
    * where clause
    */
   bool is_within_where_clause(Chunk *pc) const;


   /**
    * Marks all base classes that appear as part of an inheritance list
    */
   void mark_base_classes(Chunk *pc);


   /**
    * Marks pairs of braces associated with the body of a class/enum/struct/union,
    * and additionally calls a separate routine to mark any base classes for that
    * may precede the opening brace
    */
   void mark_braces(Chunk *start);


   /**
    * Marks the beginning chunk of an inheritance list
    */
   void mark_class_colon(Chunk *colon);


   /**
    * Mark a colon as a conditional
    */
   void mark_conditional_colon(Chunk *colon);


   /**
    * Mark any struct/class constructor declarations/definitions
    */
   void mark_constructors();


   /**
    * Marks the beginning chunk of an enumerated integral type specification
    */
   void mark_enum_integral_type(Chunk *colon);


   /**
    * Scan chunks outside the definition body and mark lvalues accordingly
    */
   void mark_extracorporeal_lvalues();


   /**
    * Mark nested name specifiers preceding qualified identifiers
    */
   void mark_nested_name_specifiers(Chunk *pc);


   /**
    * Marks pointer operators preceding a variable identifier
    */
   void mark_pointer_types(Chunk *pc);


   /**
    * Marks the beginning and ending chunks associated with a template
    * (templates may appear after the identifier type name as part of a class
    * specialization)
    */
   void mark_template(Chunk *start) const;


   /**
    * Marks the arguments within a template argument list bounded by the
    * starting and ending chunks
    */
   void mark_template_args(Chunk *start, Chunk *end) const;


   /**
    * Marks the type identifier associated with the class/enum/struct/union,
    * if not anonymously defined
    */
   void mark_type(Chunk *pc);


   /**
    * Marks all variable identifiers associated with the class/enum/struct/union
    */
   void mark_variable(Chunk *variable, PcfFlags flags);


   /**
    * Marks all chunks belonging to a c# where clause
    */
   void mark_where_clause(Chunk *where);


   /**
    * Marks the beginning of a where clause
    */
   void mark_where_colon(Chunk *colon);


public:
   /**
    * Parses the class/enum/struct/union and all associated chunks
    */
   void parse(Chunk *pc);


private:
   /**
    * Parses closing and opening angle brackets
    */
   Chunk *parse_angles(Chunk *angle_open);


   /**
    * Parses closing and opening braces
    */
   Chunk *parse_braces(Chunk *brace_open);


   /**
    * Parses a single colon, which may precede an inheritance list or
    * enumerated integral type specification
    */
   void parse_colon(Chunk *colon);


   /**
    * Parses a double colon, which may indicate a scope resolution chain
    */
   Chunk *parse_double_colon(Chunk *double_colon);


   /**
    * Returns the parsing error status
    */
   bool parse_error_detected() const;


   /**
    * Sets the parsing error status
    */
   void parse_error_detected(bool status);


   /**
    * Records all question operators encountered during parsing
    */
   void record_question_operator(Chunk *question);


   /**
    * Records a comma chunk given one the following conditions are satisfied:
    * 1) it is encountered at the level of the starting chunk
    * 2) it is not part of a right-hand side assignment
    * 3) it is not part of an inheritance list
    * 4) it is not part of a conditional/ternary expression
    */
   void record_top_level_comma(Chunk *comma);


   /**
    * Adjusts the end chunk returned by the try_find_end_chunk() function
    * for any potential trailing inline variable declarations that may follow
    * the body of a class/enum/struct/union definition
    */
   Chunk *refine_end_chunk(Chunk *pc);


   /**
    * Sets the chunk associated with the end of a class/enum/struct/union
    * body
    */
   void set_body_end(Chunk *body_end);


   /**
    * Sets the chunk associated with the start of a class/enum/struct/union
    * body
    */
   void set_body_start(Chunk *body_start);


   /**
    * Sets the chunk associated with the start of an enumerated integral
    * base type specification
    */
   void set_enum_base_start(Chunk *enum_base_start);


   /**
    * Sets the chunk associated with the start of an inheritance list
    */
   void set_inheritance_start(Chunk *inheritance_start);


   /**
    * Sets the chunk associated with the end of a template
    */
   void set_template_end(Chunk *template_end);


   /**
    * Sets the chunk associated with the start of a template
    */
   void set_template_start(Chunk *template_start);


   /**
    * Return the ending chunk associated with a where clause, if
    * detected during parsing
    */
   void set_where_end(Chunk *where_end);


   /**
    * Return the starting chunk associated with a where clause, if
    * detected during parsing
    */
   void set_where_start(Chunk *where_start);


   /**
    * Returns true if a template was detected during parsing
    */
   bool template_detected() const;


   /**
    * Attempts to find the last chunk associated with the class/enum/struct/union
    */
   Chunk *try_find_end_chunk(Chunk *pc);


   /**
    * Attempts to identify any function-like macro calls which may precede the
    * actual type identifier
    */
   void try_post_identify_macro_calls();


   /**
    * Attempts to find the identifier type name (if not anonymously-defined) post
    * variable identifier interpretation
    */
   void try_post_identify_type();


   /**
    * Attempts to find the identifier type name prior to variable identifier
    * interpretation
    */
   bool try_pre_identify_type();


   /**
    * Returns true if a corresponding type was identified for the class/enum/struct/union
    */
   bool type_identified() const;


   /**
    * Returns true if a where clause was detected during parsing
    */
   bool where_clause_detected() const;


   /**
    * Map of token-type, chunk pairs
    */
   std::map<E_Token, std::map<std::size_t, Chunk *> > m_chunk_map;


   /**
    * Indicates the last chunk associated with the class/enum/struct/union keyword
    */
   Chunk *m_end;


   /**
    * Indicates whether or not a parse error has occurred
    */
   bool m_parse_error;


   /**
    * Stores a pointer to the class/enum/struct/union keyword chunk with which the
    * parse() routine was invoked
    */
   Chunk *m_start;


   /**
    * Stores a pointer to the type identifier associated with the class/enum/struct/union,
    * if not anonymously defined
    */
   Chunk *m_type;
};


#endif
