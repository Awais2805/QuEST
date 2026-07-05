/** @file
 * API signatures for debugging QuEST behaviour, 
 * controlling input validation, changing reporter
 * parameters or seeding random generation.
 * 
 * @author Tyson Jones
 *
 * @defgroup debug Debug
 * @ingroup api
 * @brief Utilities for controlling QuEST behaviour such as seeding, input validation and printing.
 * @{
 */

#ifndef DEBUG_H
#define DEBUG_H

#include "quest/include/types.h"



/*
 * C AND C++ AGNOSTIC FUNCTIONS
 */

// enable invocation by both C and C++ binaries
#ifdef __cplusplus
extern "C" {
#endif



/** 
 * @defgroup debug_seed Seeding
 * @brief Functions for seeding QuEST's random generators.
 * @details Re-seeding with identical seeds will determine all of QuEST's subsequent 
 *          random outputs (such as measurement and random state preparation), and can
 *          be done at any stage of execution. When seeding is not explicitly performed,
 *          QuEST will attempt to use a cryptographically secure pseudorandom number generator
 *          (CSPRNG) if locally available, else fall back to a standard PRNG, via using
 *          the standard C++ `random_device` class.
 * @{
 */


/** Sets the seeds used by QuEST's random number generation.
 * 
 * This affects and determines all pseudorandom decisions made the QuEST library,
 * such as qubit measurement outcomes and random state preparation. The seeds are
 * passed without modification to QuEST's 19,937-bit
 * [Mersenne Twister](https://cplusplus.com/reference/random/mt19937_64/) generator.
 * As such, fully specifying the generator's initial state requires 19,937 seed bits, 
 * or 623 unsigned integers, although this is rarely necessary.
 * 
 * Seeding can be performed at any time before a random decision (such as measurement);
 * no random outcomes/state is stored on any object beforehand.
 *
 * > [!NOTE]
 * > In distributed simulation, only the root node's seeds are consulted (and broadcast
 * > to all nodes) while those passed on all other nodes are ignored.
 * 
 * @myexample
 * ```
    // make report() print only one trailing newline
    setQuESTNumReportedNewlines(1);

    // seed
    unsigned seeds[] = {123456789u, 987654321u, 546372819u};
    setQuESTSeeds(seeds, 2);

    // randomly collapse the plus state
    initPlusState(qureg);
    reportScalar("outcome 0", applyQubitMeasurement(qureg, 0));
    reportScalar("outcome 1", applyQubitMeasurement(qureg, 1));
    reportScalar("outcome 2", applyQubitMeasurement(qureg, 2));

    // restore RNG to state prior to collapse
    setQuESTSeeds(seeds, 2);

    // randomly collapse the plus state again, obtaining same outcomes
    reportStr("");
    initPlusState(qureg);
    reportScalar("outcome 0", applyQubitMeasurement(qureg, 0));
    reportScalar("outcome 1", applyQubitMeasurement(qureg, 1));
    reportScalar("outcome 2", applyQubitMeasurement(qureg, 2));
 * ```
 * Example output:
 * ```
    outcome 0: 0
    outcome 1: 1
    outcome 2: 1

    outcome 0: 0
    outcome 1: 1
    outcome 2: 1
 * ```
 * 
 * @param[in] seeds    a list of seeds.
 * @param[in] numSeeds the length of @p seeds.
 * @throws @validationerror
 * - if @p seeds is a null pointer.
 * - if @p numSeeds is less than one.
 * @see
 * - getQuESTSeeds()
 * - getQuESTNumSeeds()
 * - setQuESTSeedsToDefault()
 * @author Tyson Jones
 */
void setQuESTSeeds(unsigned* seeds, int numSeeds);


/** Re-randomizes QuEST, seeding its random number generator with new seeds as pseudorandomly
 * produced by the @c C++ [@c std::random_device](https://en.cppreference.com/cpp/numeric/random/random_device)
 * function, potentially using a hardware RNG.
 * 
 * Similar to setQuESTSeeds(), this affects all pseudorandom decisions made the QuEST library,
 * such as qubit measurement outcomes and random state preparation. Unlike setQuESTSeeds()
 * however, repeated calls will not guarantee identical random generation; the internally
 * processed seeds may differ at each invocation. This is, in fact, the function first
 * internally called during QuEST environment initialisation.
 * 
 * Seeding can be performed at any time after QuEST library initialisation, before a
 * random decision (such as measurement); no random outcomes/state is stored on any object beforehand.
 * 
 * This function consults @c std::random_device to produce @c DEFAULT_NUM_RNG_SEEDS=4
 * seeds, which is infact insufficient to fully specify an initial state of QuEST's
 * [19,937-bit  Mersenne Twister](https://cplusplus.com/reference/random/mt19937_64/)
 * generator. Presently, @c DEFAULT_NUM_RNG_SEEDS cannot be changed except through
 * manual modification and recompilation.
 *
 * > [!NOTE]
 * > In distributed simulation, only the root node's seeds are consulted (and broadcast
 * > to all nodes) while those passed on all other nodes are ignored.
 * 
 * @myexample
 * ```
    // make report() print only one trailing newline
    setQuESTNumReportedNewlines(1);

    // randomise the RNG state
    setQuESTSeedsToDefault();

    // randomly collapse the plus state
    initPlusState(qureg);
    reportScalar("outcome 0", applyQubitMeasurement(qureg, 0));
    reportScalar("outcome 1", applyQubitMeasurement(qureg, 1));
    reportScalar("outcome 2", applyQubitMeasurement(qureg, 2));

    // re-randomise the RNG state
    setQuESTSeedsToDefault();

    // randomly collapse the plus state again, obtaining new outcomes
    reportStr("");
    initPlusState(qureg);
    reportScalar("outcome 0", applyQubitMeasurement(qureg, 0));
    reportScalar("outcome 1", applyQubitMeasurement(qureg, 1));
    reportScalar("outcome 2", applyQubitMeasurement(qureg, 2));
 * ```
 * Example output:
 * ```
    outcome 0: 0
    outcome 1: 1
    outcome 2: 0

    outcome 0: 1
    outcome 1: 1
    outcome 2: 1
 * ```
 * We can view the random seeds chosen by QuEST using getQuESTSeeds():
 * ```
    int numSeeds = getQuESTNumSeeds(); // fixed=4
    unsigned seeds[1000]; // lazily/dangerously assume num<=1000
    
    setQuESTSeedsToDefault();
    getQuESTSeeds(seeds);

    for (int i=0; i<numSeeds; i++)
        printf("%u ", seeds[i]);
    printf("\n\n");

    setQuESTSeedsToDefault();
    getQuESTSeeds(seeds);
    
    for (int i=0; i<numSeeds; i++)
        printf("%u ", seeds[i]);
    printf("\n");
 * ```
 * Example output:
 * ```
    766738893 3449946700 2005286535 3261547242 

    3996404805 474611433 1261615115 3403708295 
 * ```
 * 
 * @throws @validationerror
 * - if the QuEST environment has not been initialised via initQuESTEnv().
 * @see
 * - setQuESTSeeds()
 * - getQuESTSeeds()
 * - getQuESTNumSeeds()
 * @author Tyson Jones
 */
void setQuESTSeedsToDefault();


/** Populates @p seeds with those which last seeded QuEST's random number generation.
 * 
 * This will return the last seeds passed to setQuESTSeeds(), or those internally
 * chosen within setQuESTSeedsToDefault() (and equivalently, those initially chosen
 * during the QuEST environment initialisation). The number of seeds, which informs
 * the necessary capacity of @p seeds, is obtained by getQuESTNumSeeds().
 * 
 * @myexample
 * 
 * ```
    int numSeeds = getQuESTNumSeeds();
    unsigned *seeds = malloc(numSeeds * sizeof *seeds);

    getQuESTSeeds(seeds);
    printf("seeds[%d] = { ", numSeeds);

    for (int i=0; i<numSeeds; i++)
        printf("%u ", seeds[i]);
    printf("} \n");

    free(seeds);
 * ```
 * may output
 * ```
    seeds[4] = { 3674477761 2499034318 2614242128 475980445 } 
 * ```
 * 
 * See setQuESTSeedsToDefault() for an example of how getQuESTSeeds() interacts with
 * other seeding calls.
 * 
 * @throws @validationerror
 * - if the QuEST environment has not been initialised via initQuESTEnv().
 * - if @p seeds is a nullptr.
 * @throws seg-fault
 * - if @p seeds does not have capacity to write as many @c unsigned as getQuESTNumSeeds() returns.
 * @see
 * - getQuESTNumSeeds()
 * - setQuESTSeeds()
 * @author Tyson Jones
 */
void getQuESTSeeds(unsigned* seeds);


/** Returns the number of seeds used by QuEST in its latest round of random number generator seeding.
 * 
 * This is the length of the list output by getQuESTSeeds().
 * 
 * @throws @validationerror
 * - if the QuEST environment has not been initialised via initQuESTEnv().
 * @see
 * - getQuESTSeeds()
 * - setQuESTSeeds()
 * @author Tyson Jones
 */
int getQuESTNumSeeds();


/** @} */



/** 
 * @defgroup debug_validation Validation
 * @brief Functions to control QuEST's user-input validation.
 * @details These can be used to adjust the precision with which properties like unitarity 
 *          are checked/enforced, or otherwise disable all input validation (e.g. is the
 *          given qubit index valid?). Note passing erroneous input while validation is 
 *          disabled can result in runtime errors like segmentation faults. 
 * @{
 */


/** @notyetdoced
 *
 * @see
 * - [C](https://github.com/QuEST-Kit/QuEST/blob/devel/examples/isolated/setting_errorhandler.c) and 
 *   [C++](https://github.com/QuEST-Kit/QuEST/blob/devel/examples/isolated/setting_errorhandler.cpp) examples
 */
void setQuESTInputErrorHandler(void (*callback)(const char* func, const char* msg));


/// @notyetdoced
void setQuESTValidationOn();


/// @notyetdoced
void setQuESTValidationOff();


/// @notyetdoced
void setQuESTValidationEpsilonToDefault();


/// @notyetdoced
void setQuESTValidationEpsilon(qreal eps);


/// @notyetdoced
qreal getQuESTValidationEpsilon();


/** @} */



/** 
 * @defgroup debug_reporting Reporting
 * @brief Functions to control how QuEST's reporters display and truncate information.
 * @{
 */


/// @notyetdoced
/// @notyettested
void setQuESTMaxNumReportedItems(qindex numRows, qindex numCols);


/** @notyetdoced
 * > This function does not affect the significant figures in printed memory sizes
 * > (e.g. `5.32 KiB`) which is always shown with three significant figures 
 * > (or four when in bytes, e.g. `1023 bytes`).
 */
void setQuESTMaxNumReportedSigFigs(int numSigFigs);


/// @notyetdoced
void setQuESTNumReportedNewlines(int numNewlines);


/** 
 * @notyetdoced
 * @notyettested
 * @myexample
 * ```
   PauliStr str = getInlinePauliStr("XYZ", {0,10,20});
   reportPauliStr(str);

   setQuESTReportedPauliChars(".xyz");
   reportPauliStr(str);
 * ```
 */
void setQuESTReportedPauliChars(const char* paulis);


/** 
 * @notyetdoced
 * @notyettested
 * @myexample
 * ```
   PauliStr str = getInlinePauliStr("XYZ", {0,10,20});

   setQuESTReportedPauliStrStyle(0);
   reportPauliStr(str);

   setQuESTReportedPauliStrStyle(1);
   reportPauliStr(str);
 * ```
 */
void setQuESTReportedPauliStrStyle(int style);


/** @} */



/** 
 * @defgroup debug_cache Caching
 * @brief Functions to control temporary memory used by the QuEST process.
 * @{
 */


/// @notyetdoced
qindex getQuESTGpuCacheSize();


/// @notyetdoced
void clearQuESTGpuCache();


/** @} */



/** 
 * @defgroup debug_info Info
 * @brief Functions for getting debugging information.
 * @{
 */


/// @notyetdoced
/// @notyettested
void getQuESTEnvironmentString(char str[200]);


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


/// @ingroup debug_seed
/// @notyettested
/// @notyetdoced
/// @cppvectoroverload
/// @see setQuESTSeeds()
void setQuESTSeeds(std::vector<unsigned> seeds);


/// @ingroup debug_seed
/// @notyettested
/// @notyetdoced
/// @cpponly
/// @see getQuESTSeeds()
std::vector<unsigned> getQuESTSeeds();


#endif // __cplusplus



#endif // DEBUG_H

/** @} */ // (end file-wide doxygen defgroup)
