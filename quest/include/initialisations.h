/** @file
 * API signatures for initialising Quregs into 
 * particular states. Note when a Qureg is GPU-
 * accelerated, these functions only update the
 * state in GPU memory; the CPU amps are unchanged.
 * 
 * @author Tyson Jones
 * 
 * @defgroup initialisations Initialisations
 * @ingroup api
 * @brief Functions for preparing Quregs in particular states.
 * @{
 */

#ifndef INITIALISATIONS_H
#define INITIALISATIONS_H

#include "quest/include/types.h"
#include "quest/include/qureg.h"
#include "quest/include/paulis.h"



/*
 * C AND C++ AGNOSTIC FUNCTIONS
 */

// enable invocation by both C and C++ binaries
#ifdef __cplusplus
extern "C" {
#endif



/** 
 * @defgroup init_states States
 * @brief Functions for initialising Qureg into physical states.
 * @{
 */


/** Initialises @p qureg to the unnormalised all-zero-amplitude state.
 *
 * Every statevector amplitude, or every density-matrix element, is set to zero.
 * This is not a physical quantum state, but is useful as a blank
 * workspace before manually setting amplitudes.
 * 
 * @equivalences
 * 
 * - This function is equivalent to (but much faster than) overwriting every
 *   amplitude to zero.
 *   ```cpp
     for (int i=0; i<qureg.numAmpsPerNode; i++)
         qureg.cpuAmps[i] = 0;
     syncQureg(qureg);
 *   ```
 *
 * @param[in,out] qureg  the Qureg to overwrite.
 * @throws @validationerror
 * - if @p qureg is uninitialised.
 * @author Tyson Jones
 */
void initBlankState(Qureg qureg);


/** Initialises @p qureg to the zero computational basis state.
 * 
 * @formulae
 *
 * Let @f$N@f$ be the number of qubits in @p qureg. 
 * 
 * - If @p qureg is a statevector, it is initialised to @f$\ket{0}^{\otimes N}@f$.
 * - If @p qureg is a density matrix, it is initialised to @f$\ket{0}\bra{0}^{\otimes N}@f$.'
 *
 * @equivalences
 * 
 * - The zero state is the first enumerated classical state.
 *   ```cpp
     initClassicalState(qureg, 0);
 *   ```
 * - The zero state has a zero amplitude everywhere except at the first index, which has one.
 *   ```cpp
     initBlankState(qureg);
     if (qureg.rank == 0)
         qureg.cpuAmps[0] = 1;
     syncQureg(qureg);
 *   ```
 *
 * @param[in,out] qureg  the Qureg to overwrite.
 * @throws @validationerror
 * - if @p qureg is uninitialised.
 * @author Tyson Jones
 */
void initZeroState(Qureg qureg);


/** Initialises @p qureg to the uniform plus state.
 * 
 * @formulae
 *
 * Let @f$N@f$ be the number of qubits in @p qureg. 
 * 
 * - If @p qureg is a statevector, it is initialised to
 *   @f[
        \begin{aligned}
        \ket{+}^{\otimes N} &= \left( \frac{1}{\sqrt{2}} \ket{0} + \frac{1}{\sqrt{2}} \ket{1} \right)^{\otimes N} \\
                            &= \frac{1}{\sqrt{2^N}} \{ 1, 1, \dots, 1 \}
        \end{aligned}
 *   @f]
 * - If @p qureg is a density matrix, it is initialised to 
 *   @f[
        \ket{+}\bra{+}^{\otimes N} = \frac{1}{2^N}
            \begin{pmatrix} 
            1 & 1 & \dots \\ 1 & \ddots \\ \vdots 
            \end{pmatrix}
 *   @f]
 *
 * @equivalences
 * 
 * - The plus state can also be produced by applying a Hadamard gate upon every zero-state qubit.
 *   ```cpp
     initZeroState(qureg);
     for (int i=0; i<qureg.numQubits; i++)
        applyHadamard(qureg, i);
 *   ``` 
 *
 * @param[in,out] qureg  the Qureg to overwrite.
 * @throws @validationerror
 * - if @p qureg is uninitialised.
 * @author Tyson Jones
 */
void initPlusState(Qureg qureg);


/** Initialises @p qureg to the state in statevector @p pure.
 * 
 * @formulae
 * 
 * Let @f$N@f$ be the number of qubits in @p qureg or @p pure, and let @f$\ket{\psi} = @f$ @p pure,
 * with amplitudes @f$\ket{\psi} = \sum_i \alpha_i \ket{i}@f$.
 * 
 * - If @p qureg is a statevector, it is overwritten by the state in @p pure.
 * - If @p qureg is a density matrix, it is initialised to 
 *   @f[
        \ket{\psi}\bra{\psi} = 
            \sum\limits_i\sum\limits_j \alpha_i \,\alpha_j^* \, \ket{i}\bra{j}
 *   @f]
 *
 * @equivalences
 * 
 * - When @p qureg is a statevector, this function is entirely equivalent to
 *   ```cpp
     setQuregToClone(qureg, pure);
 *   ``` 
 * - When @p qureg is a density matrix, this function is equivalent to
 *   ```cpp
     double prob = 1;
     setQuregToMixture(qureg, &prob, &pure, 1);
 *   ```
 *
 * @param[in,out] qureg  the Qureg to overwrite.
 * @param[in]     pure   the statevector pure state to copy.
 * @throws @validationerror
 * - if @p qureg or @p pure are uninitialised.
 * - if @p pure is not a statevector.
 * - if @p qureg and @p pure have incompatible dimensions or deployments.
 * @author Tyson Jones
 */
void initPureState(Qureg qureg, Qureg pure);


/** Initialises @p qureg to a computational basis state.
 * 
 * @formulae
 * 
 * Let @f$N@f$ be the number of qubits in @p qureg, and let @f$i=@f$ @p stateInd.
 * 
 * States are enumerated from @f$0@f$ to @f$2^N-1@f$, such that the bits of the indices
 * match the qubits of the corresponding basis states.
 * 
 * - If @p qureg is a statevector, it is initialised to @f$\ket{i}@f$.
 * - If @p qureg is a density matrix, it is initialised to @f$\ket{i}\bra{i}@f$.
 *
 * The bits of @f$i@f$ will match the qubit values of the resulting state in @p qureg, 
 *  where the zero-th qubit is the rightmost bit.
 *
 * @equivalences
 * 
 * - The resulting state contains zero for all amplitudes except that at index @f$i@f$
 *   (when @p qureg is a statevector) or the @f$i@f$-th diagonal (when @p qureg is a
 *   density matrix).
 *   ```cpp
     // when non-distributed, for simplicity
     initBlankState(qureg);
     qindex d = 1 + (1 << qureg.numQubits);
     qindex i = stateInd * (qureg.isDensityMatrix? d : 1);
     qureg.cpuAmps[i] = 1;
     syncQureg(qureg);
 *   ```
 * - The resulting state can be (pointlessly slowly) produced by qubit flips from the
 *   zero state, according to the bits in @p stateInd.
 *   ```cpp
     initZeroState(qureg);
     for (int i=0; i<qureg.numQubits; i++)
         if ((stateInd >> i) & 1)
            applyPauliX(qureg, i);
 *   ```
 *
 * @param[in,out] qureg     the Qureg to overwrite.
 * @param[in]     stateInd  the computational basis-state index.
 * @throws @validationerror
 * - if @p qureg is uninitialised.
 * - if @p stateInd is outside the computational basis of @p qureg.
 * @author Tyson Jones
 */
void initClassicalState(Qureg qureg, qindex stateInd);


/** Initialises @p qureg to the debug state.
 *
 * This is a non-physical, deterministic pattern useful for debugging.
 * The @f$j@f$-th local amplitude becomes 
 * @f[
    2j/10 + \iu(2j+1)/10,
 * @f]
 * even if @p qureg is a density matrix, in which case it is enumerated
 * column-major.
 
 * @myexample
 * 
 * ```cpp
   Qureg qureg = createQureg(3);
   initDebugState(qureg);
   reportQureg(qureg);
 * ```
 * ```text
    Qureg (3 qubit statevector, 8 qcomps, 232 bytes):
        0.1i      |0⟩
        0.2+0.3i  |1⟩
        0.4+0.5i  |2⟩
        0.6+0.7i  |3⟩
        0.8+0.9i  |4⟩
        1+1.1i    |5⟩
        1.2+1.3i  |6⟩
        1.4+1.5i  |7⟩
 * ```
 *
 * @param[in,out] qureg  the Qureg to overwrite.
 * @throws @validationerror
 * - if @p qureg is uninitialised.
 * @author Tyson Jones
 */
void initDebugState(Qureg qureg);


/** Initialises @p qureg from the statevector amplitudes in @p amps.
 * 
 * @formulae
 * 
 * Let @f$N@f$ be the number of qubits in @p qureg, and let @f$\alpha_i@f$ be
 * the amplitude `amps[i]`. Array @p amps must be length @f$2^N@f$.
 * 
 * - If @p qureg is a statevector, its amplitudes are overwritten by @p amps, to become
 *   @f[
        \sum\limits_i \alpha_i \ket{i}.
 *   @f]
 * 
 * - If @p qureg is a density matrix, it is initialised to the pure state @f$\ket{\psi}@f$ 
 *   encoded by @p amps, i.e.
 *   @f[
        \ket{\psi}\bra{\psi} = 
            \sum\limits_i\sum\limits_j \alpha_i \,\alpha_j^* \, \ket{i}\bra{j}
 *   @f]
 *
 * There is no need for @p amps to be normalised, although @p qureg will otherwise be left
 * in an unnormalised, non-physical state.
 *
 * @param[in,out] qureg  the Qureg to overwrite.
 * @param[in]     amps   an array of @f$2^N@f$ pure-state amplitudes.
 * @throws @validationerror
 * - if @p qureg is uninitialised.
 * @throws seg-fault
 * - if @p amps has fewer than @f$2^N@f$ elements.
 * @author Tyson Jones
 */
void initArbitraryPureState(Qureg qureg, qcomp* amps);


/** Initialises @p qureg (a statevector or density matrix) to a pure state with 
 * uniformly random amplitudes.
 * 
 * The resulting state is normalised, with basis state probabilities sampled
 * from a chi-squared variate, as described
 * [here](https://sumeetkhatri.com/wp-content/uploads/2020/05/random_pure_states.pdf). 
 * 
 * @param[in,out] qureg  the Qureg to overwrite.
 * @throws @validationerror
 * - if @p qureg is uninitialised.
 * @see 
 * - initRandomMixedState()
 * @author Tyson Jones
 */
void initRandomPureState(Qureg qureg);


/** Initialises a density matrix to a mixture of uniformly random pure states.
 *
 * The resulting density matrix is the equally weighted mixture of @p numPureStates
 * independently sampled random pure states, each sampled as per initRandomPureState().
 *
 * @formulae
 * 
 * Let @f$n=@f$ @p numPureStates, and let @f$\ket{\psi_i}@f$ be a random pure
 * state with number of qubits as @p qureg.
 * 
 * This function overwrites @p qureg to
 * @f[
 *      \sum\limits_i^n \frac{1}{n} \ket{\psi_i}\bra{\psi_i}.
 * @f]
 *
 * @param[in,out] qureg          the density matrix to overwrite.
 * @param[in]     numPureStates  the number of random pure states in the mixture.
 * @throws @validationerror
 * - if @p qureg is uninitialised.
 * - if @p qureg is not a density matrix.
 * - if @p numPureStates is invalid.
 * @see 
 * - initRandomPureState()
 * @author Tyson Jones
 */
void initRandomMixedState(Qureg qureg, qindex numPureStates);


/** @} */



/** 
 * @defgroup init_amps Amplitudes
 * @brief Functions for overwriting Qureg amplitudes.
 * @{
 */


/// @notyetdoced
/// @notyetvalidated
void setQuregAmps(Qureg qureg, qindex startInd, qcomp* amps, qindex numAmps);


/// @notyetdoced
/// @notyetvalidated
void setDensityQuregAmps(Qureg qureg, qindex startRow, qindex startCol, qcomp** amps, qindex numRows, qindex numCols);


/// @notyetdoced
/// @notyetvalidated
void setDensityQuregFlatAmps(Qureg qureg, qindex startInd, qcomp* amps, qindex numAmps);


/// @notyetdoced
/// @notyettested
void setQuregToClone(Qureg outQureg, Qureg inQureg);


/// @notyetdoced
/// @notyettested
void setQuregToWeightedSum(Qureg out, qcomp* coeffs, Qureg* in, int numIn);


/// @notyetdoced
/// @notyettested
void setQuregToMixture(Qureg out, qreal* probs, Qureg* in, int numIn);


/// @notyetdoced
/// @notyetvalidated
qreal setQuregToRenormalized(Qureg qureg);


/// @notyetdoced
/// @notyetvalidated
void setQuregToPauliStrSum(Qureg qureg, PauliStrSum sum);


/// @notyetdoced
/// @notyettested
void setQuregToPartialTrace(Qureg out, Qureg in, int* traceOutQubits, int numTraceQubits);


/// @notyetdoced
/// @notyettested
void setQuregToReducedDensityMatrix(Qureg out, Qureg in, int* retainQubits, int numRetainQubits);


/** @} */



// end de-mangler
#ifdef __cplusplus
}
#endif



