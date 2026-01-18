/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Author: Adrian Pietrzak
 * GitHub: https://github.com/AdrianPietrzak1998
 * Created: May 6, 2025
 */

#include "sm.h"
#include "assert.h"
#include <stddef.h>

#if SM_TICK_FROM_FUNC

SM_TIME_t (*SM_get_tick)(void) = NULL;

#define SM_GET_TICK ((SM_get_tick != NULL) ? SM_get_tick() : ((SM_TIME_t)0))

SM_operate_status SM_tick_function_register(SM_TIME_t (*Function)(void))
{
    if (Function == NULL)
    {
        return SM_INIT_ERR;
    }

    SM_get_tick = Function;
    return SM_OK;
}

#else

SM_TIME_t *SM_tick = NULL;

#define SM_GET_TICK (*(SM_tick))

SM_operate_status SM_tick_variable_register(SM_TIME_t *Variable)
{
    if (Variable == NULL)
    {
        return SM_INIT_ERR;
    }

    SM_tick = Variable;
    return SM_OK;
}

#endif

/**
 * @brief Completely resets the state machine instance.
 *
 * This function clears internal pointers, counters, flags, and timers,
 * bringing the instance to an uninitialized state. It uses assert to check
 * for a valid pointer and always returns SM_OK.
 *
 * @param SM_instance Pointer to the state machine instance to reset (must not be NULL).
 * @return Always returns SM_OK.
 */
static SM_operate_status SM_reset_instance(SM_instance_t *SM_instance)
{

    assert(SM_instance != NULL);

    SM_instance->SM_states = NULL;
    SM_instance->ActualState = NULL;
    SM_instance->PrevState = NULL;
    SM_instance->NumberOfStates = 0;

    SM_instance->Time.TransTick = 0;
    SM_instance->Time.lastExecTick = 0;
    SM_instance->Time.ExecBlockTick = 0;
    SM_instance->Time.TransLockTick = 0;
    SM_instance->Time.DelayTime = 0;
    SM_instance->Time.ExecBlockTimeout = 0;
    SM_instance->Time.TransLockTimeout = 0;
    SM_instance->SM_control_flags.ExecBreak = 0;
    SM_instance->SM_control_flags.TransitionLock = 0;
    SM_instance->Stats.StateExecutionCounter = 0;
    SM_instance->Stats.MachineExecutionCounter = 0;
    SM_instance->Stats.TransCounter = 0;

    SM_instance->onBreakTimeout = NULL;
    SM_instance->onTrans = NULL;

    SM_instance->ctx = NULL;

    return SM_OK;
}

/**
 * @brief Checks if the given state pointer is within the valid range of states.
 *
 * This function verifies whether the specified state pointer belongs to the
 * array of states defined in the state machine instance. It returns SM_OK if
 * the state is valid, or SM_WRONG_STATE if it is outside the valid range.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @param SM_state Pointer to the state to check.
 * @return SM_OK if the state is within range; otherwise SM_WRONG_STATE.
 */
static SM_operate_status SM_state_is_in_range(SM_instance_t *SM_instance, SM_state_t *SM_state)
{
    if (SM_state >= SM_instance->SM_states && SM_state < SM_instance->SM_states + SM_instance->NumberOfStates)
    {
        return SM_OK;
    }
    else
    {
        return SM_WRONG_STATE;
    }
}

/**
 * @brief Initializes the state machine instance.
 *
 * This function sets up the state machine instance with the provided array of states,
 * the index of the initial state, the total number of states, and an optional context pointer.
 * It prepares the instance for execution by resetting internal timers, counters, and flags.
 *
 * @param SM_instance Pointer to the state machine instance to initialize.
 * @param SM_states Pointer to the array of states defining the state machine.
 * @param FirstState Index of the initial state in the states array.
 * @param NumberOfStates Total number of states in the states array.
 * @param ctx Optional user-defined context pointer associated with this instance.
 * @return SM_OK on successful initialization; SM_INIT_ERR on failure (e.g., invalid parameters).
 */
