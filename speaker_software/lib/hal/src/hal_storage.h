#pragma once

#include <stddef.h>
#include <stdint.h>

class HalStorage {
 public:
  virtual ~HalStorage() = default;
  virtual bool begin() = 0;
  virtual bool open_wav(const char* path) = 0;
  virtual size_t read_bytes(uint8_t* buffer, size_t max_len) = 0;
  virtual void close() = 0;
};