/*
 * C++ OVERLOADS
 *
 * which are only accessible to C++ binaries, and accept
 * arguments more natural to C++ (e.g. std::vector). We 
 * manually add these to their respective Doxygen doc groups.
 */

#ifdef __cplusplus

#include <vector>


/// @ingroup init_amps
/// @notyettested
/// @notyetdoced
/// @notyetvalidated
/// @cpponly
/// @see setQuregAmps()
void setQuregAmps(Qureg qureg, qindex startInd, std::vector<qcomp> amps);


/// @ingroup init_amps
/// @notyettested
/// @notyetdoced
/// @notyetvalidated
/// @cpponly
/// @see setDensityQuregAmps()
void setDensityQuregAmps(Qureg qureg, qindex startRow, qindex startCol, std::vector<std::vector<qcomp>> amps);


/// @ingroup init_amps
/// @notyettested
/// @notyetdoced
/// @notyetvalidated
/// @cpponly
/// @see setDensityQuregFlatAmps()
void setDensityQuregFlatAmps(Qureg qureg, qindex startInd, std::vector<qcomp> amps);


/// @ingroup init_amps
/// @notyettested
/// @notyetdoced
/// @notyetvalidated
/// @cpponly
/// @see setQuregToPartialTrace()
void setQuregToPartialTrace(Qureg out, Qureg in, std::vector<int> traceOutQubits);


/// @ingroup init_amps
/// @notyettested
/// @notyetdoced
/// @notyetvalidated
/// @cpponly
/// @see setQuregToReducedDensityMatrix()
void setQuregToReducedDensityMatrix(Qureg out, Qureg in, std::vector<int> retainQubits);


/// @ingroup init_amps
/// @notyetdoced
/// @cpponly
/// @see setQuregToWeightedSum()
void setQuregToWeightedSum(Qureg out, std::vector<qcomp> coeffs, std::vector<Qureg> in);


/// @ingroup init_amps
/// @notyetdoced
/// @cpponly
/// @see setQuregToMixture()
void setQuregToMixture(Qureg out, std::vector<qreal> probs, std::vector<Qureg> in);


#endif // __cplusplus



#endif // INITIALISATIONS_H

/** @} */ // (end file-wide doxygen defgroup)
