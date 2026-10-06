#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

enum class IntegerType
{
  Inferred, // 无后缀
  I32,
  U32,
  ISize,
  USize
};

inline IntegerType LiteralStringToIntegerType(const std::string &literal)
{
  if (literal.size() >= 3 && literal.compare(literal.size() - 3, 3, "i32") == 0)
  {
    return IntegerType::I32;
  }

  if (literal.size() >= 3 && literal.compare(literal.size() - 3, 3, "u32") == 0)
  {
    return IntegerType::U32;
  }

  if (literal.size() >= 5 &&
      literal.compare(literal.size() - 5, 5, "isize") == 0)
  {
    return IntegerType::ISize;
  }

  if (literal.size() >= 5 &&
      literal.compare(literal.size() - 5, 5, "usize") == 0)
  {
    return IntegerType::USize;
  }

  return IntegerType::Inferred;
}

inline std::uint64_t LiteralStringToInt(std::string literal)
{
  const std::string suffixes[] = {"i32", "u32", "isize", "usize"};
  for (const auto &suffix : suffixes)
  {
    if (literal.size() >= suffix.size() &&
        literal.compare(literal.size() - suffix.size(), suffix.size(),
                        suffix) == 0)
    {
      literal.erase(literal.size() - suffix.size());
      break;
    }
  }

  literal.erase(std::remove(literal.begin(), literal.end(), '_'),
                literal.end());

  int base = 10;
  std::size_t prefix_length = 0;
  if (literal.compare(0, 2, "0b") == 0)
  {
    base = 2;
    prefix_length = 2;
  }
  else if (literal.compare(0, 2, "0o") == 0)
  {
    base = 8;
    prefix_length = 2;
  }
  else if (literal.compare(0, 2, "0x") == 0)
  {
    base = 16;
    prefix_length = 2;
  }

  return std::stoull(literal.substr(prefix_length), nullptr, base);
}
