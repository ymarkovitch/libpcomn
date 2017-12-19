/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#ifndef __PCOMN_TERNARY_H
#define __PCOMN_TERNARY_H
/*******************************************************************************
 FILE         :   pcomn_ternary.h
 COPYRIGHT    :   Yakov Markovitch, 2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Ternary logic.
 CREATION DATE:   18 Dec 2017
*******************************************************************************/
/** @file
*******************************************************************************/
#include <pcomn_utils.h>
#include <pcomn_strslice.h>
#include <pcomn_meta.h>

namespace pcomn {

struct ternary_tag ;
typedef strong_typedef<uint8_t, ternary_tag> tlogic_t ;

/****************************************************************************//**
 Ternary logic literal type.

 3 states: False, Nothing, True

 False < Nothing < True

 !False == True
 !True == False
 !Nothing == Nothing
*******************************************************************************/
template<> class tdef<uint8_t, ternary_tag, true> {
    static tlogic_t b(const bool *) ;
public:
    /// Numeric values of logical states.
    /// @note These values are _not_ arbitrary, these are not the "implementation
    /// detail", they are essential.
    enum State : uint8_t
    {
        False   = 0,
        Nothing = 1,
        True    = 2
    } ;

    constexpr tdef() = default ;
    constexpr tdef(const tdef &) = default ;
    constexpr tdef(tdef &&) = default ;

    #define P_TLOCIC_NOCTR(type) constexpr tdef(type) = delete
    P_TLOCIC_NOCTR(char) ;
    P_TLOCIC_NOCTR(int8_t) ;
    P_TLOCIC_NOCTR(short) ;
    P_TLOCIC_NOCTR(unsigned short) ;
    P_TLOCIC_NOCTR(int) ;
    P_TLOCIC_NOCTR(unsigned) ;
    P_TLOCIC_NOCTR(long) ;
    P_TLOCIC_NOCTR(unsigned long) ;
    P_TLOCIC_NOCTR(long long) ;
    P_TLOCIC_NOCTR(unsigned long long) ;
    #undef P_TLOCIC_NOCTR

    explicit constexpr tdef(uint8_t v) : _data(v) {}
    constexpr tdef(State s) : tdef((uint8_t)s) {}

    template<typename B, typename=decltype(b(std::add_pointer_t<std::decay_t<B>>()))>
    explicit constexpr tdef(B v) : _data((uint8_t)v << 1) {}

    tdef &operator=(const tdef &) = default ;
    tdef &operator=(tdef &&) = default ;

    template<typename B, typename=decltype(b(std::add_pointer_t<std::decay_t<B>>()))>
    tdef &operator=(B v) { _data = tdef(v)._data ; return *this ; }

    tdef &operator=(State v) { _data = v ; return *this ; }

    constexpr uint8_t data() const { return _data ; }

    constexpr operator State() const { return (State)data() ; }
    constexpr explicit operator uint8_t() const { return data() ; }
    constexpr explicit operator char() const
    {
        return "FNT?"[data() > True ? True + 1 : data()] ;
    }

    /// Convert ternary logic value to bool, interpreting Nothing state according
    /// to the arument.
    /// @param nothing_is How to interpret the "Nothing" state: if `true`, convert
    /// `Nothing` to `true`, otherwise to `false`.
    constexpr bool as_bool(bool nothing_is) const
    {
        return data() + nothing_is >= True ;
    }

    /***************************************************************************
     Logical operations
    ***************************************************************************/
    constexpr tdef operator!() const { return {(State)(2 - _data)} ; }

    friend constexpr inline tlogic_t operator&&(tlogic_t x, tlogic_t y)
    {
        return x.data() < y.data() ? x : y ;
    }

    friend constexpr inline tlogic_t operator||(tlogic_t x, tlogic_t y)
    {
        return x.data() < y.data() ? y : x ;
    }

    template<typename B>
    friend constexpr inline auto operator&&(B x, tlogic_t y) -> decltype(b(&x))
    {
        return tlogic_t(x) && y ;
    }
    template<typename B>
    friend constexpr inline auto operator&&(tlogic_t x, B y) -> decltype(b(&y))
    {
        return y && x ;
    }

    template<typename B>
    friend constexpr inline auto operator||(B x, tlogic_t y) -> decltype(b(&x))
    {
        return tlogic_t(x) || y ;
    }
    template<typename B>
    friend constexpr inline auto operator||(tlogic_t x, B y) -> decltype(b(&y))
    {
        return y || x ;
    }

private:
    uint8_t _data = False ;
} ;

/***************************************************************************//**
 Ternary constants
*******************************************************************************/
/**{@*/
constexpr const tlogic_t TFALSE     (tlogic_t::False) ;
constexpr const tlogic_t TNOTHING   (tlogic_t::Nothing) ;
constexpr const tlogic_t TTRUE      (tlogic_t::True) ;
/**}@*/

/***************************************************************************//**
 Comparison
*******************************************************************************/
/**{@*/
constexpr inline bool operator==(tlogic_t x, tlogic_t y)
{
    return x.data() == y.data() ;
}
constexpr inline bool operator<(tlogic_t x, tlogic_t y)
{
    return x.data() < y.data() ;
}
/**}@*/

PCOMN_DEFINE_RELOP_FUNCTIONS(constexpr, tlogic_t) ;

/*******************************************************************************
 Global functions
*******************************************************************************/
inline std::ostream &operator<<(std::ostream &os, const tlogic_t &v)
{
    return os << (char)v ;
}
} // end of namespace pcomn

/*******************************************************************************
 std::to_string specializations for pcomn::tlogic_t
*******************************************************************************/
namespace std { inline string to_string(const pcomn::tlogic_t &v) { return string(1, (char)v) ; } } ;

#endif /* __PCOMN_TERNARY_H */
