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
 * @param[out] seeds the list of seeds
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
 * @returns the number of seeds.
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


/** Sets the function which QuEST will call when encountering an invalid input.
 * 
 * By default, when a user passes an invalid input to QuEST (such as a negative qubit index),
 * an internal function @c default_inputErrorHandler() is called which prints a message to
 * @c stdout, attempts to gracefully clean up communication in distributed settings, then
 * exits execution with @c exit(EXIT_FAILURE). If this is undesired, setQuESTInputErrorHandler()
 * allows the user to substitute @p callback for the default handler, which will receive the
 * throwing API function name @p func, and the error message string @p msg.
 * 
 * QuEST endeavours to perform input validation upfront before proceeding to any mutation
 * of passed objects (like a Qureg). This permits gracefully catching validation errors through
 * custom handling without corrupting QuEST's state. For example, @c C++ users may wish 
 * to throw an exception within @p callback, or MPI superusers may wish to perform custom 
 * communicator cleanup before exiting.
 * 
 * > [!IMPORTANT]
 * > It is crucial that @p callback does not return execution back to the throwing
 * > QuEST function, which is likely to cause a segmentation fault or other internal
 * > error. Instead, @p callback should exit or throw an exception, caught by the
 * > user's control flow.
 * 
 * Note validation can be changed or disabled with setQuESTValidationEpsilon() and
 * setQuESTValidationOff(), which affects when @p callback will be called. This function
 * can be called at any time to update the error handler.
 * 
 * @myexample
 * 
 * ```
    void myErrorHandler(const char* errFunc, const char* errMsg) {
        printf("Ruh-roh, Raggy! Function '%s' has reported '%s'.\n", errFunc, errMsg);
        printf("We will now be very good children and exit immediately!\n");
        exit(0);
    }

    int main() {
        initQuESTEnv();
        setInputErrorHandler(myErrorHandler);
        createQureg(9999); // invokes myErrorHandler
        ...
    }
 * ```
 *
 * @param[in] callback a pointer to a function which accepts two `const char*` arguments.
 * @throws @validationerror
 * - if the QuEST environment has not been initialised via initQuESTEnv().
 * @throws seg-fault
 * - if @p callback is a null-ptr and an invalid input is later encountered.
 * @see
 * - [C](https://github.com/QuEST-Kit/QuEST/blob/main/examples/isolated/setting_errorhandler.c) and 
 *   [C++](https://github.com/QuEST-Kit/QuEST/blob/main/examples/isolated/setting_errorhandler.cpp) examples
 * - setQuESTValidationOff()
 * @author Tyson Jones
 */
void setQuESTInputErrorHandler(void (*callback)(const char* func, const char* msg));


/** Restores QuEST's input validation.
 * 
 * This means that invalid inputs to other QuEST API functions will call the error handler
 * (the default, or one passed to setQuESTInputErrorHandler()), rather than be silently ignored.
 * 
 * This function only has an affect if setQuESTValidationOff() was prior called. This function
 * does not affect the validation epsilon controlled with setQuESTValidationEpsilon(), which when
 * zero, will still see the skipping of numerically-approximated validations (such as unitarity checks).
 * 
 * @see
 * - setQuESTValidationOff()
 * @author Tyson Jones
 */
void setQuESTValidationOn();


/** Disables all of QuEST's input validation.
 * 
 * When a QuEST API function encounters an invalid input, the error will be ignored and execution of the
 * function will proceed. This is useful in order to call a QuEST function in a manner which is ordinarily
 * forbidden to avoid user mistakes. 
 * 
 * > [!IMPORTANT]
 * > This function disables all of QuEST's runtime input validation, meaning invalid inputs such as
 * > negative qubit indices will be accepted and trusted, likely causing internal errors and segmentation faults.
 * 
 * Users wishing only to adjust numerical validation tolerances, or disable numerical validations such as
 * matrix unitarity checks, should instead use the safer setQuESTValidationEpsilon(). If it is essential to
 * disable all validation, it should be later restored with setQuESTValidationOn().
 * 
 * @myexample
 *
 * ```
    Qureg qureg = createDensityQureg(3); // ~ 6-qubit statevector
    CompMatr1 matr = getInlineCompMatr1({{1,2},{3,4}});

    int target = 4; // 0-2 valid, 3-5 hacky, 6+ seg-fault

    setQuESTValidationOff();
    leftapplyCompMatr1(qureg, target, matr);
    setQuESTValidationOn();
 * ```
 * 
 * @see
 * - setQuESTValidationOn()
 * - setQuESTValidationEpsilon()
 * - setQuESTInputErrorHandler()
 * @author Tyson Jones
 */
