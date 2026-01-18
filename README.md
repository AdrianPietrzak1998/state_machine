# Embedded State Machine Library

Lightweight and flexible **state machine library** for embedded systems. Designed to efficiently manage states, transitions, delays, and execution flow on microcontrollers.  

> **Important:** The state machine is **not reentrant**. All API functions must be called from the **same execution context** (e.g., the same task, main loop, or ISR). Calling from multiple contexts on **the same** instance simultaneously will lead to undefined behavior.

---

## Features

- Configurable states with `onEntry`, `onExec`, and `onExit` callbacks  
- Supports execution breaks, delays, and transition locks  
- Tracks execution and transition statistics  
- Lightweight and portable, suitable for microcontroller applications  
- Flexible time base: use either a variable or a function to provide ticks  

---

## Installation

Copy the `sm.h` and `sm.c` files into your project and include the header:

```c
#include "sm.h"
```

### Configure time source

By default, the library uses a global variable for system ticks (`SM_TICK_FROM_FUNC = 0`).  
If you prefer using a function for ticks, set `SM_TICK_FROM_FUNC = 1` in `sm.h` and register your function with:

```c
SM_tick_function_register(my_tick_function);
```

Otherwise, register a pointer to a global tick variable:

```c
uint32_t sys_tick = 0;
SM_tick_variable_register(&sys_tick);
```

---

## Usage

### 1. Define your states

```c
typedef enum {
    STATE_IDLE,
    STATE_RUN,
    STATE_ERROR,

    STATE_END
} MyStates;

SM_state_t states[STATE_END] = {
    { .onEntry = idle_entry, .onExec = idle_exec, .onExit = idle_exit },
    { .onEntry = run_entry,  .onExec = run_exec,  .onExit = run_exit },
    { .onEntry = err_entry,  .onExec = err_exec,  .onExit = err_exit }
};
```

### 2. Implement state callbacks

```c
void idle_entry(SM_instance_t *me) { /* Init idle state */ }
void idle_exec(SM_instance_t *me)  { /* Do idle tasks */ }
void idle_exit(SM_instance_t *me)  { /* Cleanup before leaving idle */ }

void run_entry(SM_instance_t *me) { /* Start running */ }
void run_exec(SM_instance_t *me)  { /* Running tasks */ }
void run_exit(SM_instance_t *me)  { /* Stop running */ }

void err_entry(SM_instance_t *me) { /* Enter error state */ }
void err_exec(SM_instance_t *me)  { /* Error handling */ }
void err_exit(SM_instance_t *me)  { /* Clear error */ }
```

### 3. Initialize the state machine

```c
SM_instance_t sm;
SM_init(&sm, states, STATE_IDLE, STATE_END, NULL);
```

### 4. Execute in your main loop

```c
while (1)
{
    SM_Execution(&sm);
}
```

### 5. Perform state transitions

```c
SM_Trans(&sm, SM_TRANS_ENTRY_EXIT, STATE_RUN);  // Normal transition
SM_Trans(&sm, SM_TRANS_FAST, STATE_ERROR);      // Fast transition without ENTRY and EXIT functions
```

### 6. Control execution flow

```c
SM_exec_delay(&sm, 100);        // Delay next execution for 100 ticks
SM_exec_break(&sm, 50);         // Pause execution for 50 ticks
SM_exec_break_release(&sm);     // Resume execution immediately
SM_trans_lock(&sm, 200);        // Lock transitions for 200 ticks
SM_trans_lock_release(&sm);     // Unlock transitions
```

### 7. Register callbacks for events

```c
SM_onBreakTimeout_callback_register(&sm, break_timeout_handler);
SM_onTrans_callback_register(&sm, transition_handler);
```

### 8. Retrieve runtime metrics

```c
uint16_t state_num = SM_get_state_number(&sm);
SM_TIME_t time_in_state = SM_get_time_in_state(&sm);
uint32_t state_exec_count = SM_get_exec_counter_state(&sm);
uint32_t machine_exec_count = SM_get_exec_counter_machine(&sm);
uint32_t transitions = SM_get_trans_counter(&sm);
```

---

## Status Codes

- `SM_OK` – Operation successful  
- `SM_INIT_ERR` – Initialization failed  
- `SM_EXEC_DELAYED` – Execution delayed due to timer  
- `SM_EXEC_NULL_PTR` – Null pointer encountered  
- `SM_TRANS_ERR` – Transition failed  
- `SM_TRANS_LOCKED` – Transition is locked  
- `SM_WRONG_STATE` – State pointer invalid  
- `SM_OPRT_INSTANCE_DOES_NOT_EXIST` – Instance pointer invalid  
- `SM_TRANS_NULL_PTR` – Null pointer passed to transition function  

---

## License

This library is licensed under the **Mozilla Public License 2.0**.  
See [http://mozilla.org/MPL/2.0/](http://mozilla.org/MPL/2.0/) for details.

