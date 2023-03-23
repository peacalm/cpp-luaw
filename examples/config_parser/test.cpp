#include <peacalm/lua_wrapper.h>

#include <iostream>

int main() {
  peacalm::lua_wrapper l;

  // equals to: l.dostring("a = 1 b = math.pi c = 10^12 + 123 d = 'good'");
  l.dofile("conf.lua");

  int         a = l.get_int("a");
  double      b = l.get_double("b");
  long        c = l.get_llong("c");
  std::string d = l.get_string("d");
  std::cout << "a = " << a << std::endl;
  std::cout << "b = " << b << std::endl;
  std::cout << "c = " << c << std::endl;
  std::cout << "d = " << d << std::endl;
}