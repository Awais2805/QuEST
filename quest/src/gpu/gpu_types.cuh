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


// get gpu_qcomp from qcomp
INLINE gpu_qcomp getGpuQcomp(const qcomp& a) {
    return { a.real(), a.imag() };
}


// get qcomp from gpu_qcomp
INLINE qcomp getQcomp(const gpu_qcomp& a) {
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


#endif // GPU_TYPES_HPP