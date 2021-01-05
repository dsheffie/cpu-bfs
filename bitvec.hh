#ifndef __bitvec_hh__
#define __bitvec_hh__

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <iostream>

template <typename E>
class bitvec_template {
public:
  static const size_t bpw = 8*sizeof(E);
private:
  static const E all_ones = ~static_cast<E>(0);
  size_t n_bits = 0, n_words = 0;
  E *arr = nullptr;
public:
  bitvec_template(size_t n_bits = 64) : n_bits(n_bits), n_words((n_bits + bpw - 1) / bpw) {
    arr = new E[n_words];
    memset(arr, 0, sizeof(E)*n_words);
  }
  ~bitvec_template() {
    delete [] arr;
  }
  void clear() {
    memset(arr, 0, sizeof(E)*n_words);
  }
  size_t size() const {
    return static_cast<size_t>(n_bits);
  }
  void clear_and_resize(size_t n_bits) {
    delete [] arr;
    this->n_bits = n_bits;
    this->n_words = (n_bits + bpw - 1) / bpw;
    arr = new E[n_words];
    memset(arr, 0, sizeof(E)*n_words);
  }
  bool get_bit(size_t idx) const {
    size_t w_idx = idx / bpw;
    size_t b_idx = idx % bpw;
    return (arr[w_idx] >> b_idx)&0x1;
  }
  bool operator[](size_t idx) const {
    return get_bit(idx);
  }
  void set_bit(size_t idx) {
    assert(idx < n_bits);
    size_t w_idx = idx / bpw;
    size_t b_idx = idx % bpw;
    arr[w_idx] |= (1UL << b_idx);
  }
  void clear_bit(size_t idx) {
    assert(idx < n_bits);
    size_t w_idx = idx / bpw;
    size_t b_idx = idx % bpw;
    arr[w_idx] &= ~(1UL << b_idx);
  }
  void atomic_set_bit(size_t idx) {
    assert(idx < n_bits);
    size_t w_idx = idx / bpw;
    size_t b_idx = idx % bpw;
    __sync_fetch_and_or(&arr[w_idx], (1UL << b_idx));
  }
  void atomic_clear_bit(size_t idx) {
    assert(idx < n_bits);
    size_t w_idx = idx / bpw;
    size_t b_idx = idx % bpw;
    __sync_fetch_and_and(&arr[w_idx], ~(1UL << b_idx));
  }
  
  uint64_t popcount() const {
    uint64_t c = 0;
    assert((bpw*n_words) != n_bits);
    for(uint64_t w = 0; w < n_words; w++) {
      c += __builtin_popcountll(arr[w]);
    }
    return c;
  }
  E num_free() const {
    return n_bits - popcount();
  }
  int32_t find_first_unset() const {
    for(size_t w = 0; w < n_words; w++) {
      if(arr[w] == all_ones)
	continue;
      else if(arr[w]==0) {
	return bpw*w;
      }
      else {
	E x = ~arr[w];
	int64_t idx = bpw*w + (__builtin_ffsl(x)-1);
	if(idx < n_bits)
	  return idx;
      }
    }
    return -1;    
  }
  int32_t find_first_set() const {
    for(size_t w = 0; w < n_words; w++) {
      if(arr[w] == 0)
	continue;
      uint64_t idx = bpw*w + (__builtin_ffsl(arr[w])-1);
      return (idx < n_bits) ? static_cast<int32_t>(idx) : -1;
    }
    return -1;
  }
  
  int32_t find_next_set(int64_t idx) const {
    idx++;
    size_t w_idx = idx / bpw;
    size_t b_idx = idx % bpw;
    //check current word
    E ww =  (arr[w_idx] >> b_idx) << b_idx;
    if(ww != 0) {
      return w_idx*bpw + (__builtin_ffsl(ww)-1);
    }
    for(uint32_t w = w_idx+1; w < n_words; w++) {
      if(arr[w] == 0)
	continue;
      else {
	uint32_t idx = bpw*w + (__builtin_ffsl(arr[w])-1);
	if(idx < n_bits) {
	  return idx;
	}
	break;
      }
    }
    return -1;    
  }
};

typedef bitvec_template<uint32_t> bitvec;

#endif