void setQuESTValidationOff();


/** Restores QuEST's validation threshold for testing numerical or approximate quantities, to its default value.
 * 
 * The default value is informed by the environment variable `QUEST_DEFAULT_VALIDATION_EPSILON`. If the
 * environment variable was not specified during the launch of the QuEST executable, then the default
 * validation epsilon is specific to the precision of `qreal`, as controlled by `QUEST_FLOAT_PRECISION`.
 * These are:
 * | @c QUEST_FLOAT_PRECISION | @c qreal  | default epsilon |
 * |--------------------------|-----------|-----------------|
 * | 1                        | @c float  | @c 1E-5         |
 * | 2                        | @c double | @c 1E-12        |
 * | 4                        | @c long @c double | @c 1E-15 |
 *
 * @see
 * - setQuESTValidationEpsilon()
 * @author Tyson Jones
 */
void setQuESTValidationEpsilonToDefault();


/** Modifies QuEST's validation threshold for testing numerical or approximate quantities, to @p eps.
 * 
 * Many of QuEST's API functions validate that expected numerical properties of the input are satisfied.
 * For example, that the matrix passed to applyCompMatr1() is unitary, and ergo that the
 * product of the matrix with its own adjoint produces the identity matrix. Due to floating-point error,
 * such properties cannot be evaluated exactly, and small disagreement between the expected and given
 * property is tolerated. This difference is the validation epsilon, as overridden by this function.
 * Precisely how the validation epsilon is used by numerical validation is function specific, and
 * individually documented.
 * 
 * > [!TIP]
 * > Passing @p eps=0 effectively encodes @p eps=infinity, and *disables* all numerical validation.
 * 
 * The validation epsilon has no effect on non-numerical validation, such as whether qubit incices
 * are valid. In general, it is therefore safe to modify and disable numerical validation via this
 * function.
 * 
 * The default validation epsilon, which can itself be controlled by the `QUEST_DEFAULT_VALIDATION_EPSILON`
 * environment variable, is restored via setQuESTValidationEpsilonToDefault().
 * 
 * @myexample
 * 
 * ```
    // | max [matr . adj(matr) - identity] |^2 = 576
    CompMatr1 matr = getInlineCompMatr1({{1,2},{3,4}}); // non-unitary
    setQuESTValidationEpsilon(576.0);
    applyCompMatr1(qureg, 0, matr); // no error

    matr.elems[0][0] = 999;
    setQuESTValidationEpsilon(0); // disable all numerical validation
    applyCompMatr1(qureg, 0, matr); // no error

    // target=-1 would still trigger an error
    // applyCompMatr1(qureg, -1, matr);
 * ```
 * 
 * @param[in] eps the new validation epsilon.
 * @throws @validationerror
 * - if the QuEST environment has not been initialised via initQuESTEnv().
 * - if @p eps is negative.
 * @see
 * - setQuESTValidationEpsilonToDefault()
 * - getQuESTValidationEpsilon()
 * - setQuESTValidationOff()
 * @author Tyson Jones
 */
void setQuESTValidationEpsilon(qreal eps);


/** Returns the threshold used by QuEST's numerical validation.
 * 
 * This is the value last passed to setQuESTValidationEpsilon(), unless overridden by
 * setQuESTValidationEpsilonToDefault(), or similarly if never called. It indicates the
 * precision and correctness demanded of input numerical quantities to the QuEST API. A larger
 * epsilon corresponds to more permissive validation, while a smaller epsilon means validation
 * is harder to pass and input quantities must be more carefully prepared. 
 * 
 * > [!NOTE]
 * > A validation epsilon of @c 0 indicates numerical validation is disabled. 
 * 
 * The exact usage of the validation epsilon is function specific. For an example, see applyCompMatr1().
 * The validation has no effect when validation has been disabled entirely via setQuESTValidationOff().
 * 
 * @returns The validation epsilon.
 * @see
 * - setQuESTValidationEpsilon()
 * - setQuESTValidationEpsilonToDefault()
 * - setQuESTValidationOff()
 * @author Tyson Jones
 */
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
