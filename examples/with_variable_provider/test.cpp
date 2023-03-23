#include <peacalm/lua_wrapper.h>

#include <cstdio>
#include <iostream>
#include <memory>

struct provider {
  provider() { puts("provider()"); }
  ~provider() { puts("~provider()"); }
  void provide_variables(const std::vector<std::string> &vars,
                         peacalm::lua_wrapper           *l) {
    for (const auto &v : vars) provide_one(v, l);
  }
  void provide_one(const std::string &v, peacalm::lua_wrapper *l) {
    if (v == "a")
      l->set_integer(v, 1);
    else if (v == "b")
      l->set_integer(v, 2);
    else if (v == "c")
      l->set_integer(v, 3);
    else
      std::cout << "unknown: " << v << std::endl;
  }
};
using provider_type = std::unique_ptr<provider>;

int main() {
  peacalm::lua_wrapper_has_provider<provider_type> l;
  l.provider(std::make_unique<provider>());
  double ret = l.auto_eval_double("return a*10 + b^c");
  std::cout << ret << std::endl;
}
