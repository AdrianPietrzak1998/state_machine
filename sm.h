/*
 * sm.h
 *
 *  Created on: May 6, 2025
 *      Author: Adrian
 */

#ifndef SM_CORE_SM_H_
#define SM_CORE_SM_H_

#include "main.h"

#define SM_MAX_TIMEOUT UINT32_MAX

typedef enum{
    SM_OK = 0,
    SM_OPRT_INSTANCE_DOES_NOT_EXIST,
    SM_INIT_ERR,
    SM_EXEC_DELAYED,
    SM_EXEC_NULL_PTR,
    SM_TRANS_ERR,
	SM_TRANS_LOCKED
}SM_operate_status;

typedef enum{
    SM_TRANS_ENTRY_EXIT = 0,
    SM_TRANS_ENTRY,
    SM_TRANS_EXIT,
    SM_TRANS_FAST
}SM_transision_mode;

typedef struct SM_instance_t SM_instance_t;

typedef struct{
    uint32_t TransTick;
    uint32_t lastExecTick;
    uint32_t ExecBlockTick;
    uint32_t TransLockTick;
    uint32_t DelayTime;
    uint32_t ExecBlockTimeout;
    uint32_t TransLockTimeout;
}SM_timestamp_t;

typedef struct{
    void (*onEntry)(SM_instance_t *me);
    void (*onExec)(SM_instance_t *me);
    void (*onExit)(SM_instance_t *me);
}SM_state_t;

typedef struct{
    uint32_t ExecBreak :1;
    uint32_t TransitionLock :1;
}SM_control_flags_t;

typedef struct{
	uint32_t StateExecutionCounter, MachineExecutionCounter, TransCounter;
}SM_stats_t;

struct SM_instance_t{
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


SM_operate_status SM_init(SM_instance_t *SM_instance, SM_state_t *SM_states, uint16_t FirstState, uint16_t NumberOfStates, void *ctx);
SM_operate_status SM_onBreakTimeout_callback_register(SM_instance_t *SM_instance, void (*onBreakTimeout)(SM_instance_t *me));
SM_operate_status SM_onTrans_callback_register(SM_instance_t *SM_instance, void (*onTrans)(SM_instance_t *me));
SM_operate_status SM_Execution(SM_instance_t *SM_instance);
SM_operate_status SM_Trans(SM_instance_t *SM_instance, SM_transision_mode mode, uint16_t State);
SM_operate_status SM_exec_break(SM_instance_t *SM_instance, uint32_t Timeout);
SM_operate_status SM_exec_break_release(SM_instance_t *SM_instance);
SM_operate_status SM_trans_lock(SM_instance_t *SM_instance, uint32_t Timeout);
SM_operate_status SM_trans_lock_release(SM_instance_t *SM_instance);
SM_operate_status SM_exec_delay(SM_instance_t *SM_instance, uint32_t Delay);
uint16_t SM_get_state_number(SM_instance_t *SM_instance);
uint32_t SM_get_time_in_state(SM_instance_t *SM_instance);
uint32_t SM_get_exec_counter_state(SM_instance_t *SM_instance);
uint32_t SM_get_exec_counter_machine(SM_instance_t *SM_instance);
uint32_t SM_get_trans_counter(SM_instance_t *SM_instance);

#endif /* SM_CORE_SM_H_ */
