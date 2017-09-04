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

// for the modification of options within the SIGNAL/SLOT call.
bool     QT_SIGNAL_SLOT_found      = false;
size_t   QT_SIGNAL_SLOT_level      = 0;
bool     restoreValues             = false;
argval_t SaveUO_sp_inside_fparen_A = AV_NOT_DEFINED;
// Issue #481
// connect( timer,SIGNAL( timeout() ),this,SLOT( timeoutImage() ) );
argval_t SaveUO_sp_inside_fparens_A = AV_NOT_DEFINED;
argval_t SaveUO_sp_paren_paren_A    = AV_NOT_DEFINED;
argval_t SaveUO_sp_before_comma_A   = AV_NOT_DEFINED;
argval_t SaveUO_sp_after_comma_A    = AV_NOT_DEFINED;
// Bug #654
// connect(&mapper, SIGNAL(mapped(QString &)), this, SLOT(onSomeEvent(QString &)));
argval_t SaveUO_sp_before_byref_A         = AV_NOT_DEFINED;
argval_t SaveUO_sp_before_unnamed_byref_A = AV_NOT_DEFINED;
argval_t SaveUO_sp_after_type_A           = AV_NOT_DEFINED;


void save_set_options_for_QT(size_t level)
{
   assert(cpd.settings[UO_use_options_overriding_for_qt_macros].b);

   LOG_FMT(LGUY, "save values, level=%zu\n", level);
   // save the values
   QT_SIGNAL_SLOT_level             = level;
   SaveUO_sp_inside_fparen_A        = cpd.settings[UO_sp_inside_fparen].a;
   SaveUO_sp_inside_fparens_A       = cpd.settings[UO_sp_inside_fparens].a;
   SaveUO_sp_paren_paren_A          = cpd.settings[UO_sp_paren_paren].a;
   SaveUO_sp_before_comma_A         = cpd.settings[UO_sp_before_comma].a;
   SaveUO_sp_after_comma_A          = cpd.settings[UO_sp_after_comma].a;
   SaveUO_sp_before_byref_A         = cpd.settings[UO_sp_before_byref].a;
   SaveUO_sp_before_unnamed_byref_A = cpd.settings[UO_sp_before_unnamed_byref].a;
   SaveUO_sp_after_type_A           = cpd.settings[UO_sp_after_type].a;
   // set values for SIGNAL/SLOT
   cpd.settings[UO_sp_inside_fparen].a        = AV_REMOVE;
   cpd.settings[UO_sp_inside_fparens].a       = AV_REMOVE;
   cpd.settings[UO_sp_paren_paren].a          = AV_REMOVE;
   cpd.settings[UO_sp_before_comma].a         = AV_REMOVE;
   cpd.settings[UO_sp_after_comma].a          = AV_REMOVE;
   cpd.settings[UO_sp_before_byref].a         = AV_REMOVE;
   cpd.settings[UO_sp_before_unnamed_byref].a = AV_REMOVE;
   cpd.settings[UO_sp_after_type].a           = AV_REMOVE;
   QT_SIGNAL_SLOT_found                       = true;
}


void restore_options_for_QT(void)
{
   assert(cpd.settings[UO_use_options_overriding_for_qt_macros].b);

   LOG_FMT(LGUY, "restore values\n");
   // restore the values we had before SIGNAL/SLOT
   QT_SIGNAL_SLOT_level                       = 0;
   cpd.settings[UO_sp_inside_fparen].a        = SaveUO_sp_inside_fparen_A;
   cpd.settings[UO_sp_inside_fparens].a       = SaveUO_sp_inside_fparens_A;
   cpd.settings[UO_sp_paren_paren].a          = SaveUO_sp_paren_paren_A;
   cpd.settings[UO_sp_before_comma].a         = SaveUO_sp_before_comma_A;
   cpd.settings[UO_sp_after_comma].a          = SaveUO_sp_after_comma_A;
   cpd.settings[UO_sp_before_byref].a         = SaveUO_sp_before_byref_A;
   cpd.settings[UO_sp_before_unnamed_byref].a = SaveUO_sp_before_unnamed_byref_A;
   cpd.settings[UO_sp_after_type].a           = SaveUO_sp_after_type_A;
   SaveUO_sp_inside_fparen_A                  = AV_NOT_DEFINED;
   SaveUO_sp_inside_fparens_A                 = AV_NOT_DEFINED;
   SaveUO_sp_paren_paren_A                    = AV_NOT_DEFINED;
   SaveUO_sp_before_comma_A                   = AV_NOT_DEFINED;
   SaveUO_sp_after_comma_A                    = AV_NOT_DEFINED;
   SaveUO_sp_before_byref_A                   = AV_NOT_DEFINED;
   SaveUO_sp_before_unnamed_byref_A           = AV_NOT_DEFINED;
   SaveUO_sp_after_type_A                     = AV_NOT_DEFINED;
   QT_SIGNAL_SLOT_found                       = false;
   restoreValues                              = false;
}
