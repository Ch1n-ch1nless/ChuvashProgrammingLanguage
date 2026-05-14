#pragma once

#include <string>
#include <utils/type_tuple.hpp>

namespace token {

// Подключаем списки лексем, поддерживаемых языком
#include "token_list/arithmetic_operators.data"
#include "token_list/built_in_types.data"
#include "token_list/grammar_tokens.data"
#include "token_list/identy_tokens.data"
#include "token_list/keywords.data"
#include "token_list/literals.data"
#include "token_list/logical_operators.data"

///////////////////////////////////////////////////////////////////////////////

// Разворачиваем лексемы в структуры

// Макрос для пустых лексем
#define EMPTY_TOKEN(type, token_name)                           \
  struct type {                                                 \
    friend bool operator==(const type&, const type&) = default; \
  };

#define EMPTY_TOKEN_LAST(type, token_name)                      \
  struct type {                                                 \
    friend bool operator==(const type&, const type&) = default; \
  };

// Макрос для лексем со значением
#define VALUE_TOKEN(type, valueType)                            \
  struct type {                                                 \
    friend bool operator==(const type&, const type&) = default; \
    valueType value;                                            \
  };

#define VALUE_TOKEN_LAST(type, valueType)                       \
  struct type {                                                 \
    friend bool operator==(const type&, const type&) = default; \
    valueType value;                                            \
  };

LOGICAL_OPERATORS
ARITHMETIC_OPERATORS
BUILD_IN_TYPES
KEYWORDS
GRAMMAR_TOKENS
LITERALS
IDENTY_TOKENS

#undef EMPTY_TOKEN
#undef EMPTY_TOKEN_LAST
#undef VALUE_TOKEN
#undef VALUE_TOKEN_LAST

///////////////////////////////////////////////////////////////////////////////

// Создаём концепты и типы для лексем
#define EMPTY_TOKEN(type, token_name) type,
#define EMPTY_TOKEN_LAST(type, token_name) type
#define VALUE_TOKEN(type, token_name) type,
#define VALUE_TOKEN_LAST(type, token_name) type

using LogicalOperators = TTuple<LOGICAL_OPERATORS>;
using ArithmeticOperators = TTuple<ARITHMETIC_OPERATORS>;
using Operators = Concat<ArithmeticOperators, LogicalOperators>::Result;
using KeyWords = TTuple<KEYWORDS>;
using BuildInTypes = TTuple<BUILD_IN_TYPES>;
using Literals = TTuple<LITERALS>;
using OtherTokens = TTuple<IDENTY_TOKENS, GRAMMAR_TOKENS>;

// clang-format off
using Tokens =
  Concat
  < Concat
    < Concat
      < Operators
      , Literals
      >::Result
    , Concat
      < BuildInTypes
      , KeyWords
      >::Result
    >::Result
  , OtherTokens
  >::Result;
// clang-format on

#undef EMPTY_TOKEN
#undef EMPTY_TOKEN_LAST
#undef VALUE_TOKEN
#undef VALUE_TOKEN_LAST

namespace concepts {

template <typename T>
concept IsToken = Contains<Tokens, T>::value == true;

template <typename T>
concept IsOperator = Contains<Operators, T>::value == true && IsToken<T>;

template <typename T>
concept IsLogicalOperator =
    Contains<LogicalOperators, T>::value == true && IsOperator<T>;

template <typename T>
concept IsLiteral = Contains<Literals, T>::value == true && IsToken<T>;

template <typename T>
concept IsKeyword = Contains<KeyWords, T>::value == true && IsToken<T>;

template <typename T>
concept IsBuildInType = Contains<BuildInTypes, T>::value == true && IsToken<T>;

}  // namespace concepts

using TokenVariant = TupleToVariant<Tokens>::Result;

///////////////////////////////////////////////////////////////////////////////

}  // namespace token
