/**
 * @file enum_flags.h
 * Operators for working with bit-flag enumerators.
 *
 * @author  Matthew Woehlke (but mostly "borrowed" from Qt)
 * @license GPL v2+
 */

#ifndef ENUM_FLAGS_H_INCLUDED
#define ENUM_FLAGS_H_INCLUDED

#include <type_traits>

#if __GNUC__ == 4 && !defined (__clang__)
#pragma GCC diagnostic push
#if __GNUC_MINOR__ < 9 || __GNUC_PATCHLEVEL__ < 2
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59624
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#endif
#endif

#define UNC_DECLARE_FLAGS(flag_type, enum_type) \
   using flag_type = ::uncrustify::flags<enum_type>

#define UNC_DECLARE_OPERATORS_FOR_FLAGS(flag_type)                        \
   inline flag_type operator&(flag_type::enum_t f1, flag_type::enum_t f2) \
   { return(flag_type{ f1 } &f2); }                                       \
   inline flag_type operator|(flag_type::enum_t f1, flag_type::enum_t f2) \
   { return(flag_type{ f1 } | f2); }                                      \
   inline flag_type operator|(flag_type::enum_t f1, flag_type f2)         \
   { return(f2 | f1); }                                                   \
   inline void operator|(flag_type::enum_t f1, int f2) = delete

namespace uncrustify
{

//-----------------------------------------------------------------------------
template<typename Enum>
class flags
{
public:
   using enum_t = Enum;
   using int_t  = typename std::underlying_type<enum_t>::type;

   template<typename T> using integral =
      typename std::enable_if<std::is_integral<T>::value, bool>::type;

   inline flags() = default;
   inline flags(Enum flag)
      : m_i{static_cast<int_t>(flag)}
   {}

   inline bool operator==(Enum const &other) const
   { return(m_i == static_cast<int_t>(other)); }
   inline bool operator==(flags const &other) const
   { return(m_i == other.m_i); }
   inline bool operator!=(Enum const &other) const
   { return(m_i != static_cast<int_t>(other)); }
   inline bool operator!=(flags const &other) const
   { return(m_i != other.m_i); }

   template<typename T, integral<T> = true>
   inline flags &operator&=(T mask)
   { m_i &= static_cast<int_t>(mask); return(*this); }

   inline flags &operator|=(flags f)
   { m_i |= f.m_i; return(*this); }
   inline flags &operator|=(Enum f)
   { m_i |= f; return(*this); }

   inline flags &operator^=(flags f)
   { m_i ^= f.m_i; return(*this); }
   inline flags &operator^=(Enum f)
   { m_i ^= f; return(*this); }

   inline operator int_t() const { return(m_i); }
   inline operator enum_t() const { return(static_cast<enum_t>(m_i)); }

   inline flags operator&(Enum f) const
   { flags g; g.m_i = m_i & static_cast<int_t>(f); return(g); }
   inline flags operator&(flags f) const
   { flags g; g.m_i = m_i & static_cast<int_t>(f); return(g); }

   template<typename T, integral<T> = true>
   inline flags operator&(T mask) const
   { flags g; g.m_i = m_i & static_cast<int_t>(mask); return(g); }

   inline flags operator|(flags f) const
   { flags g; g.m_i = m_i | f.m_i; return(g); }
   inline flags operator|(Enum f) const
   { flags g; g.m_i = m_i | static_cast<int_t>(f); return(g); }

   inline flags operator^(flags f) const
   { flags g; g.m_i = m_i ^ f.m_i; return(g); }
   inline flags operator^(Enum f) const
   { flags g; g.m_i = m_i ^ static_cast<int_t>(f); return(g); }

   inline int_t operator~() const
   { return(~m_i); }

   inline operator bool() const { return(!!m_i); }
   inline bool operator!() const { return(!m_i); }

   inline bool test(flags f) const { return((*this & f) == f); }
   inline bool test(Enum f) const { return((*this & f) == f); }

   inline bool test_any() const { return(m_i != 0); }
   inline bool test_any(flags f) const { return((*this & f).test_any()); }

protected:
   int_t m_i = 0;
};

} // namespace uncrustify

#if __GNUC__ == 4 && !defined (__clang__)
#pragma GCC diagnostic pop
#endif

#endif
