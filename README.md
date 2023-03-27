# Lua Wrapper for C++

[![Build](https://github.com/peacalm/cpp-lua_wrapper/actions/workflows/ci.yml/badge.svg)](https://github.com/peacalm/cpp-lua_wrapper/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

This library is a simple wrapper of Lua which makes interaction between Lua and 
C++ easier.

* Lua version >= 5.4
* C++ version >= C++14

Features:
* Set or get values from Lua
* Eval string Lua expression in C++ to get values
* Detect variabls needed in expression
* If a variable provider is provided, it can automatically seek variabls while
evalurate expressions.

**Notice**: Type conversions may different to Lua!

We mainly use C++'s type conversion strategy, in addition, 
a conversion strategy between number (float number) and 
number-literal-string, which is supported by Lua.

Details:
1. Implicitly conversion between integer, number, boolean using
   C++'s static_cast
2. Implicitly conversion between number and number-literal-string by Lua
3. When convert number 0 to boolean, will get false (not true as Lua does)
4. NONE and NIL won't convert to any value, default will be returned
5. Non-number-literal-string, including empty string, can't convert to any
   other types, default will be returned
6. Integer's precision won't lost if it's value is representable by 64bit
   signed integer type, i.e. between [-2^63, 2^63 -1], which is
   [-9223372036854775808, 9223372036854775807]
7. When conversion fails, return default value, and print an error log.

Examples:
- number 2.5 -> string "2.5" (By Lua)
- string "2.5" -> double 2.5 (By Lua)
- double 2.5 -> int 2 (By C++)
- string "2.5" -> int 2 (Firstly "2.5"->2.5 by lua, then 2.5->2 by C++)
- bool true -> int 1 (By C++)
- int 0 -> bool false (By C++)
- double 2.5 -> bool true (By C++)
- string "2.5" -> bool true ("2.5"->2.5 by Lua, then 2.5->true by C++)

## Usage Examples

### As a config file parser
suppose config file is "conf.lua":
```C++
#include <iostream>
#include <peacalm/lua_wrapper.h>
int main() {
  peacalm::lua_wrapper l;

  // equals to: l.dostring("a = 1 b = math.pi c = 10^12 + 123 d = 'good'");
  l.dofile("conf.lua"); 

  int a = l.get_int("a");
  double b = l.get_double("b");
  long c = l.get_llong("c");
  std::string d = l.get_string("d");
  std::cout << "a = " << a << std::endl;
  std::cout << "b = " << b << std::endl;
  std::cout << "c = " << c << std::endl;
  std::cout << "d = " << d << std::endl;
  std::cout << "nx = " << l.get_int("nx", -1) << std::endl;
}
```
Output:
```txt
a = 1
b = 3.14159
c = 1000000000123
d = good
nx = -1
```

### As a dynamic expression evaluator
```C++
peacalm::lua_wrapper l;
l.set_integer("a", 10);
l.set_integer("b", 5);
l.set_integer("c", 2);
std::string expr = "return a^2 + b/c";
double ret = l.eval_double(expr);
std::cout << "ret = " << ret << std::endl; // 102.5
std::string s = l.eval_string("if a > b + c then return 'good' else return 'bad' end");
std::cout << "s = " << s << std::endl; // good
```

### With variable provider
```C++
struct provider {
  bool provide(lua_State *L, const char *vname) {
    if (strcmp(vname, "a") == 0)
      lua_pushinteger(L, 1);
    else if (strcmp(vname, "b") == 0)
      lua_pushinteger(L, 2);
    else if (strcmp(vname, "c") == 0)
      lua_pushinteger(L, 3);
    else
      return false;
    // If variables won't change, could set them to global:
    // lua_pushvalue(L, -1);
    // lua_setglobal(L, vname);
    return true;
  }
};
using provider_type = std::unique_ptr<provider>;
int main() {
  peacalm::custom_lua_wrapper<provider_type> l;
  l.provider(std::make_unique<provider>());
  double ret = l.eval_double("return a*10 + b^c");
  std::cout << ret << std::endl;  // 18
}
```

## Build and Install
Install Lua first:
```bash
curl -R -O http://www.lua.org/ftp/lua-5.4.4.tar.gz
tar zxf lua-5.4.4.tar.gz
cd lua-5.4.4
make all test
sudo make install
```

Build then install:
```bash
git clone https://github.com/peacalm/cpp-lua_wrapper.git
cd cpp-lua_wrapper
mkdir build
cd build
cmake .. 
sudo make install
```

Test is developed using GoogleTest, if GoogleTest is installed, then can run 
test like:
```bash
cd cpp-lua_wrapper/build
cmake .. -DBUILD_TEST=TRUE
make
ctest
```
