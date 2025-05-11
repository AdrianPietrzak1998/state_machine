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

#include <stdint.h>
#include <limits.h>

#define SM_TICK_FROM_FUNC 0

#define SM_TIME_BASE_TYPE_CUSTOM volatile uint32_t
#define SM_TIME_BASE_TYPE_CUSTOM_IS_UINT32

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

typedef enum
{
    SM_TRANS_ENTRY_EXIT = 0,
    SM_TRANS_ENTRY,
    SM_TRANS_EXIT,
    SM_TRANS_FAST
} SM_transision_mode;

typedef struct SM_instance_t SM_instance_t;

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

typedef struct
{
    void (*onEntry)(SM_instance_t *me);
    void (*onExec)(SM_instance_t *me);
    void (*onExit)(SM_instance_t *me);
} SM_state_t;

typedef struct
{
    unsigned int ExecBreak : 1;
    unsigned int TransitionLock : 1;
} SM_control_flags_t;

typedef struct
{
    uint32_t StateExecutionCounter;
    uint32_t MachineExecutionCounter;
    uint32_t TransCounter;
} SM_stats_t;

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

// Tick source registration
#if SM_TICK_FROM_FUNC
SM_operate_status SM_tick_function_register(SM_TIME_t (*Function)(void));
#else
SM_operate_status SM_tick_variable_register(SM_TIME_t *Variable);
#endif

// Core API
SM_operate_status SM_init(SM_instance_t *SM_instance, SM_state_t *SM_states, uint16_t FirstState, uint16_t NumberOfStates, void *ctx);
SM_operate_status SM_Execution(SM_instance_t *SM_instance);
SM_operate_status SM_Trans(SM_instance_t *SM_instance, SM_transision_mode mode, uint16_t State);

// Control mechanisms
SM_operate_status SM_exec_break(SM_instance_t *SM_instance, SM_TIME_t Timeout);
SM_operate_status SM_exec_break_release(SM_instance_t *SM_instance);
SM_operate_status SM_trans_lock(SM_instance_t *SM_instance, SM_TIME_t Timeout);
SM_operate_status SM_trans_lock_release(SM_instance_t *SM_instance);
SM_operate_status SM_exec_delay(SM_instance_t *SM_instance, SM_TIME_t Delay);

// Callbacks
SM_operate_status SM_onBreakTimeout_callback_register(SM_instance_t *SM_instance, void (*onBreakTimeout)(SM_instance_t *me));
SM_operate_status SM_onTrans_callback_register(SM_instance_t *SM_instance, void (*onTrans)(SM_instance_t *me));

// Introspection
uint16_t    SM_get_state_number(const SM_instance_t *SM_instance);
SM_TIME_t   SM_get_time_in_state(const SM_instance_t *SM_instance);
uint32_t    SM_get_exec_counter_state(const SM_instance_t *SM_instance);
uint32_t    SM_get_exec_counter_machine(const SM_instance_t *SM_instance);
uint32_t    SM_get_trans_counter(const SM_instance_t *SM_instance);

#endif /* SM_CORE_SM_H_ */
