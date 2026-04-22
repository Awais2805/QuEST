/** @file
 * CUDA and HIP-compatible complex types. This file is only ever included
 * when COMPILE_CUDA=1 so it can safely invoke CUDA signatures without guards. 
 * 
 * This header is safe to re-include by multiple files because typedef 
 * redefinition is legal in C++, and all functions herein are inline. 
 * Furthermore, since it is only ever parsed by nvcc, the __host__ symbols 
 * are safely processed by other nvcc-only GPU files, like the cuquantum backend.
 * 
 * @author Tyson Jones
 * @author Oliver Brown (patched HIP arithmetic overloads)
 */

#ifndef GPU_TYPES_HPP
#define GPU_TYPES_HPP

#include "quest/include/config.h"
#include "quest/include/types.h"
#include "quest/include/precision.h"

#include "quest/src/core/inliner.hpp"

#if ! COMPILE_CUDA
    #error "A file being compiled somehow included gpu_types.hpp despite QuEST not being compiled in GPU-accelerated mode."
#endif

#if (FLOAT_PRECISION == 4)
    #error "Build bug; precision.h should have prevented non-float non-double qcomp precision on GPU."
#endif

#if defined(__HIP__)
    #include "quest/src/gpu/cuda_to_hip.hpp"
#endif

#include <array>
#include <vector>



/*
 * COMPLEX SCALAR
 *
 * The user-facing qcomp (which in the QuEST middle-end, resolves to
 * a std::complex) is not used by the GPU backend, since incompatible
 * with CUDA kernels. We use our own custom gpu_qcomp type below, in
 * lieu of cuComplex or Thrust types, to workaround compatibility issues
 * with HIP, and for better symmetry with cpu_qcomp.
 */


struct gpu_qcomp {

    // memory layout
    qreal re;
    qreal im;

    // in-place complex arithmetic overloads
    INLINE gpu_qcomp& operator += (const gpu_qcomp& a) noexcept {
        re += a.re;
        im += a.im;
        return *this;
    }
    INLINE gpu_qcomp& operator -= (const gpu_qcomp& a) noexcept {
        re -= a.re;
        im -= a.im;
        return *this;
    }
    INLINE gpu_qcomp& operator *= (const gpu_qcomp& a) noexcept {
        qreal re_ = re;
        qreal im_ = im;
        re = (re_ * a.re) - (im_ * a.im);
        im = (re_ * a.im) + (im_ * a.re);
        return *this;
    }

    // in-place mixed-type arithmetic overloads
    INLINE gpu_qcomp& operator *= (const int& a) noexcept {
        re *= a;
        im *= a;
        return *this;
    }
    INLINE gpu_qcomp& operator *= (const qreal& a) noexcept {
        re *= a;
        im *= a;
        return *this;
    }
};


// out-of-place complex arithmetic overloads (optimised)
INLINE gpu_qcomp operator + (gpu_qcomp a, const gpu_qcomp& b) noexcept {
    a += b;
    return a;
}
INLINE gpu_qcomp operator - (gpu_qcomp a, const gpu_qcomp& b) noexcept {
    a -= b;
    return a;
}
INLINE gpu_qcomp operator * (gpu_qcomp a, const gpu_qcomp& b) noexcept {
    a *= b;
    return a;
}


// out-of-place mixed-type arithmetic overloads
INLINE gpu_qcomp operator * (gpu_qcomp a, const int& b) noexcept {
    a *= b;
    return a;
}
INLINE gpu_qcomp operator * (gpu_qcomp a, const qreal& b) noexcept {
    a *= b;
    return a;
}
INLINE gpu_qcomp operator * (gpu_qcomp a, const size_t& b) noexcept {
    a *= static_cast<qreal>(b);
    return a;
}


// reverse order of out-of-place mixed-type arithmetic (via commutation)
INLINE gpu_qcomp operator * (const int& a, const gpu_qcomp& b) noexcept {
    return b * a;
}
INLINE gpu_qcomp operator * (const qreal& a, const gpu_qcomp& b) noexcept {
    return b * a;
}


// no-op cast of pointers
INLINE gpu_qcomp* getGpuQcompPtr(qcomp* list) {

    return reinterpret_cast<gpu_qcomp*>(list);
}


// get gpu_qcomp from components
INLINE gpu_qcomp getGpuQcomp(qreal re, qreal im) {
    return { re, im };
}


// get gpu_qcomp from qcomp (host only; qcomp forbiddin in device code)
__host__ gpu_qcomp getGpuQcomp(const qcomp& a) {
    return { a.real(), a.imag() };
}


// get qcomp from gpu_qcomp (host only; qcomp forbiddin in device code)
__host__ qcomp getQcomp(const gpu_qcomp& a) {
    return qcomp( a.re, a.im );
}

    // // creator for fixed-size dense matrices (CompMatr1 and CompMatr2)
    // template <int dim>
    // INLINE std::array<std::array<cpu_qcomp,dim>,dim> getCpuQcomps(qcomp matr[dim][dim]) {

    //     std::array<std::array<cpu_qcomp,dim>,dim> out;

    //     for (int i=0; i<dim; i++)
    //         for (int j=0; j<dim; j++)
    //             out[i][j] = getCpuQcomp(matr[i][j]);

    //     return out;
    // }


