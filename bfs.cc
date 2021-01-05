#include <limits>
#include <list>
#include <map>
#include <set>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>

#include "bitvec.hh"
#include "graph.hh"


uint32_t bfs_v2(uint32_t src, const graph *g) {
  bitvec visited(g->n_vertices);
  uint32_t *frontier = nullptr;
  uint32_t curr_start = 0, curr_cnt = 0;
  uint32_t next_start = 0, next_cnt = 0;

  frontier = new uint32_t[g->n_vertices];
  frontier[next_cnt++] = src;

  visited.set_bit(src);

  while(next_cnt != 0) {
    curr_cnt = next_cnt;
    curr_start = next_start;
    next_cnt = 0;
    next_start = (curr_start + curr_cnt) % g->n_vertices;
    //std::cout << "curr_cnt = " << curr_cnt << ",curr_start = " << curr_start << "\n";
    //std::cout << "next_cnt = " << next_cnt << ",next_start = " << next_start << "\n";
    for(uint32_t ii = 0; ii < curr_cnt; ++ii) {
      //uint32_t i = (ii + curr_start) % g->n_vertices;
      uint32_t i = wrap((ii + curr_start),g->n_vertices);
      uint32_t u = frontier[i];
      for(uint32_t j = g->edge_offs[u], jj = g->edge_offs[u+1]; j < jj; ++j) {
	uint32_t v = g->edges[j];
	if(not(visited[v])) {
	  //uint32_t k = (next_cnt + next_start) % g->n_vertices;
	  uint32_t k = wrap((next_cnt + next_start),g->n_vertices);
	  frontier[k] = v;
	  visited.set_bit(v);
	  next_cnt++;
	}
      }
    }
  }
  delete [] frontier;
  return visited.popcount();
}


uint32_t bfs_v3(uint32_t src, const graph *g) {
  bitvec visited(g->n_vertices);
  uint32_t n = next_pow2(g->n_vertices);
  uint32_t *frontier = nullptr;
  uint32_t curr_start = 0, curr_cnt = 0;
  uint32_t next_start = 0, next_cnt = 0;

  frontier = new uint32_t[n];
  frontier[next_cnt++] = src;

  visited.set_bit(src);

  while(next_cnt != 0) {
    curr_cnt = next_cnt;
    curr_start = next_start;
    next_cnt = 0;
    next_start = (curr_start + curr_cnt) & (n-1);
    for(uint32_t ii = 0; ii < curr_cnt; ++ii) {
      uint32_t i = (ii + curr_start) & (n-1);
      uint32_t u = frontier[i];
      for(uint32_t j = g->edge_offs[u], jj = g->edge_offs[u+1]; j < jj; ++j) {
	uint32_t v = g->edges[j];
	if(not(visited[v])) {
	  uint32_t k = (next_cnt + next_start) & (n-1);
	  frontier[k] = v;
	  visited.set_bit(v);
	  next_cnt++;
	}
      }
    }
  }
  delete [] frontier;
  return visited.popcount();
}

struct bfs_t {
  graph *g;
  bitvec *visited;
  pthread_barrier_t b;
  uint32_t *c0 ;
  uint32_t *q0 ;
  uint32_t *c1 ;
  uint32_t *q1 ;
  uint32_t count __attribute__((aligned(64)));
  uint32_t epoch __attribute__((aligned(64)));
};

static bfs_t bb;
uint32_t nthr = 1;

static void *bfs_worker(void *args) {
  size_t tid = reinterpret_cast<size_t>(args);
  bitvec *visited = bb.visited;
  const graph *g = bb.g;
  uint32_t *c0 = bb.c0;
  uint32_t *c1 = bb.c1;
  uint32_t *q0 = bb.q0;
  uint32_t *q1 = bb.q1;
  
  while(*c0 != 0) {
    for(uint32_t ii = tid, in_count = *c0; ii < in_count; ii+=nthr) {
      uint32_t u = q0[ii];
      for(uint32_t j = g->edge_offs[u], jj = g->edge_offs[u+1]; j < jj; ++j) {
	uint32_t v = g->edges[j];
	if(visited->get_bit(v) == 0) {
	  visited->atomic_set_bit(v);
	  uint32_t p = __sync_fetch_and_add(c1, 1);
	  q1[p] = v;
	}
      }
    }
    std::swap(c0, c1);
    std::swap(q0, q1);
    pthread_barrier_wait(&(bb.b));
    if(tid==0) *c1 = 0;
    pthread_barrier_wait(&(bb.b));
  }
  pthread_exit(nullptr);
}

