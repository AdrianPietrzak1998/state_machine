/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Author: Adrian Pietrzak
 * GitHub: https://github.com/AdrianPietrzak1998
 * Created: May 6, 2025
 */

#ifndef SM_CORE_SM_H_
#define SM_CORE_SM_H_

#include <limits.h>
#include <stdint.h>

/**
 * @brief Selects the time source for the state machine tick.
 *
 * If set to 1, the state machine uses a user-provided function to retrieve the current time.
 * If set to 0, the state machine uses a pointer to a global time variable.
 */
#define SM_TICK_FROM_FUNC 0

/**
 * @brief Time base configuration for the state machine module.
 *
 * By default, the system tick type is `uint32_t`. To use a custom type, define `SM_TIME_BASE_TYPE_CUSTOM`
 * as the desired type (e.g. `uint16_t`, `int64_t`, etc.) and define the appropriate `_IS_*` macro
 * (e.g. `SM_TIME_BASE_TYPE_CUSTOM_IS_UINT16`) to allow the code to determine `SM_MAX_TIMEOUT`.
 */
#ifndef SM_TIME_BASE_TYPE_CUSTOM

#define SM_MAX_TIMEOUT UINT32_MAX
typedef volatile uint32_t SM_TIME_t;

#else

typedef SM_TIME_BASE_TYPE_CUSTOM SM_TIME_t;

#if defined(SM_TIME_BASE_TYPE_CUSTOM_IS_UINT8)
#define SM_MAX_TIMEOUT UINT8_MAX
#elif defined(SM_TIME_BASE_TYPE_CUSTOM_IS_UINT16)
#define SM_MAX_TIMEOUT UINT16_MAX
#elif defined(SM_TIME_BASE_TYPE_CUSTOM_IS_UINT32)
#define SM_MAX_TIMEOUT UINT32_MAX
#elif defined(SM_TIME_BASE_TYPE_CUSTOM_IS_UINT64)
#define SM_MAX_TIMEOUT UINT64_MAX
#elif defined(SM_TIME_BASE_TYPE_CUSTOM_IS_INT8)
#define SM_MAX_TIMEOUT INT8_MAX
#elif defined(SM_TIME_BASE_TYPE_CUSTOM_IS_INT16)
#define SM_MAX_TIMEOUT INT16_MAX
#elif defined(SM_TIME_BASE_TYPE_CUSTOM_IS_INT32)
#define SM_MAX_TIMEOUT INT32_MAX
#elif defined(SM_TIME_BASE_TYPE_CUSTOM_IS_INT64)
#define SM_MAX_TIMEOUT INT64_MAX
#else
#error "SM_MAX_TIMEOUT: Unknown SM_TIME_BASE_TYPE_CUSTOM or missing _IS_* define"
#endif

#endif

/**
 * @brief Enumeration of status codes returned by state machine operations.
 *
 * This enum defines the possible return values for state machine operations:
 *
 * - SM_OK: Operation completed successfully.
 * - SM_OPRT_INSTANCE_DOES_NOT_EXIST: The specified instance does not exist.
 * - SM_INIT_ERR: Error during initialization of the state machine.
 * - SM_EXEC_DELAYED: Execution was delayed due to an active delay timer.
 * - SM_EXEC_NULL_PTR: A null pointer was passed where a valid pointer was required.
 * - SM_TRANS_ERR: Transition error due to invalid configuration or state.
 * - SM_TRANS_LOCKED: Transition is locked and cannot be performed.
 * - SM_WRONG_STATE: Current state does not match the expected one.
 */
typedef enum
{
    SM_OK = 0,
    SM_OPRT_INSTANCE_DOES_NOT_EXIST,
    SM_INIT_ERR,
    SM_EXEC_DELAYED,
    SM_EXEC_NULL_PTR,
    SM_TRANS_ERR,
    SM_TRANS_LOCKED,
    SM_WRONG_STATE
} SM_operate_status;

