#pragma once

#include <stddef.h>
#include <stdint.h>

class HalAudioOut {
 public:
  virtual ~HalAudioOut() = default;
  virtual bool begin() = 0;
  virtual bool write_samples(const uint8_t* data, size_t len) = 0;
  virtual void end() = 0;
};

