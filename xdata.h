#include "xtask.h"
#include <stdlib.h>

// XData is a layer on top of XTask which provides a dataflow-style interface to
// the underlying task-tree system.

// There are two styles for the API, the xdata_* style and the xd_* style.
// The xdata_* style is more mundane and works stably in all case.
// The xd_* style is shorter and is for end-user code written on XData.
// Often the xd_* style will be implemented as a macro using the xdata_* style.

// The xd_* macros require that XD_STATE be defined and expand to the name of
// the xdata_state* variable in the context of usage. xd_O and xd_F also require
// that XD_OUT expand to the xdata_line argument of the task.

#ifndef XDATA_H_
#define XDATA_H_

// Names for some of the parameters for tasks. Can be re-def'd without issue.
#define XD_STATE _xdata_state_param
#define XD_OUT _xdata_out_param

// The internal state of the currently running task. Do not transfer between tasks.
typedef struct xdata_state xdata_state;

// A "line" that connects tasks, whose data can be written or read by new tasks.
// Don't try to create too many of these
typedef size_t xdata_line;

// The function signature for a task. Number of inputs is assumed to be known.
typedef void (*xdata_task)(void* xstate, xdata_state*,
	xdata_line out, void* in[]);

// Usage: xd_F(<name>, <xstate>, <in>) (; or {...})
// Define a task, using the defined names.
#define xd_F(N, X, I) void N (void* X, xdata_state* XD_STATE, \
	xdata_line XD_OUT, void* I[])

// Create a line, with the given size of data.
xdata_line xdata_create(xdata_state*, size_t);

// Copy some data into a line, useful for constants and size-markers.
void xdata_setdata(xdata_state*, xdata_line, void*);

// Usage: xd_C(<type>, <var>); or xd_CV(<type>, <var>, <membervalues>...);
// Defines a line <var> with enough space to hold <type>. If specified,
// <membervalues> are wrapped in braces and form the initializer for the line.
#define xd_C(T, V) xdata_line V = xdata_create((XD_STATE), sizeof(T));
#define xd_CV(T, V, ...) xd_C(T, V); ({ \
	T _val = {__VA_ARGS__}; \
	xdata_setdata((XD_STATE), (V), &_val); \
})

// Usage: xd_O(<type>, <membervalues>...);
// Like xd_CV, but uses the output line instead of creating a line.
#define xd_O(T, ...) ({ \
	T _val = {__VA_ARGS__}; \
	xdata_setdata((XD_STATE), (XD_OUT), &_val); \
})

// Prepare a task, which will be spawned and executed sometime in the future.
void xdata_prepare(xdata_state*, xdata_task, xdata_line out,
	int numinputs, xdata_line in[]);

// Usage: xd_P(<out>, <func>, <in>...); or xd_P0(<out>, <func>);
// Prepares a task, <in>s and <out> are line references (ie. XD_PREFIX##V)
#define xd_P(O, F, ...) ({ \
	xdata_line _ins[] = {__VA_ARGS__}; \
	xdata_prepare((XD_STATE), (F), (O), sizeof(_ins)/sizeof(_ins[0]), _ins); \
})
#define xd_P0(O, F) ( xdata_prepare((XD_STATE), (F), (O), 0, NULL) )

// From outside of a task, fire up the system and run a task.
void xdata_run(xdata_task, xtask_config, size_t, void* out, int numin, void* in[]);

// Usage: xd_R(<cfg>, <out>, <func>, <in>...); or xd_R0(<cfg>, <out>, <func>);
// Runs a task from outside the system, <in>s and <out> are pointers to data.
#define xd_R(C, O, F, ...) ({ \
	void* _ins[] = {__VA_ARGS__}; \
	xdata_run((F), (C), sizeof(*O), (O), sizeof(_ins)/sizeof(_ins[0]), _ins); \
})
#define xd_R0(C, O, F) ( xdata_run((F), (C), (O), 0, NULL) )

#endif
