/**
 * @file handle_oc.h
 * prototypes for handle_oc.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.cpp
 */

#ifndef HANDLE_OC_H_INCLUDED
#define HANDLE_OC_H_INCLUDED

#include "chunk_list.h"
#include "uncrustify_types.h"


/**
 * Process an ObjC 'class'
 * pc is the chunk after '@implementation' or '@interface' or '@protocol'.
 * Change colons, etc. Processes stuff until '@end'.
 * Skips anything in braces.
 */
void handle_oc_class(chunk_t *pc);


/**
 *  Mark Objective-C blocks (aka lambdas or closures)
 *  The syntax and usage is exactly like C function pointers
 *  but instead of an asterisk they have a caret as pointer symbol.
 *  Although it may look expensive this functions is only triggered
 *  on appearance of an OC_BLOCK_CARET for LANG_OC.
 *  repeat(10, ^{ putc('0'+d); });
 *  typedef void (^workBlk_t)(void);
 *
 * @param pc  points to the '^'
 */
void handle_oc_block_literal(chunk_t *pc);


/**
 * Mark Objective-C block types.
 * The syntax and usage is exactly like C function pointers
 * but instead of an asterisk they have a caret as pointer symbol.
 *  typedef void (^workBlk_t)(void);
 *  const char * (^workVar)(void);
 *  -(void)Foo:(void(^)())blk { }
 *
 * This is triggered when the sequence '(' '^' is found.
 *
 * @param pc  points to the '^'
 */
void handle_oc_block_type(chunk_t *pc);


/**
 * Process an ObjC message spec/dec
 *
 * Specs:
 * -(void) foo ARGS;
 *
 * Declaration:
 * -(void) foo ARGS {  }
 *
 * LABEL : (ARGTYPE) ARGNAME
 *
 * ARGS is ': (ARGTYPE) ARGNAME [MOREARGS...]'
 * MOREARGS is ' [ LABEL] : (ARGTYPE) ARGNAME '
 * -(void) foo: (int) arg: {  }
 * -(void) foo: (int) arg: {  }
 * -(void) insertObject:(id)anObject atIndex:(int)index
 */
void handle_oc_message_decl(chunk_t *pc);


/**
 * Process an ObjC message send statement:
 * [ class func: val1 name2: val2 name3: val3] ; // named params
 * [ class func: val1      : val2      : val3] ; // unnamed params
 * [ class <proto> self method ] ; // with protocol
 * [[NSMutableString alloc] initWithString: @"" ] // class from msg
 * [func(a,b,c) lastObject ] // class from func
 *
 * Mainly find the matching ']' and ';' and mark the colons.
 *
 * @param pc  points to the open square '['
 */
void handle_oc_message_send(chunk_t *pc);


//! Process @Property values and re-arrange them if necessary
void handle_oc_property_decl(chunk_t *pc);

//! Process @available annotation
void handle_oc_available(chunk_t *pc);

/**
 * Process a type that is enclosed in parens in message declarations.
 * TODO: handle block types, which get special formatting
 *
 * @param pc  points to the open paren
 *
 * @return the chunk after the type
 */
chunk_t *handle_oc_md_type(chunk_t *paren_open, c_token_t ptype, pcf_flags_t flags, bool &did_it);

bool is_oc_block(chunk_t *pc);


#endif /* HANDLE_OC_H_INCLUDED */
