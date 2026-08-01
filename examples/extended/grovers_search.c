/** @file
 * Implements Grover's algorithm for unstructured search,
 * using only X, H and multi-controlled Z gates
 *
 * Originally written by Tyson Jones against the QuEST v3 API. Ported to
 * the v4 API and extended to 20 qubits as a learning exercise; the
 * algorithm itself is unchanged.
 *
 * @author Tyson Jones (original)
 * @author Awais Rafique (v4 port)
 */

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include "quest.h"



/* effect |solElem> -> -|solElem> via a
 * multi-controlled phase flip gate
 */
void applyOracle(Qureg qureg, int numQubits, int solElem) {

    // apply X to transform |111> into |solElem>
    for (int q=0; q<numQubits; q++)
        if (((solElem >> q) & 1) == 0)
            applyPauliX(qureg, q);

    // effect |111> -> -|111>
    int ctrls[numQubits];
    for (int q=0; q<numQubits; q++)
        ctrls[q] = q;
    applyMultiQubitPhaseFlip(qureg, ctrls, numQubits);

    // apply X to transform |solElem> into |111>
    for (int q=0; q<numQubits; q++)
        if (((solElem >> q) & 1) == 0)
            applyPauliX(qureg, q);
}



/* apply 2|+><+|-I by transforming into the Hadamard basis
 * and effecting 2|0><0|-I. We do this, by observing that
 *   c..cZ = diag{1,..,1,-1}
 *         = I - 2|1..1><1..1|
 * and hence
 *   X..X c..cZ X..X = I - 2|0..0><0..0|
 * which differs from the desired 2|0><0|-I state only by
 * the irrelevant global phase pi
 */
void applyDiffuser(Qureg qureg, int numQubits) {

    // apply H to transform |+> into |0>
    for (int q=0; q<numQubits; q++)
        applyHadamard(qureg, q);

    // apply X to transform |11..1> into |00..0>
    for (int q=0; q<numQubits; q++)
        applyPauliX(qureg, q);

    // effect |11..1> -> -|11..1>
    int ctrls[numQubits];
    for (int q=0; q<numQubits; q++)
        ctrls[q] = q;
    applyMultiQubitPhaseFlip(qureg, ctrls, numQubits);

    // apply X to transform |00..0> into |11..1>
    for (int q=0; q<numQubits; q++)
        applyPauliX(qureg, q);

    // apply H to transform |0> into |+>
    for (int q=0; q<numQubits; q++)
        applyHadamard(qureg, q);
}



int main() {

    // prepare the hardware-agnostic QuEST environment
    initQuESTEnv();

    // choose the system size
    int numQubits = 20;
    int numElems = (int) pow(2, numQubits);
    int numReps = ceil(M_PI/4 * sqrt(numElems));

    printf("numQubits: %d, numElems: %d, numReps: %d\n",
        numQubits, numElems, numReps);

    // randomly choose the element for which to search
    srand(time(NULL));
    int solElem = rand() % numElems;

    // prepare |+>
    Qureg qureg = createQureg(numQubits);
    initPlusState(qureg);

    // apply Grover's algorithm
    for (int r=0; r<numReps; r++) {
        applyOracle(qureg, numQubits, solElem);
        applyDiffuser(qureg, numQubits);

        // monitor the probability of the solution state
        printf("Iteration %d: prob of solution |%d> = " QREAL_FORMAT_SPECIFIER "\n",
            r, solElem, calcProbOfBasisState(qureg, solElem));
    }

    // free memory
    destroyQureg(qureg);
    finalizeQuESTEnv();
    return 0;
}
