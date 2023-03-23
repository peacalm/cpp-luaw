
#include <peacalm/lua_wrapper.h>

#include <iostream>
#include <string>

int main() {
  peacalm::lua_wrapper l;
  l.set_integer("a", 10);
  l.set_integer("b", 5);
  l.set_integer("c", 2);
  std::string expr = "return a^2 + b/c";
  double      ret  = l.eval_double(expr);
  std::cout << "ret = " << ret << std::endl;  // 102.5
  std::string s =
      l.eval_string("if a > b + c then return 'good' else return 'bad' end");
  std::cout << "s = " << s << std::endl;  // good
}