SM_operate_status SM_init(SM_instance_t *SM_instance, SM_state_t *SM_states, uint16_t FirstState,
                          uint16_t NumberOfStates, void *ctx)
{
    if ((SM_instance == NULL) || (SM_states == NULL) || (NumberOfStates <= FirstState) || (NumberOfStates == 0))
    {
        return SM_INIT_ERR;
    }

    if (SM_reset_instance(SM_instance) != SM_OK)
    {
        return SM_INIT_ERR;
    }
    SM_instance->SM_states = SM_states;
    SM_instance->ActualState = &SM_states[FirstState];
    SM_instance->NumberOfStates = NumberOfStates;
    SM_instance->ctx = ctx;

    if (NULL != SM_instance->ActualState->onEntry)
    {
        SM_instance->ActualState->onEntry(SM_instance);
    }

    return SM_OK;
}

/**
 * @brief Registers a callback function to be called when the execution break timeout occurs.
 *
 * This function assigns the user-provided callback to the state machine instance,
 * which will be invoked if the execution break timeout is exceeded during runtime.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @param onBreakTimeout Pointer to the callback function to be called on break timeout.
 *                       The function receives a pointer to the state machine instance.
 * @return SM_OK if the callback was registered successfully;
 *         SM_OPRT_INSTANCE_DOES_NOT_EXIST if the provided instance pointer is NULL.
 */
SM_operate_status SM_onBreakTimeout_callback_register(SM_instance_t *SM_instance,
                                                      void (*onBreakTimeout)(SM_instance_t *me))
{
    if (SM_instance == NULL)
    {
        return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
    }

    SM_instance->onBreakTimeout = onBreakTimeout;
    return SM_OK;
}

/**
 * @brief Registers a callback function to be called on state transitions.
 *
 * This function assigns the user-provided callback to the state machine instance,
 * which will be invoked whenever a state transition occurs.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @param onTrans Pointer to the callback function to be called on state transition.
 *                The function receives a pointer to the state machine instance.
 * @return SM_OK if the callback was registered successfully;
 *         SM_OPRT_INSTANCE_DOES_NOT_EXIST if the provided instance pointer is NULL.
 */
SM_operate_status SM_onTrans_callback_register(SM_instance_t *SM_instance, void (*onTrans)(SM_instance_t *me))
{
    if (SM_instance == NULL)
    {
        return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
    }

    SM_instance->onTrans = onTrans;
    return SM_OK;
}

/**
 * @brief Executes the state machine instance logic.
 *
 * This function runs the logic of the current state of the state machine instance,
 * calling the appropriate execution callback if conditions allow.
 * It handles delays and checks for valid state before execution.
 *
 * @param SM_instance Pointer to the state machine instance to execute.
 * @return SM_OK if execution completed successfully.
 *         SM_WRONG_STATE if the current state is invalid or out of range.
 *         SM_EXEC_DELAYED if execution was delayed due to active delay timer.
 *         SM_EXEC_NULL_PTR if the instance pointer or required function pointers are NULL.
 */
SM_operate_status SM_Execution(SM_instance_t *SM_instance)
{
    if (SM_instance == NULL)
    {
        return SM_EXEC_NULL_PTR;
    }

    if (SM_instance->SM_control_flags.ExecBreak &&
        (SM_GET_TICK - SM_instance->Time.ExecBlockTick >= SM_instance->Time.ExecBlockTimeout))
    {
        SM_instance->SM_control_flags.ExecBreak = 0;

        if (SM_instance->onBreakTimeout != NULL)
        {
            SM_instance->onBreakTimeout(SM_instance);
        }
    }

    if (SM_instance->ActualState->onExec != NULL)
    {
        if (((SM_instance->Time.DelayTime == 0) ||
             (SM_GET_TICK - SM_instance->Time.lastExecTick >= SM_instance->Time.DelayTime)) &&
            !SM_instance->SM_control_flags.ExecBreak)
        {
            if (SM_state_is_in_range(SM_instance, SM_instance->ActualState) != SM_OK)
            {
                return SM_WRONG_STATE;
            }
            SM_instance->Time.lastExecTick = SM_GET_TICK;
            SM_instance->Time.DelayTime = 0;
            SM_instance->ActualState->onExec(SM_instance);
            SM_instance->Stats.StateExecutionCounter++;
            SM_instance->Stats.MachineExecutionCounter++;
            return SM_OK;
        }
        else
        {
            return SM_EXEC_DELAYED;
        }
    }
    else
    {
        return SM_EXEC_NULL_PTR;
    }
}

