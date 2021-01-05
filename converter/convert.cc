#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <cassert>
#include <fstream>
#include <set>
#include <cstring>
#include <map>
#include <algorithm>
#include <list>
#include <ctype.h>

inline uint32_t str_to_num(const char* str, ssize_t start, ssize_t end) {
  uint32_t num = 0, scale = 1;
  //std::cout << start << "," << end << "\n";
  for(ssize_t i = end; i >= start; --i) {
    num += (str[i]-48)*scale;
    scale *= 10;
  }
  return num;
}

static inline bool isnum(int c) {
  if(c < '0') return false;
  else if(c > '9') return false;
  return true;
}

bool parse(std::string &line, uint32_t &src, uint32_t &dst) {
  enum parse_state {SRC,WHITE,DST,DONE};
  parse_state state = parse_state::SRC;
  ssize_t p = 0;
  ssize_t num_start = 0;
  const char* c_str = line.c_str();
  ssize_t l = strlen(c_str);

  while((p <= l) and (state != parse_state::DONE)) {
    int c = c_str[p];
    switch(state)
      {
      case parse_state::SRC:
	if(isnum(c)==false) {
	  src = str_to_num(c_str, num_start, p-1);
	  state = parse_state::WHITE;
	}
	break;
      case parse_state::WHITE:
	if(isnum(c)) {
	  state = parse_state::DST;
	  //std::cout << "moving to DST state @ " << p << "\n";
	  num_start = p;
	}
	break;
      case parse_state::DST:
	//std::cout << "c = " << (int)c << "\n";	
	if(isnum(c)==false) {
	  dst = str_to_num(c_str, num_start, p-1);
	  //std::cout << "in terminal state\n";
	  //std::cout << "num_start = " << num_start << "\n";
	  //std::cout << "p = " << p << "\n";
	  state = parse_state::DONE;
	}
      	break;
      case parse_state::DONE:
	break;
      }
    p++;
  }
  //std::cout << c_str << "\n";
  //std::cout << "src = " << src << ", dst = " << dst << "\n";
  
  return (state == parse_state::DONE);
}

static const uint32_t magic = 0xdeadbeef;

int main(int argc, char *argv[]) {
  std::map<uint32_t, std::list<uint32_t>> g;
  uint32_t src, dst;
  std::string line;
  
  std::ifstream myfile (argv[1]);
  if (not(myfile.is_open()))
    return -1;
  int c = 0;
  std::set<uint32_t> vertices;
  uint32_t max_vertex = 0;
  uint32_t n_edges = 0;
  while (std::getline (myfile,line) ) {
    if(line[0] == '#')
      continue;
    bool d = parse(line, src, dst);
    if(d) {
      vertices.insert(src);
      vertices.insert(dst);
      g[src].push_back(dst);
      n_edges++;
      max_vertex = std::max(max_vertex, src);
      max_vertex = std::max(max_vertex, dst);
    }
  }
  for(auto v : g) {
    std::list<uint32_t> &edge_list = v.second;
    edge_list.sort();
  }
  myfile.close();

  
  uint32_t n_vertices = vertices.size();
  std::cout << "found " << n_vertices << " vertices\n";
  std::cout << "found " << n_edges << " edges\n";
  std::cout << "max vertex " << max_vertex << "\n";
  
  uint32_t *edge_offs = new uint32_t[n_vertices+1];
  uint32_t *edges = new uint32_t[n_edges];

  
  edge_offs[0] = 0;
  for(uint32_t v = 1; v <= (max_vertex+1); v++) {
    edge_offs[v] = edge_offs[v-1] + g[v-1].size();
  }
  uint32_t p = 0;
  for(uint32_t v = 0; v < max_vertex; v++) {
    std::list<uint32_t> &edge_list = g[v];
    for(uint32_t e : edge_list) {
      edges[p++] = e;
    }
  }
  
  FILE *fp = fopen("graph.bin", "wb");
  fwrite(&magic, sizeof(magic), 1, fp);
  fwrite(&n_vertices, sizeof(n_vertices), 1, fp);
  fwrite(&n_edges, sizeof(n_edges), 1, fp);
  fwrite(edge_offs, sizeof(uint32_t), (n_vertices+1), fp);
  fwrite(edges, sizeof(uint32_t), n_edges, fp);
  fclose(fp);
  delete [] edges;
  delete [] edge_offs;
  return 0;
}