/**
 * @brief State transition mode used when switching between states.
 *
 * This enumeration defines how entry and exit callbacks should be handled
 * during a state transition.
 *
 * - SM_TRANS_ENTRY_EXIT: Call both exit function of the current state and entry function of the next state.
 * - SM_TRANS_ENTRY: Call only the entry function of the next state.
 * - SM_TRANS_EXIT: Call only the exit function of the current state.
 * - SM_TRANS_FAST: Perform transition without calling entry or exit functions (fast transition).
 */
typedef enum
{
    SM_TRANS_ENTRY_EXIT = 0,
    SM_TRANS_ENTRY,
    SM_TRANS_EXIT,
    SM_TRANS_FAST
} SM_transision_mode;

typedef struct SM_instance_t SM_instance_t;

/**
 * @brief Structure holding all time-related variables used by the state machine.
 *
 * This structure encapsulates all timing fields necessary for managing
 * delayed executions, transition locks, and timeouts within the state machine.
 * It is intended for internal use and automatically managed by the state machine engine.
 */
typedef struct
{
    SM_TIME_t TransTick;
    SM_TIME_t lastExecTick;
    SM_TIME_t ExecBlockTick;
    SM_TIME_t TransLockTick;
    SM_TIME_t DelayTime;
    SM_TIME_t ExecBlockTimeout;
    SM_TIME_t TransLockTimeout;
} SM_timestamp_t;

/**
 * @brief Structure representing a single state in the state machine.
 *
 * This structure defines the behavior of a state through three optional callbacks:
 * entry, execution, and exit functions. Each function can be independently set
 * to implement the desired behavior when the state is entered, executed cyclically,
 * or exited.
 */
typedef struct
{
    void (*onEntry)(SM_instance_t *me);
    void (*onExec)(SM_instance_t *me);
    void (*onExit)(SM_instance_t *me);
} SM_state_t;

/**
 * @brief Bitfield structure containing internal control flags for the state machine.
 *
 * These flags are used to control execution flow within the state machine,
 * such as breaking execution or temporarily locking state transitions.
 */
typedef struct
{
    unsigned int ExecBreak : 1;
    unsigned int TransitionLock : 1;
} SM_control_flags_t;

/**
 * @brief Structure for collecting runtime statistics of the state machine.
 *
 * This structure holds counters related to state execution, machine operation,
 * and the number of performed transitions.
 */
typedef struct
{
    uint32_t StateExecutionCounter;
    uint32_t MachineExecutionCounter;
    uint32_t TransCounter;
} SM_stats_t;

/**
 * @brief Structure representing a single instance of the state machine.
 *
 * This structure encapsulates all necessary state, control, and timing information for a state machine instance.
 *
 * - `SM_states`        – Pointer to the array describing all possible states of the machine.
 * - `ActualState`      – Pointer to the currently active state.
 * - `PrevState`        – Pointer to the previously active state.
 * - `NumberOfStates`   – Total number of defined states in the state table.
 * - `Time`             – Structure containing timing-related variables for delay and timeout handling.
 * - `SM_control_flags` – Bitfield with control flags (e.g., execution break, transition lock).
 * - `Stats`            – Statistics for monitoring the machine’s performance (executions, transitions).
 * - `onBreakTimeout`   – Callback executed when an execution block timeout is exceeded.
 * - `onTrans`          – Callback invoked during state transition.
 * - `ctx`              – Context pointer for user-defined data shared between states or unique per instance.
 */
struct SM_instance_t
{
    SM_state_t *SM_states;
    SM_state_t *ActualState;
    SM_state_t *PrevState;
    uint16_t NumberOfStates;

    SM_timestamp_t Time;
    SM_control_flags_t SM_control_flags;
    SM_stats_t Stats;

    void (*onBreakTimeout)(SM_instance_t *me);
    void (*onTrans)(SM_instance_t *me);

    void *ctx;
};

