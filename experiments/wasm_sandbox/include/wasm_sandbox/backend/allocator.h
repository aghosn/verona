#pragma once

namespace sandbox
{
  class CustomAllocator
  {
  public:
    CustomAllocator() = default;

    void* alloc(size_t count)
    {
      abort();
    }

    void dealloc(void* ptr)
    {
      abort();
    }
  };
} // namespace sandbox;
