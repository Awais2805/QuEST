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
 * complex type and operator overloads, which must ergo crucially
 * be POD and share the memory layout and alignment of qcomp
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


// get cpu_qcomp from components
INLINE cpu_qcomp getCpuQcomp(qreal re, qreal im) {
    return { re, im };
}


// get cpu_qcomp from qcomp
INLINE cpu_qcomp getCpuQcomp(const qcomp& a) {
    return { a.real(), a.imag() };
}


// get qcomp from cpu_qcomp
INLINE qcomp getQcomp(const cpu_qcomp& a) {
    return qcomp( a.re, a.im );
}


// creator for fixed-size dense matrices (CompMatr1 and CompMatr2) ((not inlined!))
std::array<std::array<cpu_qcomp,2>,2> getCpuQcompsMatr1(qcomp matr[2][2]) {

    // dumb and explicit here because MSVC + OpenMP breaks
    // when templating this - not worth fixing here because
    // we are considering a refactor which merges cpu_types.hpp
    // with gpu_types.cuh anyway

    std::array<std::array<cpu_qcomp,2>,2> out;

    for (int i=0; i<2; i++)
        for (int j=0; j<2; j++)
            out[i][j] = getCpuQcomp(matr[i][j]);

    return out;
}
std::array<std::array<cpu_qcomp,4>,4> getCpuQcompsMatr2(qcomp matr[4][4]) {

    std::array<std::array<cpu_qcomp,4>,4> out;

    for (int i=0; i<4; i++)
        for (int j=0; j<4; j++)
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

    // Here, we re-use std::pow(std::complex) to avoid a custom definition,
    // and so accept NaN-check performance penalties. Notice too we also
    // create new qcomp(), rather than just reinterpreting the given cpu_qcomp,
    // just to avoid any insiduous issues alignment/aliasing issues (since the
    // creation time iss occluded by std::pow time).
    qcomp base_ = getQcomp(base);
    qcomp expo_ = getQcomp(expo);
    qcomp out_ = std::pow(base_, expo_);
    return getCpuQcomp(out_);
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





INLINE cpu_qcomp fast_getPauliStrElem(PauliStr str, qindex row, qindex col) {

    // this function is called by both fullstatediagmatr_setElemsToPauliStrSum()
    // and densmatr_setAmpsToPauliStrSum_sub(). The former's PauliStr can have
    // Paulis on any of the 64 sites, but the latter's PauliStr is always
    // constrainted to the lower 32 sites (because a 32-qubit density matrix
    // is already too large for the world's computers). As such, the latter
    // scenario can be optimised since str.highPaulis == 0, making the second
    // loop below redundant. Avoiding this loop can at most half the runtime,
    // though opens the risk that the former caller erroneously has its upper
    // Paulis ignore. We forego this optimisation in defensive design, and
    // because this function is only invoked during data structure initilisation
    // and ergo infrequently.

    // regrettably duplicated from paulis.cpp which is inaccessible here
    constexpr int numPaulisPerMask = sizeof(PAULI_MASK_TYPE) * 8 / 2;

    // T-agnostic complex literals
    cpu_qcomp p0, p1,n1, pI,nI;
    p0 = {0,  0}; //  0
    p1 = {+1, 0}; //  1
    n1 = {-1, 0}; // -1
    pI = {0, +1}; //  i
    nI = {0, -1}; // -i

    // 'matrices' below is not declared constexpr or static const, even though
    // it is fixed/known at compile-time, because this makes it incompatible
    // with CUDA kernels/thrust. It is instead left as runtime innitialisation
    // but this poses no real slowdown; this function, and its caller, are inlined
    // so these 16 amps are re-processed one for each full enumeration of the
    // PauliStrSum which is expected to have significantly more terms/coeffs
    cpu_qcomp matrices[][2][2] = {
        {{p1,p0},{p0,p1}},  // I
        {{p0,p1},{p1,p0}},  // X
        {{p0,nI},{pI,p0}},  // Y
        {{p1,p0},{p0,n1}}}; // Z

    cpu_qcomp elem = p1; // 1

    // could be compile-time unrolled into 32 iterations
    for (int t=0; t<numPaulisPerMask; t++) {
        int p = getTwoAdjacentBits(str.lowPaulis, 2*t);
        int i = getBit(row, t);
        int j = getBit(col, t);
        elem *= matrices[p][i][j];
    }

    // could be compile-time unrolled into 32 iterations
    for (int t=0; t<numPaulisPerMask; t++) {
        int p = getTwoAdjacentBits(str.highPaulis, 2*t);
        int i = getBit(row, t + numPaulisPerMask);
        int j = getBit(col, t + numPaulisPerMask);
        elem *= matrices[p][i][j];
    }

    return elem;
}



INLINE cpu_qcomp fast_getPauliStrSumElem(cpu_qcomp* coeffs, PauliStr* strings, qindex numTerms, qindex row, qindex col) {

    // this function accepts unpacked PauliStrSum fields since a PauliStrSum cannot 
    // be directly processed in CUDA kernels/thrust due to its 'qcomp' field.
    // it also assumes str.highPaulis==0 for all str in strings, as per above func.

    cpu_qcomp elem = {0, 0}; // type-agnostic complex literal

    // this loop is expected exponentially smaller than caller's loop
    for (qindex n=0; n<numTerms; n++)
        elem += coeffs[n] * fast_getPauliStrElem(strings[n], row, col);

    return elem;
}




#endif // CPU_TYPES_HPP