static void *bfs_worker_local(void *args) {
  static const uint32_t LOCAL_SZ = 64;

  uint32_t lb_sz = 0;
  uint32_t lb[LOCAL_SZ];
  
  size_t tid = reinterpret_cast<size_t>(args);
  bitvec *visited = bb.visited;
  const graph *g = bb.g;
  uint32_t *c0 = bb.c0;
  uint32_t *c1 = bb.c1;
  uint32_t *q0 = bb.q0;
  uint32_t *q1 = bb.q1;
  
  while(*c0 != 0) {
    for(uint32_t ii = tid, in_count = *c0; ii < in_count; ii+=nthr) {
      uint32_t u = q0[ii];
      for(uint32_t j = g->edge_offs[u], jj = g->edge_offs[u+1]; j < jj; ++j) {
	uint32_t v = g->edges[j];
	if(visited->get_bit(v) == 0) {
	  visited->atomic_set_bit(v);
	  //flush if overflow
	  if(lb_sz == LOCAL_SZ) {
	    uint32_t p = __sync_fetch_and_add(c1, LOCAL_SZ);
	    for(uint32_t k = 0; k < LOCAL_SZ; k++) {
	      q1[p+k] = lb[k];
	    }
	    lb_sz = 0;
	  }
	  lb[lb_sz++] = v;
	}
      }
    }
    //flush before next iteration
    if(lb_sz != 0) {
      uint32_t p = __sync_fetch_and_add(c1, lb_sz);
      for(uint32_t k = 0; k < lb_sz; k++) {
	q1[p+k] = lb[k];
      }
      lb_sz = 0;
    }
    std::swap(c0, c1);
    std::swap(q0, q1);

    int next = bb.epoch + 1;
    if(__sync_sub_and_fetch(&bb.count, 1) == 0) {
      bb.count = nthr;
      *c1 = 0;
      __sync_synchronize();
      bb.epoch = next;
    }
    else {
      volatile uint32_t* next_epoch = &(bb.epoch);
      while(*next_epoch != next) {}
    }
    
#if 0
    pthread_barrier_wait(&(bb.b));
    if(tid==0) *c1 = 0;
    pthread_barrier_wait(&(bb.b));
#endif
  }
  pthread_exit(nullptr);
}


uint32_t bfs_thr(uint32_t src, const graph *g) {
  bitvec visited(g->n_vertices);
  pthread_t *thr = new pthread_t[nthr];
  uint32_t c0 = 0;
  uint32_t c1 = 0;
  uint32_t *q0 = new uint32_t[g->n_vertices];
  uint32_t *q1 = new uint32_t[g->n_vertices];

  bb.g = const_cast<graph*>(g);
  bb.c0 = &c0;
  bb.c1 = &c1;
  bb.q0 = q0;
  bb.q1 = q1;
  bb.visited = &visited;
  pthread_barrier_init(&(bb.b), nullptr, nthr);
  bb.count = nthr;
  
  c0 = 1;
  q0[0] = src;

  for(uint32_t i = 0; i < nthr; i++) {
    pthread_create(&thr[i], nullptr, bfs_worker_local, reinterpret_cast<size_t*>(i));
  }
  for(int i = 0; i < nthr; i++) {
    pthread_join(thr[i], nullptr);
  }
  pthread_barrier_destroy(&(bb.b));
  delete [] q0;
  delete [] q1;
  delete [] thr;

  return visited.popcount();
}

uint32_t stl_bfs(uint32_t src, const graph *g) {
  std::set<uint32_t> v;
  std::list<uint32_t> q;

  q.push_back(src);

  while(not(q.empty())) {
    uint32_t x = q.front();
    q.pop_front();
    if(v.find(x) != v.end())
      continue;
    v.insert(x);
    for(uint32_t j = g->edge_offs[x], jj = g->edge_offs[x+1]; j < jj; ++j) {
      uint32_t y = g->edges[j];
      q.push_back(y);
    }
  }
  return static_cast<uint32_t>(v.size());
}


uint32_t bfs(uint32_t src, const graph *g) {
  bitvec visited(g->n_vertices);
  uint32_t *curr_frontier = nullptr;
  uint32_t *next_frontier = nullptr;
  uint32_t curr_cnt = 0;
  uint32_t next_cnt = 0;

  curr_frontier = new uint32_t[g->n_vertices];
  next_frontier = new uint32_t[g->n_vertices];
  curr_frontier[curr_cnt++] = src;
  visited.set_bit(src);
  
  while(curr_cnt != 0) {
    next_cnt = 0;
    //std::cout << "curr_cnt = " << curr_cnt << "\n";
    for(uint32_t i = 0; i < curr_cnt; ++i) {
      uint32_t u = curr_frontier[i];
      for(uint32_t j = g->edge_offs[u], jj = g->edge_offs[u+1]; j < jj; ++j) {
	uint32_t v = g->edges[j];
	if(not(visited[v])) {
	  next_frontier[next_cnt++] = v;
	  visited.set_bit(v);
	}
      }
    }
    std::swap(curr_cnt, next_cnt);
    std::swap(curr_frontier, next_frontier);
  }
  delete [] curr_frontier;
  delete [] next_frontier;
  return visited.popcount();
}

