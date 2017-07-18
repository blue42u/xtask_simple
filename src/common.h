#ifndef COMMON_H_
#define COMMON_H_

// Common operations used by Queues

// Count the non-leaves in a task-tree
inline static int rcount(xtask_task* tt) {
	int cnt = 0;
	for(xtask_task* t = tt; t; t = t->sibling)
		if(t->child) cnt += 1 + rcount(t->child);
	return cnt;
}

// Replace default values in <in> with the values from <def>
inline static void defaults(xtask_config* in, xtask_config def) {
#define def(V) if(in->V == 0) in->V = def.V
	def(workers);
	def(max_leafing);
	def(max_tailing);
	def(init);
	def(dest);
}

#endif