// maths functions
INLINE qreal real(const gpu_qcomp& a) {
    return a.re;
}
INLINE qreal imag(const gpu_qcomp& a) {
    return a.im;
}
INLINE gpu_qcomp conj(const gpu_qcomp& a) {
    return {a.re, - a.im};
}
INLINE qreal norm(const gpu_qcomp& a) noexcept {
    return (a.re * a.re) + (a.im * a.im);
}
INLINE gpu_qcomp pow(gpu_qcomp base, gpu_qcomp exponent) {

    // using https://mathworld.wolfram.com/ComplexExponentiation.html,
    // and the principal argument of 'base'

    // base = a + b i, exponent = c + d i
    qreal a = base.re;
    qreal b = base.im;
    qreal c = exponent.re;
    qreal d = exponent.im;

    // intermediate quantities (uses CUDA atan2,log,pow,exp,cos,sin)
    qreal arg = atan2(b, a);
    qreal mag = a*a + b*b;
    qreal ln = log(mag);
    qreal fac = pow(mag, c/2) * exp(-d * arg);
    qreal ang = c*arg + d*ln/2;

    // output scalar
    qreal re = fac * cos(ang);
    qreal im = fac * sin(ang);
    return getGpuQcomp(re, im);
}


// check the memory layout of gpu_qcomp agrees with qcomp, since
// it is not formally gauranteed, unlike _Complex and std::complex
static_assert(sizeof (gpu_qcomp) == sizeof (qcomp));
static_assert(alignof(gpu_qcomp) == alignof(qcomp));
static_assert(std::is_standard_layout_v   <gpu_qcomp>);
static_assert(std::is_trivially_copyable_v<gpu_qcomp>);


// TODO:
// the above checks are potentially inadequate to identify an
// insidious incompatibility between qcomp and gpu_qcomp - perhaps
// we should perform a compile-time duck-check, casting a small
// array between them and checking no data is corrupted? Perhaps
// a runtime check in initQuESTEnv() is also necessary, checking the
// casting is safe for all circumstances (e.g. heap mem, static lists)




/*
 * TODO:
 * OLD UNPACKERS
 *
 * which I am hestitant to switch to the CPU-style until I better
 * understand why the explicit gpu_qcomp instantiation is necessary
 * (iirc static HIP structs have a different alignment than qcomp?!)
 */


__host__ inline std::array<gpu_qcomp,2> unpackMatrixToGpuQcomps(DiagMatr1 in) {

    // it's crucial we explicitly copy over the elements,
    // rather than just reinterpret the pointer, to avoid
    // segmentation faults when memory misaligns (like on HIP)

    // oh YES we must not cast statically created HIP arrays
    // like within kernels?!?!



        // UMMMMMM is the above true?!?!
        // Wen did I witness misalignment between std::complex and gpu_qcomp?!

    return {getGpuQcomp(in.elems[0]), getGpuQcomp(in.elems[1])};
}


__host__ inline std::array<gpu_qcomp,4> unpackMatrixToGpuQcomps(DiagMatr2 in) {

    return {
        getGpuQcomp(in.elems[0]), getGpuQcomp(in.elems[1]),
        getGpuQcomp(in.elems[2]), getGpuQcomp(in.elems[3])};
}


__host__ inline std::array<gpu_qcomp,4> unpackMatrixToGpuQcomps(CompMatr1 in) {

    std::array<gpu_qcomp,4> out{};
    for (int i=0; i<4; i++)
        out[i] = getGpuQcomp(in.elems[i/2][i%2]);

    return out;
}


__host__ inline std::array<gpu_qcomp,16> unpackMatrixToGpuQcomps(CompMatr2 in) {

    std::array<gpu_qcomp,16> out{};
    for (int i=0; i<16; i++)
        out[i] = getGpuQcomp(in.elems[i/4][i%4]);

    return out;
}









INLINE gpu_qcomp fast_getPauliStrElem(PauliStr str, qindex row, qindex col) {

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
    gpu_qcomp p0, p1,n1, pI,nI;
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
    gpu_qcomp matrices[][2][2] = {
        {{p1,p0},{p0,p1}},  // I
        {{p0,p1},{p1,p0}},  // X
        {{p0,nI},{pI,p0}},  // Y
        {{p1,p0},{p0,n1}}}; // Z

    gpu_qcomp elem = p1; // 1

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



INLINE gpu_qcomp fast_getPauliStrSumElem(gpu_qcomp* coeffs, PauliStr* strings, qindex numTerms, qindex row, qindex col) {

    // this function accepts unpacked PauliStrSum fields since a PauliStrSum cannot 
    // be directly processed in CUDA kernels/thrust due to its 'qcomp' field.
    // it also assumes str.highPaulis==0 for all str in strings, as per above func.

    gpu_qcomp elem = {0, 0}; // type-agnostic complex literal

    // this loop is expected exponentially smaller than caller's loop
    for (qindex n=0; n<numTerms; n++)
        elem += coeffs[n] * fast_getPauliStrElem(strings[n], row, col);

    return elem;
}



#endif // GPU_TYPES_HPP