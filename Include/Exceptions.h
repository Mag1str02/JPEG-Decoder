#pragma once

#include "STDInclude.h"

struct SectionError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct DataError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

#define THROW_IF(expr, message) \
    if (expr)                   \
    throw std::runtime_error(message)
#define DATA_ERROR_IF(expr, message) \
    if (expr)                        \
    throw DataError(message)
#define INVALID_ARGUMENT_IF(expr, message) \
    if (expr)                              \
    throw std::invalid_argument(message)
#define SECTION_ERROR_IF(expr, message) \
    if (expr)                           \
    throw SectionError(message)