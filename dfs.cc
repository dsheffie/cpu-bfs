#include <limits>
#include <list>
#include <map>
#include <set>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "bitvec.hh"
#include "graph.hh"

uint32_t dfs(uint32_t src, const graph *g) {
  bitvec visited(g->n_vertices);
  uint32_t *stack = nullptr;
  uint32_t tos = g->n_vertices;

  stack = new uint32_t[g->n_vertices];
  stack[--tos] = src;
  visited.set_bit(src);

  while(tos != g->n_vertices) {

    uint32_t u = stack[tos++];
    for(uint32_t j = g->edge_offs[u], jj = g->edge_offs[u+1]; j < jj; ++j) {
	uint32_t v = g->edges[j];
	if(not(visited[v])) {
	  visited.set_bit(v);
	  stack[--tos] = v;
	}
    }
  }
  delete [] stack;
  return visited.popcount();
}
