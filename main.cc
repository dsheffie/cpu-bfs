// -*- c++ -*-
#include <limits>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sys/time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "bitvec.hh"
#include "graph.hh"


static inline double timeval_to_sec(struct timeval &t) {
  return t.tv_sec + 1e-6 * static_cast<double>(t.tv_usec);
}

static std::ostream &operator <<(std::ostream &out, struct rusage &usage) {
  out << "user = " << timeval_to_sec(usage.ru_utime) << "s,"
      << "sys = " << timeval_to_sec(usage.ru_stime) << "s,"
      << "mss = " << usage.ru_maxrss << " kbytes";
  return out;
}


static double timestamp() {
  struct timeval t;
  gettimeofday(&t, nullptr);
  return timeval_to_sec(t);
}

extern uint32_t nthr;

bool graph::sanity_check() const {
  for(uint32_t i = 0; i < n_vertices; i++) {
    uint32_t s = edge_offs[i], e = edge_offs[i+1];
    if((e-s)==0)
      continue;
    uint32_t m = edges[s];
    for(uint32_t j = 1; j < (e-s); j++) {
      if(edges[j+s] < m) {
	printf("edge ordering broken\n");
	return false;
	      
      }
      m = edges[j+s];
    }
  }
  return true;
}

int main(int argc, char *argv[]) {
  int c;
  bool need_bswap = false;
  uint32_t magic, n_vertices, n_edges;
  FILE *fp = nullptr; 
  size_t rc;
  graph *g = nullptr;  
  std::string g_src = "graph.bin";
  uint32_t vv = 0;
  double now = 0;
  struct rusage usage;
  
  while ((c = getopt (argc, argv, "g:t:")) != -1) {
    switch(c)
      {
      case 'g':
	g_src = std::string(optarg);
	break;
      case 't':
	nthr = atoi(optarg);
	break;
      default:
	break;
      }
  }
  now = timestamp();
  fp = fopen(g_src.c_str(), "rb");
  assert(fp != nullptr);
  rc = fread(&magic, sizeof(magic), 1, fp);

  if(magic == 0xdeadbeef) {
    need_bswap = false;
  }
  else if(byteSwap(magic) == 0xdeadbeef) {
    need_bswap = true;
  }
  else {
    fclose(fp);
    return -1;
  }
    
  rc = fread(&n_vertices, sizeof(n_vertices), 1, fp);
  rc = fread(&n_edges, sizeof(n_edges), 1, fp);

  if(need_bswap) {
    n_vertices = byteSwap(n_vertices);
    n_edges = byteSwap(n_edges);
  }
  
  g = new graph(n_vertices, n_edges);
  rc = fread(g->edge_offs, sizeof(uint32_t), (g->n_vertices+1), fp);
  rc = fread(g->edges, sizeof(uint32_t), g->n_edges, fp);
  fclose(fp);
  
  if(need_bswap) {
    for(uint32_t i = 0; i <= n_vertices; i++) {
      g->edge_offs[i] = byteSwap(g->edge_offs[i]);
    }
    for(uint32_t i = 0; i < n_edges; i++) {
      g->edges[i] = byteSwap(g->edges[i]);
    }
  }
  now = timestamp() - now;
  std::cout << "graph load took " << now << " seconds\n";
  std::cout << "n_vertices = " << n_vertices << "\n";
  std::cout << "n_edges = " << n_edges << "\n";

  if(not(g->sanity_check())) {
    return -1;
  }

  for(int root  = 0; root < n_vertices; root++) {
    now = timestamp();
    int v0 = bfs_v2(root, g);
    now = timestamp() - now;
    std::cout << "bfs_v2 took " << now << " seconds\n";
    
    now = timestamp();
    int v1 = bfs_avx512(root, g, 128);
    now = timestamp() - now;
    std::cout << "xmm bfs_avx512 took " << now << " seconds\n";
    
    now = timestamp();
    int v2 = bfs_avx512(root, g, 256);
    now = timestamp() - now;
    std::cout << "ymm bfs_avx512 took " << now << " seconds\n";
    

    now = timestamp();
    int v3 = bfs_avx512(root, g, 512);
    now = timestamp() - now;
    std::cout << "zmm bfs_avx512 took " << now << " seconds\n";
       
    if(v0 != v1 or v0 != v2 or v0 != v3) {
      printf("v0 = %d, v1 = %d, v2 = %d, v3 = %d\n", v0, v1,v2,v3);
      std::cout << "avx512 error\n";
      break;
    }
  }
  
  getrusage(RUSAGE_SELF,&usage);
  std::cout << usage << "\n";
  delete g;
  
  return 0;
}
