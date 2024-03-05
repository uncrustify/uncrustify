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

#include "log_rules.h"


constexpr static auto LCURRENT = LQT;


using namespace uncrustify;


// for the modification of options within the SIGNAL/SLOT call.
bool   QT_SIGNAL_SLOT_found = false;
size_t QT_SIGNAL_SLOT_level = 0;
bool   restoreValues        = false;

namespace
{

//-----------------------------------------------------------------------------
class temporary_iarf_option
{
public:
   temporary_iarf_option(Option<iarf_e> *option,
                         iarf_e         override_value = IARF_REMOVE)
      : m_option{option}
      , m_override_value{override_value}
   {}

   void save_and_override();
   void restore();

private:
   Option<iarf_e> *m_option;
   const iarf_e   m_override_value;

   iarf_e         m_saved_value = IARF_IGNORE;
};


//-----------------------------------------------------------------------------
void temporary_iarf_option::save_and_override()
{
   m_saved_value = (*m_option)();
   (*m_option)   = m_override_value;
}


//-----------------------------------------------------------------------------
void temporary_iarf_option::restore()
{
   (*m_option)   = m_saved_value;
   m_saved_value = IARF_IGNORE;
}

//-----------------------------------------------------------------------------
temporary_iarf_option for_qt_options[] =
{
   { &options::sp_inside_fparen           },
// Issue #481
// connect( timer,SIGNAL( timeout() ),this,SLOT( timeoutImage() ) );
   { &options::sp_inside_fparens          },
   { &options::sp_paren_paren             },
   { &options::sp_before_comma            },
   { &options::sp_after_comma             },
// Bug #654
// connect(&mapper, SIGNAL(mapped(QString &)), this, SLOT(onSomeEvent(QString &)));
   { &options::sp_before_byref            },
   { &options::sp_before_unnamed_byref    },
   { &options::sp_after_type              },
// Issue #1969
// connect( a, SIGNAL(b(c *)), this, SLOT(d(e *)) );
   { &options::sp_before_ptr_star         },
   { &options::sp_before_unnamed_ptr_star },
// connect( a, SIGNAL(b(c< d >)), this, SLOT(e(f< g >)) );
   { &options::sp_inside_angle            },
};

} // anonymous namespace


//-----------------------------------------------------------------------------
void save_set_options_for_QT(size_t level)
{
   log_rule_B("use_options_overriding_for_qt_macros");
   assert(options::use_options_overriding_for_qt_macros());

   LOG_FMT(LGUY, "save values, level=%zu\n", level);
   // save the values
   QT_SIGNAL_SLOT_level = level;

   for (auto &opt : for_qt_options)
   {
      opt.save_and_override();
   }

   QT_SIGNAL_SLOT_found = true;
}


//-----------------------------------------------------------------------------
void restore_options_for_QT()
{
   log_rule_B("use_options_overriding_for_qt_macros");
   assert(options::use_options_overriding_for_qt_macros());

   LOG_FMT(LGUY, "restore values\n");
   // restore the values we had before SIGNAL/SLOT
   QT_SIGNAL_SLOT_level = 0;

   for (auto &opt : for_qt_options)
   {
      opt.restore();
   }

   QT_SIGNAL_SLOT_found = false;
   restoreValues        = false;
}
