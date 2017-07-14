#include "xtask.h"
#include <stdlib.h>

// XData is a layer on top of XTask which provides a dataflow-style interface to
// the underlying task-tree system.

// Nomenclature: xdata_* for all types and functions, xd_* for macros.

// The shorthand macros (i.e. those of the form xd_<letter>) require that
// XDATA_STATE be defined to the variable name of the xdata_state for the task.

// Internal structure to allow for proper thread-safety. Do not use after the
// execution of the corrosponding task is complete.
typedef struct xdata_state xdata_state;

// Create a new piece of data, of the given size. For use inside Tasks.
void* xdata_create(xdata_state*, size_t size);

// Macros for xdata_create. Defines a variable that contains a pointer to a new
// piece of data, casted to the given type. Usage: xd_create(state, type, name);
#define xd_create(S, T, N) T* N = (T*)xdata_create(S, sizeof(T))
#define xd_C(T, N) xd_create(XDATA_STATE, T, N)

// The function signature for Tasks. It is assumed the number of inputs and outputs
// for the Task is defined ahead of time. If a Task completes without preparing
// a Task to write to a particular output, it is assumed to have written that
// output.
typedef void (*xdata_task)(void* xstate, xdata_state*,
	void* inputs[], void* outputs[]);

// Prepare a Task to be executed. It will execute once all of its inputs have
// been written.
void xdata_prepare(xdata_state*, xdata_task, int ninputs, void* inputs[],
	int noutputs, void* outputs[]);

// Macros for xdata_prepare. Usage: xd_p(state, func, {in1...}, {out1...});
#define xd_prep(S, F, INS, OUTS) ({ \
	void* ins[] = INS; \
	void* outs[] = OUTS; \
	xdata_prepare(S, F, sizeof(ins)/sizeof(ins[0]), ins, \
		sizeof(outs)/sizeof(outs[0]), outs); \
})
#define xd_P(F, I, O) xd_prep(XDATA_STATE, F, I, O)

// Run a Task (and subTasks) from outside a Task. Returns once the outputs have
// been written. Assumes the inputs have been written.
void xdata_run(xdata_task, xtask_config, int ninputs, void* inputs[],
	int noutputs, void* outputs[]);

// Macro for xdata_run. Usage: xd_r(f, config, {in1...}, {out1...});
#define xd_run(F, C, INS, OUTS) ({ \
	void* ins[] = INS; \
	void* outs[] = OUTS; \
	xdata_run(F, C, sizeof(ins)/sizeof(ins[0]), ins, \
		sizeof(outs)/sizeof(outs[0]), outs); \
})
#define xd_R(F, C, I, O) xd_run(F, C, I, O)
