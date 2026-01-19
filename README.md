# Embedded State Machine Library

[![License: MPL 2.0](https://img.shields.io/badge/License-MPL_2.0-brightgreen.svg)](https://opensource.org/licenses/MPL-2.0)
[![C99](https://img.shields.io/badge/C-99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![Platform](https://img.shields.io/badge/platform-embedded-orange.svg)](https://github.com/AdrianPietrzak1998/state_machine)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](http://makeapullrequest.com)
[![GitHub stars](https://img.shields.io/github/stars/AdrianPietrzak1998/state_machine.svg?style=social&label=Star)](https://github.com/AdrianPietrzak1998/state_machine)
[![GitHub forks](https://img.shields.io/github/forks/AdrianPietrzak1998/state_machine.svg?style=social&label=Fork)](https://github.com/AdrianPietrzak1998/state_machine/fork)


Lightweight and flexible **state machine library** for embedded systems. Designed to efficiently manage states, transitions, delays, and execution flow on microcontrollers.  

> **Important:** The state machine is **not reentrant**. All API functions must be called from the **same execution context** (e.g., the same task, main loop, or ISR). Calling from multiple contexts on **the same** instance simultaneously will lead to undefined behavior.

---

## Quick Start

```c
#include "sm.h"

// 1. Define states
typedef enum { STATE_IDLE, STATE_RUN, STATE_COUNT } States;

void idle_exec(SM_instance_t *me) { /* idle logic */ }
void run_exec(SM_instance_t *me) { /* running logic */ }

SM_state_t states[STATE_COUNT] = {
    { .onEntry = NULL, .onExec = idle_exec, .onExit = NULL },
    { .onEntry = NULL, .onExec = run_exec, .onExit = NULL }
};

// 2. Register time source
uint32_t tick = 0;
SM_tick_variable_register(&tick);

// 3. Initialize and run
SM_instance_t sm;
SM_init(&sm, states, STATE_IDLE, STATE_COUNT, NULL);

while(1) {
    tick++;  // Update in your timer ISR
    SM_Execution(&sm);
}
```

---

## Features

- Configurable states with `onEntry`, `onExec`, and `onExit` callbacks  
- Supports execution breaks, delays, and transition locks  
- Tracks execution and transition statistics  
- Lightweight and portable, suitable for microcontroller applications  
- Flexible time base: use either a variable or a function to provide ticks  

---

## Requirements

- **Compiler:** C99 or later
- **Platform:** Any (tested on ARM Cortex-M, AVR, x86)
- **Dependencies:** None (only standard C library)
- **RAM usage:** ~64 bytes per instance (depends on platform)
- **Code size:** ~2KB (with -Os optimization)

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

### 7. Using context for state-specific data

```c
typedef struct {
    int counter;
    bool error_flag;
} MyContext;

MyContext ctx = {0};
SM_init(&sm, states, STATE_IDLE, STATE_END, &ctx);

void idle_exec(SM_instance_t *me) {
    MyContext *c = (MyContext*)me->ctx;
    c->counter++;
    if (c->counter > 100) {
        SM_Trans(me, SM_TRANS_ENTRY_EXIT, STATE_RUN);
    }
}
```

### 8. Register callbacks for events

```c
SM_onBreakTimeout_callback_register(&sm, break_timeout_handler);
SM_onTrans_callback_register(&sm, transition_handler);
```

### 9. Retrieve runtime metrics

```c
uint16_t state_num = SM_get_state_number(&sm);
SM_TIME_t time_in_state = SM_get_time_in_state(&sm);
uint32_t state_exec_count = SM_get_exec_counter_state(&sm);
uint32_t machine_exec_count = SM_get_exec_counter_machine(&sm);
uint32_t transitions = SM_get_trans_counter(&sm);
```

---

## Example: Traffic Light Controller

```c
#include "sm.h"

typedef enum {
    LIGHT_RED,
    LIGHT_YELLOW, 
    LIGHT_GREEN,
    LIGHT_COUNT
} TrafficLight;

uint32_t tick = 0;

void red_entry(SM_instance_t *me) {
    set_red_light(ON);
}

void red_exec(SM_instance_t *me) {
    if (SM_get_time_in_state(me) >= 3000) {  // Red for 3 seconds
        SM_Trans(me, SM_TRANS_ENTRY_EXIT, LIGHT_GREEN);
    }
}

void red_exit(SM_instance_t *me) {
    set_red_light(OFF);
}

void green_entry(SM_instance_t *me) {
    set_green_light(ON);
}

void green_exec(SM_instance_t *me) {
    if (SM_get_time_in_state(me) >= 5000) {  // Green for 5 seconds
        SM_Trans(me, SM_TRANS_ENTRY_EXIT, LIGHT_YELLOW);
    }
}

void green_exit(SM_instance_t *me) {
    set_green_light(OFF);
}

void yellow_entry(SM_instance_t *me) {
    set_yellow_light(ON);
}

void yellow_exec(SM_instance_t *me) {
    if (SM_get_time_in_state(me) >= 2000) {  // Yellow for 2 seconds
        SM_Trans(me, SM_TRANS_ENTRY_EXIT, LIGHT_RED);
    }
}

void yellow_exit(SM_instance_t *me) {
    set_yellow_light(OFF);
}

SM_state_t traffic_states[LIGHT_COUNT] = {
    [LIGHT_RED] = { .onEntry = red_entry, .onExec = red_exec, .onExit = red_exit },
    [LIGHT_YELLOW] = { .onEntry = yellow_entry, .onExec = yellow_exec, .onExit = yellow_exit },
    [LIGHT_GREEN] = { .onEntry = green_entry, .onExec = green_exec, .onExit = green_exit }
};

int main(void) {
    SM_instance_t sm;
    
    // Register tick source
    SM_tick_variable_register(&tick);
    
    // Initialize state machine
    SM_init(&sm, traffic_states, LIGHT_RED, LIGHT_COUNT, NULL);
    
    while(1) {
        tick++;  // Increment every 1ms (from timer ISR)
        SM_Execution(&sm);
    }
}
```

---

## Best Practices

- **Always check return values** from `SM_Trans()` and `SM_Execution()`
- **Use context pointer** to share data between states instead of globals
- **Keep state callbacks short** - avoid blocking operations
- **Update tick regularly** - ideally in a timer interrupt
- **Test edge cases** - NULL callbacks, invalid transitions, timeout overflows
- **Consider transition guards** - check conditions before calling `SM_Trans()`

---

## Common Pitfalls

⚠️ **Forgetting to register tick source before SM_init()**
```c
// WRONG - will assert/crash
SM_init(&sm, states, STATE_IDLE, STATE_COUNT, NULL);

// CORRECT
SM_tick_variable_register(&tick);
SM_init(&sm, states, STATE_IDLE, STATE_COUNT, NULL);
```

⚠️ **Not updating tick variable**
```c
// WRONG - timeouts will never expire
while(1) {
    SM_Execution(&sm);  // tick never increments!
}

// CORRECT
while(1) {
    tick++;  // or update in timer ISR
    SM_Execution(&sm);
}
```

⚠️ **Calling from multiple contexts**
```c
// WRONG - not thread-safe!
void Task1() { SM_Execution(&sm); }
void Task2() { SM_Trans(&sm, ...); }  // Race condition!

// CORRECT - single context only
void MainTask() {
    SM_Execution(&sm);
    if (condition) SM_Trans(&sm, ...);
}
```

---

## Troubleshooting

**Q: My transitions don't work**
- Check if you registered tick source before `SM_init()`
- Verify tick variable is being updated
- Check return value of `SM_Trans()` - might be `SM_TRANS_LOCKED`

**Q: State never executes**
- Ensure `onExec` callback is not NULL
- Check if `SM_exec_break()` was called and not released
- Verify `SM_Execution()` is being called in your loop

**Q: Time-based delays don't work**
- Confirm tick variable increments regularly
- Check for tick overflow handling (automatic with unsigned types)
- Verify timeout values are reasonable for your tick rate

---

## Status Codes

- `SM_OK` — Operation successful  
- `SM_INIT_ERR` — Initialization failed  
- `SM_EXEC_DELAYED` — Execution delayed due to timer  
- `SM_EXEC_NULL_PTR` — Null pointer encountered  
- `SM_TRANS_ERR` — Transition failed  
- `SM_TRANS_LOCKED` — Transition is locked  
- `SM_WRONG_STATE` — State pointer invalid  
- `SM_OPRT_INSTANCE_DOES_NOT_EXIST` — Instance pointer invalid  
- `SM_TRANS_NULL_PTR` — Null pointer passed to transition function  

---

## License

This library is licensed under the **Mozilla Public License 2.0**.  
See [http://mozilla.org/MPL/2.0/](http://mozilla.org/MPL/2.0/) for details.

---

## Author

**Adrian Pietrzak**  
GitHub: [https://github.com/AdrianPietrzak1998](https://github.com/AdrianPietrzak1998)
