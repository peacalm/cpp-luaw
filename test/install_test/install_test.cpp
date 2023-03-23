#include <peacalm/lua_wrapper.h>

#include <iostream>

int main() {
  peacalm::lua_wrapper l;
  std::cout << l.eval_string("return 'Hello ' .. 'LuaWrapper!'") << std::endl;
  return 0;
}
