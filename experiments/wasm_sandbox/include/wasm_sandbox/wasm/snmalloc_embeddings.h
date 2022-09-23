#pragma once
#include "WAVM/IR/Value.h"
#include "WAVM/Runtime/Intrinsics.h"

#include <iostream>

using namespace WAVM;
using namespace WAVM::IR;
using namespace WAVM::WASI;
using namespace WAVM::Runtime;

namespace sandbox
{
  // ———————————————————————— Embeddings for snmalloc —————————————————————————

  enum Embeddings
  {
    E_MALLOC = 0,
    E_FREE = 1,
    E_CALLOC = 2,
    E_MEMALIGN = 3,
    SIZE = 4,
  };

  const char* ENames[Embeddings::SIZE] = {
    "malloc",
    "free",
    "__libc_calloc",
    "posix_memalign",
  };

  const FunctionType intrinsic_types[Embeddings::SIZE] = {
    FunctionType({ValueType::i32}, {ValueType::i32}), // malloc
    FunctionType({ValueType::none}, {ValueType::i32}), // free
    FunctionType({ValueType::i32}, {ValueType::i32, ValueType::i32}), // calloc
    FunctionType(
      {ValueType::i32},
      {ValueType::i32, ValueType::i32, ValueType::i32}), // memalign
  };

  WAVM_DEFINE_INTRINSIC_MODULE(env)
  WAVM_DEFINE_INTRINSIC_FUNCTION(
    env, ENames[Embeddings::E_MALLOC], I32, env_malloc, I32 size)
  {
    std::cout << "in env_malloc " << size << std::endl;
    abort();
    return size;
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(
    env, ENames[Embeddings::E_FREE], void, env_free, I32 ptr)
  {
    std::cout << "in free " << ptr << std::endl;
    abort();
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(
    env, ENames[Embeddings::E_CALLOC], I32, env_calloc, I32 nmemb, I32 size)
  {
    std::cout << "in calloc " << std::endl;
    abort();
    return 0;
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(
    env,
    ENames[Embeddings::E_MEMALIGN],
    I32,
    env_memalign,
    I32 memptr,
    I32 alignment,
    I32 size)
  {
    std::cout << "in posix memalign" << std::endl;
    abort();
    return 0;
  }

} // namespace sandbox;
