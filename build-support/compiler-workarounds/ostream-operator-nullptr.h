#ifndef OSTREAM_OPERATOR_NULLPTR_H
#define OSTREAM_OPERATOR_NULLPTR_H

#include <iostream>
#include <cstddef>

namespace std
{
inline std::ostream &operator<< (std::ostream &os, std::nullptr_t ptr)
{
  return os << "nullptr";   //whatever you want nullptr to show up as in the console
}
}

#endif //OSTREAM_OPERATOR_NULLPTR_H
