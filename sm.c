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
#include <stddef.h>

#if SM_TICK_FROM_FUNC

SM_TIME_t (*SM_get_tick)(void) = NULL;

#define SM_GET_TICK    ((SM_get_tick != NULL) ? SM_get_tick() : ((SM_TIME_t)0))

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

#define SM_GET_TICK    (*(SM_tick))

static SM_operate_status SM_reset_instance(SM_instance_t *SM_instance)
{
	if (NULL == SM_instance)
	{
		return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
	}

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

static SM_operate_status SM_state_is_in_range(SM_instance_t *SM_instance, SM_state_t *SM_state)
{
	if (SM_state >= SM_instance->SM_states &&
	    SM_state <  SM_instance->SM_states + SM_instance->NumberOfStates)
	{
		return SM_OK;
	}
	else
	{
		return SM_WRONG_STATE;
	}
}

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

SM_operate_status SM_init(SM_instance_t *SM_instance,
                          SM_state_t *SM_states,
                          uint16_t FirstState,
                          uint16_t NumberOfStates,
                          void *ctx)
{
    if ((SM_instance == NULL) ||
        (SM_states == NULL) ||
        (NumberOfStates <= FirstState) ||
        (NumberOfStates == 0))
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

    return SM_OK;
}

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

SM_operate_status SM_onTrans_callback_register(SM_instance_t *SM_instance,
                                               void (*onTrans)(SM_instance_t *me))
{
    if (SM_instance == NULL)
    {
        return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
    }

    SM_instance->onTrans = onTrans;
    return SM_OK;
}

SM_operate_status SM_Execution(SM_instance_t *SM_instance)
{
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

SM_operate_status SM_Trans(SM_instance_t *SM_instance, SM_transision_mode mode, uint16_t State)
{
    if (SM_instance->NumberOfStates <= State)
    {
        return SM_TRANS_ERR;
    }

    if (SM_instance->SM_control_flags.TransitionLock)
    {
        if (SM_GET_TICK - SM_instance->Time.ExecBlockTick >= SM_instance->Time.TransLockTimeout)
        {
            SM_instance->SM_control_flags.TransitionLock = 0;
        }
        else
        {
            return SM_TRANS_LOCKED;
        }
    }

    switch (mode)
    {
        case SM_TRANS_ENTRY_EXIT:
        {
            if ((SM_instance->ActualState->onExit != NULL) || (SM_instance->ActualState->onEntry != NULL))
            {
                SM_instance->ActualState->onExit(SM_instance);
            }
            else
            {
                return SM_TRANS_ERR;
            }

            SM_instance->PrevState = SM_instance->ActualState;
            SM_instance->ActualState = &SM_instance->SM_states[State];
            SM_instance->ActualState->onEntry(SM_instance);
            break;
        }

        case SM_TRANS_ENTRY:
        {
            if (SM_instance->ActualState->onEntry == NULL)
            {
                return SM_TRANS_ERR;
            }

            SM_instance->PrevState = SM_instance->ActualState;
            SM_instance->ActualState = &SM_instance->SM_states[State];
            SM_instance->ActualState->onEntry(SM_instance);
            break;
        }

        case SM_TRANS_EXIT:
        {
            if (SM_instance->ActualState->onExit != NULL)
            {
                SM_instance->ActualState->onExit(SM_instance);
            }
            else
            {
                return SM_TRANS_ERR;
            }

            SM_instance->PrevState = SM_instance->ActualState;
            SM_instance->ActualState = &SM_instance->SM_states[State];
            break;
        }

        case SM_TRANS_FAST:
        {
            SM_instance->PrevState = SM_instance->ActualState;
            SM_instance->ActualState = &SM_instance->SM_states[State];
            break;
        }

        default:
        {
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

SM_operate_status SM_exec_break_release(SM_instance_t *SM_instance)
{
    if (SM_instance == NULL)
    {
        return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
    }

    SM_instance->SM_control_flags.ExecBreak = 0;
    return SM_OK;
}

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

SM_operate_status SM_trans_lock_release(SM_instance_t *SM_instance)
{
    if (SM_instance == NULL)
    {
        return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
    }

    SM_instance->SM_control_flags.TransitionLock = 0;
    return SM_OK;
}

SM_operate_status SM_exec_delay(SM_instance_t *SM_instance, SM_TIME_t Delay)
{
    if (SM_instance == NULL)
    {
        return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
    }

    SM_instance->Time.DelayTime = Delay;
    return SM_OK;
}

uint16_t SM_get_state_number(const SM_instance_t *SM_instance)
{
    if ((SM_instance == NULL) || (SM_instance->ActualState == NULL))
    {
        return UINT16_MAX;
    }

    return (uint16_t)(SM_instance->ActualState - SM_instance->SM_states);
}

SM_TIME_t SM_get_time_in_state(const SM_instance_t *SM_instance)
{
    if ((SM_instance == NULL) || (SM_instance->ActualState == NULL))
    {
        return SM_MAX_TIMEOUT;
    }

    return (SM_GET_TICK - SM_instance->Time.TransTick);
}

uint32_t SM_get_exec_counter_state(const SM_instance_t *SM_instance)
{
    if ((SM_instance == NULL) || (SM_instance->ActualState == NULL))
    {
        return UINT32_MAX;
    }

    return SM_instance->Stats.StateExecutionCounter;
}

uint32_t SM_get_exec_counter_machine(const SM_instance_t *SM_instance)
{
    if (SM_instance == NULL)
    {
        return UINT32_MAX;
    }

    return SM_instance->Stats.MachineExecutionCounter;
}

uint32_t SM_get_trans_counter(const SM_instance_t *SM_instance)
{
    if (SM_instance == NULL)
    {
        return UINT32_MAX;
    }

    return SM_instance->Stats.TransCounter;
}

