/**
 * @file options_for_QT.cpp
 * Save the options which are needed to be changed to
 * process the SIGNAL and SLOT QT macros.
 * http://doc.qt.io/qt-4.8/qtglobal.html
 *
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */

#include "options_for_QT.h"

using namespace uncrustify;

// for the modification of options within the SIGNAL/SLOT call.
bool   QT_SIGNAL_SLOT_found      = false;
size_t QT_SIGNAL_SLOT_level      = 0;
bool   restoreValues             = false;
iarf_e SaveUO_sp_inside_fparen_A = IARF_NOT_DEFINED;
// Issue #481
// connect( timer,SIGNAL( timeout() ),this,SLOT( timeoutImage() ) );
iarf_e SaveUO_sp_inside_fparens_A = IARF_NOT_DEFINED;
iarf_e SaveUO_sp_paren_paren_A    = IARF_NOT_DEFINED;
iarf_e SaveUO_sp_before_comma_A   = IARF_NOT_DEFINED;
iarf_e SaveUO_sp_after_comma_A    = IARF_NOT_DEFINED;
// Bug #654
// connect(&mapper, SIGNAL(mapped(QString &)), this, SLOT(onSomeEvent(QString &)));
iarf_e SaveUO_sp_before_byref_A         = IARF_NOT_DEFINED;
iarf_e SaveUO_sp_before_unnamed_byref_A = IARF_NOT_DEFINED;
iarf_e SaveUO_sp_after_type_A           = IARF_NOT_DEFINED;


void save_set_options_for_QT(size_t level)
{
   assert(options::use_options_overriding_for_qt_macros());

   LOG_FMT(LGUY, "save values, level=%zu\n", level);
   // save the values
   QT_SIGNAL_SLOT_level             = level;
   SaveUO_sp_inside_fparen_A        = options::sp_inside_fparen();
   SaveUO_sp_inside_fparens_A       = options::sp_inside_fparens();
   SaveUO_sp_paren_paren_A          = options::sp_paren_paren();
   SaveUO_sp_before_comma_A         = options::sp_before_comma();
   SaveUO_sp_after_comma_A          = options::sp_after_comma();
   SaveUO_sp_before_byref_A         = options::sp_before_byref();
   SaveUO_sp_before_unnamed_byref_A = options::sp_before_unnamed_byref();
   SaveUO_sp_after_type_A           = options::sp_after_type();
   // set values for SIGNAL/SLOT
   options::sp_inside_fparen()        = IARF_REMOVE;
   options::sp_inside_fparens()       = IARF_REMOVE;
   options::sp_paren_paren()          = IARF_REMOVE;
   options::sp_before_comma()         = IARF_REMOVE;
   options::sp_after_comma()          = IARF_REMOVE;
   options::sp_before_byref()         = IARF_REMOVE;
   options::sp_before_unnamed_byref() = IARF_REMOVE;
   options::sp_after_type()           = IARF_REMOVE;
   QT_SIGNAL_SLOT_found                       = true;
}


void restore_options_for_QT(void)
{
   assert(options::use_options_overriding_for_qt_macros());

   LOG_FMT(LGUY, "restore values\n");
   // restore the values we had before SIGNAL/SLOT
   QT_SIGNAL_SLOT_level                       = 0;
   options::sp_inside_fparen()        = SaveUO_sp_inside_fparen_A;
   options::sp_inside_fparens()       = SaveUO_sp_inside_fparens_A;
   options::sp_paren_paren()          = SaveUO_sp_paren_paren_A;
   options::sp_before_comma()         = SaveUO_sp_before_comma_A;
   options::sp_after_comma()          = SaveUO_sp_after_comma_A;
   options::sp_before_byref()         = SaveUO_sp_before_byref_A;
   options::sp_before_unnamed_byref() = SaveUO_sp_before_unnamed_byref_A;
   options::sp_after_type()           = SaveUO_sp_after_type_A;
   SaveUO_sp_inside_fparen_A                  = IARF_NOT_DEFINED;
   SaveUO_sp_inside_fparens_A                 = IARF_NOT_DEFINED;
   SaveUO_sp_paren_paren_A                    = IARF_NOT_DEFINED;
   SaveUO_sp_before_comma_A                   = IARF_NOT_DEFINED;
   SaveUO_sp_after_comma_A                    = IARF_NOT_DEFINED;
   SaveUO_sp_before_byref_A                   = IARF_NOT_DEFINED;
   SaveUO_sp_before_unnamed_byref_A           = IARF_NOT_DEFINED;
   SaveUO_sp_after_type_A                     = IARF_NOT_DEFINED;
   QT_SIGNAL_SLOT_found                       = false;
   restoreValues                              = false;
}
