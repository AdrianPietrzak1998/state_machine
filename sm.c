/*
 * sm.c
 *
 *  Created on: May 6, 2025
 *      Author: Adrian
 */


#include "sm.h"
#include "string.h"

static uint32_t SM_get_tick(void)
{
    return uwTick;
}

SM_operate_status SM_init(SM_instance_t *SM_instance, SM_state_t *SM_states, uint16_t FirstState, uint16_t NumberOfStates, void *ctx)
{
    if(NULL == SM_instance || NULL == SM_states || NumberOfStates <= FirstState || 0 == NumberOfStates)
    {
        return SM_INIT_ERR;
    }

    memset(SM_instance, 0x00, sizeof(SM_instance_t));
    SM_instance->SM_states = SM_states;
    SM_instance->ActualState = &SM_states[FirstState];
    SM_instance->NumberOfStates = NumberOfStates;
    SM_instance->ctx = ctx;

    return SM_OK;
}

SM_operate_status SM_onBreakTimeout_callback_register(SM_instance_t *SM_instance, void (*onBreakTimeout)(SM_instance_t *me))
{
	if(NULL==SM_instance) return SM_OPRT_INSTANCE_DOES_NOT_EXIST;

	SM_instance->onBreakTimeout = onBreakTimeout;
	return SM_OK;
}

SM_operate_status SM_onTrans_callback_register(SM_instance_t *SM_instance, void (*onTrans)(SM_instance_t *me))
{
	if(NULL==SM_instance) return SM_OPRT_INSTANCE_DOES_NOT_EXIST;

	SM_instance->onTrans = onTrans;
	return SM_OK;
}

SM_operate_status SM_Execution(SM_instance_t *SM_instance)
{
    if(SM_instance->SM_control_flags.ExecBreak && (SM_get_tick() - SM_instance->Time.ExecBlockTick >= SM_instance->Time.ExecBlockTimeout))
    {
        SM_instance->SM_control_flags.ExecBreak = 0;
        if(NULL != SM_instance->onBreakTimeout) SM_instance->onBreakTimeout(SM_instance);
    }

    if(NULL != SM_instance->ActualState->onExec)
    {
        if(((0 == SM_instance->Time.DelayTime) ||
        (SM_get_tick()-SM_instance->Time.lastExecTick >= SM_instance->Time.DelayTime)) &&
        !SM_instance->SM_control_flags.ExecBreak)
        {
            SM_instance->Time.lastExecTick = SM_get_tick();
            SM_instance->Time.DelayTime = 0;
            SM_instance->ActualState->onExec(SM_instance);
            SM_instance->Stats.StateExecutionCounter++;
            SM_instance->Stats.MachineExecutionCounter++;
            return SM_OK;
        }
        else return SM_EXEC_DELAYED;
    }
    else return SM_EXEC_NULL_PTR;
}

SM_operate_status SM_Trans(SM_instance_t *SM_instance, SM_transision_mode mode, uint16_t State)
{
    if(SM_instance->NumberOfStates <= State) return SM_TRANS_ERR;
    if(SM_instance->SM_control_flags.TransitionLock)
    {
    	if(SM_get_tick() - SM_instance->Time.ExecBlockTick >= SM_instance->Time.TransLockTimeout)
    	{
    		SM_instance->SM_control_flags.TransitionLock = 0;
    	}
    	else
    	{
    		return SM_TRANS_LOCKED;
    	}
    }

    switch(mode)
    {
        case SM_TRANS_ENTRY_EXIT:
            if((NULL!=SM_instance->ActualState->onExit) || NULL!=SM_instance->ActualState->onEntry)
            {
                SM_instance->ActualState->onExit(SM_instance);
            }
            else return SM_TRANS_ERR;
            SM_instance->PrevState = SM_instance->ActualState;
            SM_instance->ActualState = &SM_instance->SM_states[State];
            SM_instance->ActualState->onEntry(SM_instance);
        break;
        case SM_TRANS_ENTRY:
            if(NULL==SM_instance->ActualState->onEntry) return SM_TRANS_ERR;
            SM_instance->PrevState = SM_instance->ActualState;
            SM_instance->ActualState = &SM_instance->SM_states[State];
            SM_instance->ActualState->onEntry(SM_instance);
        break;
        case SM_TRANS_EXIT:
            if((NULL!=SM_instance->ActualState->onExit))
            {
                SM_instance->ActualState->onExit(SM_instance);
            }
            else return SM_TRANS_ERR;
            SM_instance->PrevState = SM_instance->ActualState;
            SM_instance->ActualState = &SM_instance->SM_states[State];
        break;
        case SM_TRANS_FAST:
            SM_instance->PrevState = SM_instance->ActualState;
            SM_instance->ActualState = &SM_instance->SM_states[State];
        break;
    }

    SM_instance->Time.TransTick = SM_get_tick();
    if(NULL != SM_instance->onTrans) SM_instance->onTrans(SM_instance);
    SM_instance->Stats.StateExecutionCounter = 0;
    SM_instance->Stats.TransCounter++;
    return SM_OK;
}

SM_operate_status SM_exec_break(SM_instance_t *SM_instance, uint32_t Timeout)
{
    if(NULL==SM_instance) return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
    SM_instance->Time.ExecBlockTick = SM_get_tick();
    SM_instance->Time.ExecBlockTimeout = Timeout;
    SM_instance->SM_control_flags.ExecBreak = 1;
    return SM_OK;
}

SM_operate_status SM_exec_break_release(SM_instance_t *SM_instance)
{
    if(NULL==SM_instance) return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
    SM_instance->SM_control_flags.ExecBreak = 0;
    return SM_OK;
}

SM_operate_status SM_trans_lock(SM_instance_t *SM_instance, uint32_t Timeout)
{
    if(NULL==SM_instance) return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
    SM_instance->Time.TransLockTick = SM_get_tick();
    SM_instance->Time.TransLockTimeout = Timeout;
    SM_instance->SM_control_flags.TransitionLock = 1;
    return SM_OK;
}

SM_operate_status SM_trans_lock_release(SM_instance_t *SM_instance)
{
    if(NULL==SM_instance) return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
    SM_instance->SM_control_flags.TransitionLock = 0;
    return SM_OK;
}

SM_operate_status SM_exec_delay(SM_instance_t *SM_instance, uint32_t Delay)
{
    if(NULL==SM_instance) return SM_OPRT_INSTANCE_DOES_NOT_EXIST;
    SM_instance->Time.DelayTime = Delay;
    return SM_OK;
}

uint16_t SM_get_state_number(SM_instance_t *SM_instance)
{
    if (!SM_instance || !SM_instance->ActualState) return 0xFFFF;

    return SM_instance->ActualState - SM_instance->SM_states;
}

uint32_t SM_get_time_in_state(SM_instance_t *SM_instance)
{
	if (!SM_instance || !SM_instance->ActualState) return 0xFFFF;

	return SM_get_tick() - SM_instance->Time.TransTick;
}

uint32_t SM_get_exec_counter_state(SM_instance_t *SM_instance)
{
	if (!SM_instance || !SM_instance->ActualState) return 0xFFFF;

	return SM_instance->Stats.StateExecutionCounter;
}

uint32_t SM_get_exec_counter_machine(SM_instance_t *SM_instance)
{
	if (!SM_instance) return 0xFFFF;

	return SM_instance->Stats.MachineExecutionCounter;
}

uint32_t SM_get_trans_counter(SM_instance_t *SM_instance)
{
	if (!SM_instance) return 0xFFFF;

	return SM_instance->Stats.TransCounter;
}