/**
 * @brief Registers the time source for the state machine tick mechanism.
 *
 * This function is used to provide the state machine with a time base for managing timeouts,
 * delays, and transition timing. Depending on the configuration (`SM_TICK_FROM_FUNC`),
 * the user must provide either a function that returns the current time or a pointer to
 * a variable representing the current time.
 *
 * - If `SM_TICK_FROM_FUNC` is set to 1, call `SM_tick_function_register()` with a function
 *   that returns the current tick value.
 * - If `SM_TICK_FROM_FUNC` is set to 0, call `SM_tick_variable_register()` with a pointer
 *   to a variable that is periodically updated with the current tick value.
 *
 * @param Function Pointer to the function returning the current time tick (only if `SM_TICK_FROM_FUNC` is 1).
 * @param Variable Pointer to the time tick variable (only if `SM_TICK_FROM_FUNC` is 0).
 * @return SM_OK if registration was successful; an error code otherwise.
 */
#if SM_TICK_FROM_FUNC
SM_operate_status SM_tick_function_register(SM_TIME_t (*Function)(void));
#else
SM_operate_status SM_tick_variable_register(SM_TIME_t *Variable);
#endif

/**
 * @brief Core API of the state machine engine.
 *
 * These functions provide the main interface for initializing and executing a state machine,
 * as well as performing state transitions.
 */

/**
 * @brief Initializes a state machine instance.
 *
 * This function configures a new state machine instance with the specified state table,
 * starting state, total number of states, and optional user context.
 *
 * @param SM_instance Pointer to the instance to be initialized.
 * @param SM_states Pointer to the array of state descriptors.
 * @param FirstState Index of the initial state to start from.
 * @param NumberOfStates Total number of states defined in the state table.
 * @param ctx Optional user context pointer to be stored in the instance.
 * @return SM_OK on successful initialization; SM_INIT_ERR on failure (e.g., invalid parameters).
 */
SM_operate_status SM_init(SM_instance_t *SM_instance, SM_state_t *SM_states, uint16_t FirstState,
                          uint16_t NumberOfStates, void *ctx);

/**
 * @brief Executes the logic of the current state.
 *
 * This function calls the `onExec` callback of the current state, handles delays and timeouts,
 * and manages the execution flow according to internal timing and control flags.
 *
 * @param SM_instance Pointer to the instance being executed.
 * @return SM_OK if execution completed successfully.
 *         SM_WRONG_STATE if the current state is invalid or out of range.
 *         SM_EXEC_DELAYED if execution was delayed due to active delay timer.
 *         SM_EXEC_NULL_PTR if the instance pointer or required function pointers are NULL.
 */
SM_operate_status SM_Execution(SM_instance_t *SM_instance);

/**
 * @brief Performs a state transition.
 *
 * This function transitions the machine from the current state to a new state,
 * using the specified transition mode to control entry and exit callbacks.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @param mode Transition mode (entry/exit/both/fast).
 * @param State Index of the state to transition into.
 * @return SM_OK if the transition was successful.
 *         SM_TRANS_ERR if the requested state is invalid or transition failed.
 *         SM_TRANS_LOCKED if transitions are currently locked and cannot be performed.
 */
SM_operate_status SM_Trans(SM_instance_t *SM_instance, SM_transision_mode mode, uint16_t State);

/**
 * @brief Control mechanisms for execution flow and transitions.
 *
 * These functions allow fine-grained control over the execution of a state machine,
 * including the ability to introduce delays, temporarily block execution or transitions,
 * and release those blocks.
 */

/**
 * @brief Temporarily blocks state execution for a defined period.
 *
 * This function activates an execution break, preventing the `onExec` function
 * from being called until the specified timeout has elapsed.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @param Timeout Duration of the execution break in system ticks.
 * @return SM_OK if the break was successfully set.
 *         SM_OPRT_INSTANCE_DOES_NOT_EXIST if the provided instance pointer is NULL.
 */
SM_operate_status SM_exec_break(SM_instance_t *SM_instance, SM_TIME_t Timeout);

/**
 * @brief Releases a previously set execution break.
 *
 * This function cancels any active execution block, immediately allowing
 * the `onExec` function to resume execution.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @return SM_OK if the execution break was successfully released.
 *         SM_OPRT_INSTANCE_DOES_NOT_EXIST if the provided instance pointer is NULL.
 */
SM_operate_status SM_exec_break_release(SM_instance_t *SM_instance);