/**
 * @brief Performs a state transition in the state machine instance.
 *
 * This function attempts to transition the state machine to the specified state,
 * respecting the transition mode which determines whether entry and exit callbacks
 * are called during the transition.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @param mode Transition mode, specifying how entry/exit functions are called.
 * @param State Index of the target state to transition to.
 * @return SM_OK if the transition was successful.
 *         SM_TRANS_ERR if the requested state is invalid or transition failed.
 *         SM_TRANS_LOCKED if transitions are currently locked and cannot be performed.
 *         SM_TRANS_NULL_PTR if the provided instance pointer is NULL.
 */
SM_operate_status SM_Trans(SM_instance_t *SM_instance, SM_transision_mode mode, uint16_t State)
{
    if (SM_instance == NULL)
    {
        return SM_TRANS_NULL_PTR;
    }

    if (SM_instance->NumberOfStates <= State)
    {
        return SM_TRANS_ERR;
    }

    if (SM_instance->SM_control_flags.TransitionLock)
    {
        if (SM_GET_TICK - SM_instance->Time.TransLockTick >= SM_instance->Time.TransLockTimeout)
        {
            SM_instance->SM_control_flags.TransitionLock = 0;
        }
        else
        {
            return SM_TRANS_LOCKED;
        }
    }

    SM_state_t *target_state = &SM_instance->SM_states[State];

    switch (mode)
    {
    case SM_TRANS_ENTRY_EXIT: {
        if ((SM_instance->ActualState->onExit != NULL) && (target_state->onEntry != NULL))
        {
            SM_instance->ActualState->onExit(SM_instance);
        }
        else
        {
            return SM_TRANS_ERR;
        }

        SM_instance->PrevState = SM_instance->ActualState;
        SM_instance->ActualState = target_state;
        SM_instance->ActualState->onEntry(SM_instance);
        break;
    }

    case SM_TRANS_ENTRY: {
        if (target_state->onEntry == NULL)
        {
            return SM_TRANS_ERR;
        }

        SM_instance->PrevState = SM_instance->ActualState;
        SM_instance->ActualState = target_state;
        SM_instance->ActualState->onEntry(SM_instance);
        break;
    }

    case SM_TRANS_EXIT: {
        if (SM_instance->ActualState->onExit != NULL)
        {
            SM_instance->ActualState->onExit(SM_instance);
        }
        else
        {
            return SM_TRANS_ERR;
        }

        SM_instance->PrevState = SM_instance->ActualState;
        SM_instance->ActualState = target_state;
        break;
    }

    case SM_TRANS_FAST: {
        SM_instance->PrevState = SM_instance->ActualState;
        SM_instance->ActualState = target_state;
        break;
    }

    default: {
        return SM_TRANS_ERR;
    }
    }

    SM_instance->Time.TransTick = SM_GET_TICK;

    if (SM_instance->onTrans != NULL)
    {
        SM_instance->onTrans(SM_instance);
    }

    SM_instance->Stats.StateExecutionCounter = 0;
    SM_instance->Stats.TransCounter++;

    return SM_OK;
}

/**
 * @brief Temporarily blocks execution of the state machine instance.
 *
 * This function sets an execution break with a specified timeout, during which
 * the state machine will pause its execution cycle.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @param Timeout Duration of the execution break.
 * @return SM_OK if the break was successfully set.
 *         SM_OPRT_INSTANCE_DOES_NOT_EXIST if the provided instance pointer is NULL.
 */
SM_operate_status SM_exec_break(SM_instance_t *SM_instance, SM_TIME_t Timeout)
{
    if (SM_instance == NULL)
    {
        return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
    }

    SM_instance->Time.ExecBlockTick = SM_GET_TICK;
    SM_instance->Time.ExecBlockTimeout = Timeout;
    SM_instance->SM_control_flags.ExecBreak = 1;

    return SM_OK;
}

/**
 * @brief Releases the execution break, allowing the state machine to resume execution.
 *
 * This function clears any active execution break on the specified state machine instance,
 * enabling normal operation to continue.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @return SM_OK if the execution break was successfully released.
 *         SM_OPRT_INSTANCE_DOES_NOT_EXIST if the provided instance pointer is NULL.
 */
SM_operate_status SM_exec_break_release(SM_instance_t *SM_instance)
{
    if (SM_instance == NULL)
    {
        return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
    }

    SM_instance->SM_control_flags.ExecBreak = 0;
    return SM_OK;
}

