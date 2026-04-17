/** @file
 * Custom types used exclusively by the CPU backend.
 * 
 * @author Tyson Jones
 */

#ifndef CPU_TYPES_HPP
#define CPU_TYPES_HPP

#include "quest/include/types.h"

#include "quest/src/core/inliner.hpp"

#include <array>



/*
 * COMPLEX SCALAR
 *
 * The user-facing qcomp (which in the QuEST middle-end, resolves to
 * a std::complex) is not used by the CPU backend, since it creates
 * performance pitfalls (e.g. expensive NaN checks within arithmetic
 * operators) in some compilers. Instead, we use the below custom
 * complex type and operator overloads.
 */


struct cpu_qcomp {

    // memory layout
    qreal re;
    qreal im;

    // in-place complex arithmetic overloads
    INLINE cpu_qcomp& operator += (const cpu_qcomp& a) noexcept {
        re += a.re;
        im += a.im;
        return *this;
    }
    INLINE cpu_qcomp& operator -= (const cpu_qcomp& a) noexcept {
        re -= a.re;
        im -= a.im;
        return *this;
    }
    INLINE cpu_qcomp& operator *= (const cpu_qcomp& a) noexcept {
        qreal re_ = re;
        qreal im_ = im;
        re = (re_ * a.re) - (im_ * a.im);
        im = (re_ * a.im) + (im_ * a.re);
        return *this;
    }

    // in-place mixed-type arithmetic overloads
    INLINE cpu_qcomp& operator *= (const int& a) noexcept {
        re *= a;
        im *= a;
        return *this;
    }
    INLINE cpu_qcomp& operator *= (const qreal& a) noexcept {
        re *= a;
        im *= a;
        return *this;
    }
};


// out-of-place complex arithmetic overloads (optimised)
INLINE cpu_qcomp operator + (cpu_qcomp a, const cpu_qcomp& b) noexcept {
    a += b;
    return a;
}
INLINE cpu_qcomp operator - (cpu_qcomp a, const cpu_qcomp& b) noexcept {
    a -= b;
    return a;
}
INLINE cpu_qcomp operator * (cpu_qcomp a, const cpu_qcomp& b) noexcept {
    a *= b;
    return a;
}


// out-of-place mixed-type arithmetic overloads
INLINE cpu_qcomp operator * (cpu_qcomp a, const int& b) noexcept {
    a *= b;
    return a;
}
INLINE cpu_qcomp operator * (cpu_qcomp a, const qreal& b) noexcept {
    a *= b;
    return a;
}


// reverse order of out-of-place mixed-type arithmetic (via commutation)
INLINE cpu_qcomp operator * (const int& a, const cpu_qcomp& b) noexcept {
    return b * a;
}
INLINE cpu_qcomp operator * (const qreal& a, const cpu_qcomp& b) noexcept {
    return b * a;
}


// no-op cast of pointers
INLINE cpu_qcomp* getCpuQcompPtr(qcomp* list) {

    return reinterpret_cast<cpu_qcomp*>(list);
}


// creator for cpu_qcomp literals
INLINE cpu_qcomp getCpuQcomp(qreal re, qreal im) {
    return { re, im };
}

// creator for qcomp conversion
INLINE cpu_qcomp getCpuQcomp(const qcomp& a) {
    return { a.real(), a.imag() };
}

// creator for fixed-size dense matrices (CompMatr1 and CompMatr2)
template <int dim>
INLINE std::array<std::array<cpu_qcomp,dim>,dim> getCpuQcomps(qcomp matr[dim][dim]) {

    std::array<std::array<cpu_qcomp,dim>,dim> out;

    for (int i=0; i<dim; i++)
        for (int j=0; j<dim; j++)
            out[i][j] = getCpuQcomp(matr[i][j]);

    return out;
}

// maths functions
INLINE qreal real(const cpu_qcomp& a) {
    return a.re;
}
INLINE qreal imag(const cpu_qcomp& a) {
    return a.im;
}
INLINE cpu_qcomp conj(const cpu_qcomp& a) {
    return {a.re, - a.im};
}
INLINE qreal norm(const cpu_qcomp& a) noexcept {
    return (a.re * a.re) + (a.im * a.im);
}
INLINE cpu_qcomp pow(cpu_qcomp base, cpu_qcomp expo) noexcept {
    
    // TODO: here, we re-use std::pow(std::complex), accepting NaN-check
    //       performance penalties - find a speedup without compiler flags!
    auto& base_ = reinterpret_cast<const qcomp&>(base);
    auto& expo_ = reinterpret_cast<const qcomp&>(expo);
    qcomp out = std::pow(base_, expo_);
    return reinterpret_cast<cpu_qcomp&>(out);
}


// check the memory layout of cpu_qcomp agrees with qcomp, since
// it is not formally gauranteed, unlike _Complex and std::complex
static_assert(sizeof (cpu_qcomp) == sizeof (qcomp));
static_assert(alignof(cpu_qcomp) == alignof(qcomp));
static_assert(std::is_standard_layout_v   <cpu_qcomp>);
static_assert(std::is_trivially_copyable_v<cpu_qcomp>);


// TODO:
// the above checks are potentially inadequate to identify an
// insidious incompatibility between qcomp and cpu_qcomp - perhaps
// we should perform a compile-time duck-check, casting a small
// array between them and checking no data is corrupted? Perhaps
// a runtime check in initQuESTEnv() is also necessary, checking the
// casting is safe for all circumstances (e.g. heap mem, static lists)


#endif // CPU_TYPES_HPP