/**
 * @brief Locks state transitions for a specified timeout period.
 *
 * This function prevents any transition from being performed during the timeout window.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @param Timeout Duration for which transitions will be locked.
 * @return SM_OK if the transition lock was successfully set.
 *         SM_OPRT_INSTANCE_DOES_NOT_EXIST if the provided instance pointer is NULL.
 */
SM_operate_status SM_trans_lock(SM_instance_t *SM_instance, SM_TIME_t Timeout);

/**
 * @brief Releases a previously applied transition lock.
 *
 * Allows state transitions to be performed again.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @return SM_OK if the transition lock was successfully released.
 *         SM_OPRT_INSTANCE_DOES_NOT_EXIST if the provided instance pointer is NULL.
 */
SM_operate_status SM_trans_lock_release(SM_instance_t *SM_instance);

/**
 * @brief Introduces a delay before the next execution cycle.
 *
 * This function defers the next call to `onExec` for the specified duration.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @param Delay Delay duration in system ticks.
 * @return SM_OK if the delay was set successfully.
 *         SM_OPRT_INSTANCE_DOES_NOT_EXIST if the provided instance pointer is NULL.
 */
SM_operate_status SM_exec_delay(SM_instance_t *SM_instance, SM_TIME_t Delay);

/**
 * @brief Callback registration for state machine events.
 *
 * These functions allow registering user-defined callback functions that are triggered
 * when specific events occur in the state machine, such as a timeout during execution break
 * or a successful state transition.
 */

/**
 * @brief Registers a callback for execution break timeout.
 *
 * The specified function will be called when the execution break times out,
 * allowing custom recovery or logging behavior.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @param onBreakTimeout Function pointer to be called on execution break timeout.
 * @return SM_OK if the callback was registered successfully;
 *         SM_OPRT_INSTANCE_DOES_NOT_EXIST if the provided instance pointer is NULL.
 */
SM_operate_status SM_onBreakTimeout_callback_register(SM_instance_t *SM_instance,
                                                      void (*onBreakTimeout)(SM_instance_t *me));

/**
 * @brief Registers a callback for state transition events.
 *
 * The specified function will be called immediately after a successful transition,
 * enabling custom logic to be executed between state changes.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @param onTrans Function pointer to be called on successful state transition.
 * @return SM_OK if the callback was registered successfully;
 *         SM_OPRT_INSTANCE_DOES_NOT_EXIST if the provided instance pointer is NULL.
 */
SM_operate_status SM_onTrans_callback_register(SM_instance_t *SM_instance, void (*onTrans)(SM_instance_t *me));

/**
 * @brief Introspection functions for retrieving state machine metrics.
 *
 * These functions provide insight into the internal state and performance of the state machine.
 * They allow querying the current state index, time spent in the current state, and various
 * execution counters useful for diagnostics, debugging, or statistics collection.
 */

/**
 * @brief Returns the index number of the current active state.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @return The index of the current state, or UINT16_MAX if the instance pointer is NULL.
 */
uint16_t SM_get_state_number(const SM_instance_t *SM_instance);

/**
 * @brief Returns the time elapsed since entering the current state.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @return Time elapsed in the current state. Returns SM_MAX_TIMEOUT if SM_instance is NULL.
 */
SM_TIME_t SM_get_time_in_state(const SM_instance_t *SM_instance);

/**
 * @brief Returns the number of times the current state has been executed.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @return Execution count of the current state. Returns UINT32_MAX if SM_instance is NULL.
 */
uint32_t SM_get_exec_counter_state(const SM_instance_t *SM_instance);

/**
 * @brief Returns the total number of execution cycles of the entire state machine.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @return Total execution count of the state machine. Returns UINT32_MAX if SM_instance is NULL.
 */
uint32_t SM_get_exec_counter_machine(const SM_instance_t *SM_instance);

/**
 * @brief Returns the number of successful transitions performed.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @return Total count of performed transitions. Returns UINT32_MAX if SM_instance is NULL.
 */
uint32_t SM_get_trans_counter(const SM_instance_t *SM_instance);

#endif /* SM_CORE_SM_H_ */
