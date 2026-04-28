#include "ringbuffer.hpp"


RingBuffer::RingBuffer(size_t capacity): buffer_(capacity), capacity_(capacity), head_(0), tail_(0) {}
RingBuffer::~RingBuffer(){}
bool RingBuffer::push(const float *data, size_t n){
  size_t head = head_.load(std::memory_order_relaxed);
  size_t tail = tail_.load(std::memory_order_acquire);
  for (size_t i = 0; i < n; i++) {
    size_t next = (head + 1) % capacity_;
    if (next == tail) {
      return false;
    }
    buffer_[head] = data[i];
    head = next;
  }
  head_.store(head, std::memory_order_release);
  return true;
}

size_t RingBuffer::pop(std::vector<float> &out, size_t n){
  size_t count = 0;
  if (out.size() < n) {
    out.resize(n);
  }
  size_t head = head_.load(std::memory_order_acquire);
  size_t tail = tail_.load(std::memory_order_relaxed);
  while (tail != head && count < n) {
    out[count] = buffer_[tail];
    size_t next = (tail + 1) % capacity_;
    tail = next;
    count++;
  }
  tail_.store(tail, std::memory_order_release);
  return count;
}