/**
 * @brief Locks state transitions for a specified timeout period.
 *
 * This function prevents any state transitions from occurring on the given state machine instance
 * for the duration of the specified timeout. It is useful to avoid state changes during critical operations.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @param Timeout Duration (in time units) for which the transition lock is active.
 * @return SM_OK if the transition lock was successfully set.
 *         SM_OPRT_INSTANCE_DOES_NOT_EXIST if the provided instance pointer is NULL.
 */
SM_operate_status SM_trans_lock(SM_instance_t *SM_instance, SM_TIME_t Timeout)
{
    if (SM_instance == NULL)
    {
        return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
    }

    SM_instance->Time.TransLockTick = SM_GET_TICK;
    SM_instance->Time.TransLockTimeout = Timeout;
    SM_instance->SM_control_flags.TransitionLock = 1;

    return SM_OK;
}

/**
 * @brief Releases the transition lock on the state machine instance.
 *
 * This function clears any active transition lock, allowing state transitions to occur immediately.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @return SM_OK if the transition lock was successfully released.
 *         SM_OPRT_INSTANCE_DOES_NOT_EXIST if the provided instance pointer is NULL.
 */
SM_operate_status SM_trans_lock_release(SM_instance_t *SM_instance)
{
    if (SM_instance == NULL)
    {
        return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
    }

    SM_instance->SM_control_flags.TransitionLock = 0;
    return SM_OK;
}

/**
 * @brief Sets an execution delay on the state machine instance.
 *
 * This function starts a delay timer during which the execution of the state machine
 * is paused. After the delay period expires, normal execution resumes.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @param Delay Duration of the delay in ticks.
 * @return SM_OK if the delay was set successfully.
 *         SM_OPRT_INSTANCE_DOES_NOT_EXIST if the provided instance pointer is NULL.
 */
SM_operate_status SM_exec_delay(SM_instance_t *SM_instance, SM_TIME_t Delay)
{
    if (SM_instance == NULL)
    {
        return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
    }

    SM_instance->Time.DelayTime = Delay;
    return SM_OK;
}

/**
 * @brief Returns the current state number (index) of the state machine instance.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @return The index of the current state, or UINT16_MAX if the instance pointer is NULL.
 */
uint16_t SM_get_state_number(const SM_instance_t *SM_instance)
{
    if ((SM_instance == NULL) || (SM_instance->ActualState == NULL))
    {
        return UINT16_MAX;
    }

    return (uint16_t)(SM_instance->ActualState - SM_instance->SM_states);
}

/**
 * @brief Returns the elapsed time since the state machine entered the current state.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @return Time elapsed in the current state. Returns SM_MAX_TIMEOUT if SM_instance is NULL.
 */
SM_TIME_t SM_get_time_in_state(const SM_instance_t *SM_instance)
{
    if ((SM_instance == NULL) || (SM_instance->ActualState == NULL))
    {
        return SM_MAX_TIMEOUT;
    }

    return (SM_GET_TICK - SM_instance->Time.TransTick);
}

/**
 * @brief Returns the number of times the current state's execution function has been called.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @return Execution count of the current state. Returns UINT32_MAX if SM_instance is NULL.
 */
uint32_t SM_get_exec_counter_state(const SM_instance_t *SM_instance)
{
    if ((SM_instance == NULL) || (SM_instance->ActualState == NULL))
    {
        return UINT32_MAX;
    }

    return SM_instance->Stats.StateExecutionCounter;
}

/**
 * @brief Returns the total number of times the state machine execution function has been called.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @return Total execution count of the state machine. Returns UINT32_MAX if SM_instance is NULL.
 */
uint32_t SM_get_exec_counter_machine(const SM_instance_t *SM_instance)
{
    if (SM_instance == NULL)
    {
        return UINT32_MAX;
    }

    return SM_instance->Stats.MachineExecutionCounter;
}

/**
 * @brief Returns the total number of state transitions performed by the state machine.
 *
 * @param SM_instance Pointer to the state machine instance.
 * @return Total count of performed transitions. Returns UINT32_MAX if SM_instance is NULL.
 */
uint32_t SM_get_trans_counter(const SM_instance_t *SM_instance)
{
    if (SM_instance == NULL)
    {
        return UINT32_MAX;
    }

    return SM_instance->Stats.TransCounter;
}