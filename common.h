#ifndef COMMON_H_
#define COMMON_H_

// Common operations used by Queues

// Count the non-leaves in a task-tree
inline static void rcount(xtask_task* tt, int* cnt) {
	for(xtask_task* t = tt; t; t = t->sibling) {
		if(t->child) (*cnt)++;
		rcount(t->child, cnt);
	}
}

// Replace default values in <in> with the values from <def>
inline static xtask_config defaults(xtask_config in, xtask_config def) {
#define def(V) .V = in.V ? in.V : def.V
	return (xtask_config){
		def(workers),
		def(max_leafing),
		def(max_tailing),
		def(init),
		def(dest)
	};
}

#endif

