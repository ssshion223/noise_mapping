#pragma once
#include <vector>
#include <iostream>
#include <atomic>
class RingBuffer {
public:
  explicit RingBuffer(size_t capacity);
  ~RingBuffer();

public: //public function
  bool push(const float *data, size_t n);
  size_t pop(std::vector<float> &out, size_t n);

private: //private memeber
  std::vector<float> buffer_;
  size_t capacity_;
  std::atomic<size_t> head_;
  std::atomic<size_t> tail_;
};
