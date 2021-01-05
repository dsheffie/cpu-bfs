#ifndef __graphhh__
#define __graphhh__

#include <cstdint>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <sys/mman.h>

#ifdef __linux__
#define USE_MMAP
#endif

struct graph {
  uint32_t n_vertices;
  uint32_t n_edges;
  uint32_t *edge_offs;
  uint32_t *edges;

  graph(uint32_t n_vertices, uint32_t n_edges) :
    n_vertices(n_vertices), n_edges(n_edges) {

#ifdef USE_MMAP
#define PROT (PROT_READ | PROT_WRITE)
#define MAP (MAP_ANONYMOUS|MAP_PRIVATE|MAP_POPULATE)
    auto p0  = mmap(nullptr, sizeof(uint32_t)*(n_vertices+1), PROT, MAP|MAP_HUGETLB, -1, 0);
    auto p1  = mmap(nullptr, sizeof(uint32_t)*(n_edges), PROT, MAP|MAP_HUGETLB, -1, 0);
    edge_offs = reinterpret_cast<uint32_t*>(p0);
    edges = reinterpret_cast<uint32_t*>(p1);
#else
    edge_offs = new uint32_t[n_vertices+1];
    edges = new uint32_t[n_edges];
#endif

    memset(edge_offs,0,sizeof(uint32_t)*(n_vertices+1));
  }

  ~graph() {
#ifdef USE_MMAP
    munmap(edge_offs, sizeof(uint32_t)*(n_vertices+1));
    munmap(edges, sizeof(uint32_t)*n_edges);
#else
    delete [] edge_offs;
    delete [] edges;
#endif
  }  
};

template <typename T, typename std::enable_if<std::is_integral<T>::value, T>::type* = nullptr>
T wrap(T x, T c) {
  return x >= c ? 0 : x;
}

template <typename T, typename std::enable_if<std::is_integral<T>::value, T>::type* = nullptr>
T next_pow2(T x) {
  T y = 1;
  while(y < x) {
    y *= 2;
  }
  return y;
}

#define BS_PRED(SZ) (std::is_integral<T>::value && (sizeof(T)==SZ))
template <typename T, typename std::enable_if<BS_PRED(1),T>::type* = nullptr>
T byteSwap(T x) {
  return x;
}
template <typename T, typename std::enable_if<BS_PRED(2),T>::type* = nullptr>
T byteSwap(T x) {
  return __builtin_bswap16(x);
}
template <typename T, typename std::enable_if<BS_PRED(4),T>::type* = nullptr> 
T byteSwap(T x) {
  return __builtin_bswap32(x);
}
template <typename T, typename std::enable_if<BS_PRED(8),T>::type* = nullptr>
T byteSwap(T x) {
  return __builtin_bswap64(x);
}
#undef BS_PRED

uint32_t stl_bfs(uint32_t src, const graph *g);
uint32_t bfs(uint32_t src, const graph *g);
uint32_t dfs(uint32_t src, const graph *g);
uint32_t bfs_v2(uint32_t src, const graph *g);
uint32_t bfs_v3(uint32_t src, const graph *g);
uint32_t bfs_thr(uint32_t src, const graph *g);

#endif
