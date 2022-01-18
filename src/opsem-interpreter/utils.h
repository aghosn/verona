#pragma once

#include <map>
#include <memory>
#include <vector>
#include <verona.h>

#include "ir.h"

namespace rt = verona::rt;

namespace ir = verona::ir;

template<typename T>
using List = std::vector<T>;

template<typename T, typename U>
using Map = std::map<T, U>;

using Id = std::string;

template<typename T>
using Shared = std::shared_ptr<T>;
