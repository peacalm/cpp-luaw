# Lua Wrapper for C++

[![Build](https://github.com/peacalm/cpp-lua_wrapper/actions/workflows/ci.yml/badge.svg)](https://github.com/peacalm/cpp-lua_wrapper/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

This library is a simple wrapper of Lua which makes interaction between Lua and 
C++ easier.

* Lua version >= 5.4
* C++ version >= C++14

Features:
* Get values from Lua to C++
* Set values from C++ to Lua
* Evaluate string Lua expression in C++ to get values
* If a variable provider is provided, it can automatically seek variabls from 
provider while evaluate expressions.

## Value Conversions

**Notice**: Value conversions from Lua to C++ may be different with that in Lua!

Conversion process: Firstly convert value in Lua to C++'s value with
corresponding type, e.g. boolean to bool, integer to long long, number to
double, string to string, then cast it to target type by C++'s type
conversion strategy. Note that number in Lua is also string, and
number-literal-string is also number.

This lib mainly uses C++'s value conversion strategy, in addition, a implicitly
conversion strategy between number and number-literal-string, which is
supported by Lua.

In total, this lib makes value conversions behave more like C++.

Details:
1. Implicitly conversion between integer, number, boolean using
   C++'s static_cast
2. Implicitly conversion between number and number-literal-string by Lua
3. When convert number 0 to boolean, will get false (not true as Lua does)
4. NONE and NIL won't convert to any value, default value (user given or
   initial value of target type) will be returned
5. Non-number-literal-string, including empty string, can't convert to any
   other types, default value will be returned (can't convert to true as 
   Lua does)
6. Integer's precision won't lost if it's value is representable by 64bit
   signed integer type, i.e. between [-2^63, 2^63 -1], which is
   [-9223372036854775808, 9223372036854775807]

Examples:
* number 2.5 -> string "2.5" (By Lua)
* number 3 -> string "3.0" (By Lua)
* ingeger 3 -> string "3" (By Lua)
* string "2.5" -> double 2.5 (By Lua)
* number 2.5 -> int 2 (By C++)
* string "2.5" -> int 2 (Firstly "2.5"->2.5 by lua, then 2.5->2 by C++)
* boolean true -> int 1 (By C++)
* boolean false -> int 0 (By C++)
* integer 0 -> bool false (By C++)
* number 2.5 -> bool true (By C++)
* string "2.5" -> bool true ("2.5"->2.5 by Lua, then 2.5->true by C++)
* string "0" -> bool false ("0"->0 by Lua, then 0->false by C++)

## API Introduction

### 1. Get Global Variables From Lua

In the following API, `@NAME_TYPE@` could be `const char*` or `const std::string&`,
They are overloaded.

#### 1.1 Get Global Variables with Simple Type

* @param [in] name The variable's name.
* @param [in] def The default value returned if failed or target does not exist.
* @param [in] disable_log Whether print a log when exception occurs.
* @param [out] failed Will be set whether the operation is failed if this
pointer is not nullptr.
* @param [out] exists Set whether the variable exists. Regard none and nil as 
not exists.
* @return Return the variable's value if the variable exists and conversion 
succeeded.

```C++
bool               get_bool  (@NAME_TYPE@ name, const bool&               def = false, bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
int                get_int   (@NAME_TYPE@ name, const int&                def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
unsigned int       get_uint  (@NAME_TYPE@ name, const unsigned int&       def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
long               get_long  (@NAME_TYPE@ name, const long&               def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
unsigned long      get_ulong (@NAME_TYPE@ name, const unsigned long&      def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
long long          get_llong (@NAME_TYPE@ name, const long long&          def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
unsigned long long get_ullong(@NAME_TYPE@ name, const unsigned long long& def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
double             get_double(@NAME_TYPE@ name, const double&             def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
std::string        get_string(@NAME_TYPE@ name, const std::string&        def = "",    bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);

// Caller is responsible for popping the stack after calling this API. You'd better use get_string unless you know the difference.
const char*        get_c_str (@NAME_TYPE@ name, const char*                def = "",    bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
```

Example:
```C++
peacalm::lua_wrapper l;
l.dostring("a = 1 b = true c = 2.5 d = 'good'");
int a = l.get_int("a");            // 1
bool b = l.get_bool("b");          // true
bool b2 = l.get_bool("b2", false); // false
bool b3 = l.get_bool("b2", true);  // true
double c = l.get_double("c");      // 2.5
std::string d = l.get_string("d"); // "good"
bool dfailed, dexists;
long dlong = l.get_long("d", -1, false, &dfailed, &dexists); // dlong == -1, dfailed == true, dexists == true
```

#### 1.2 Get Global Variables with Complex Type

This version of API support get container types from Lua, 
and it doesn't support parameter default value.
When getting a container type and the variable exists, the result will contain 
elements who are successfully converted, and discard who are not or who are nil.
Regard the operation failed if any element failed.

* @tparam T The result type user expected. T can be any type composited by 
bool, integer types, double, std::string, std::vector, std::set, 
std::unordered_set, std::map and std::unordered_map. 
Note that here const char* is not supported, which is unsafe.
* @param [out] failed Will be set whether the operation is failed if this
pointer is not nullptr. If T is a container type, it regards the operation
as failed if any element converts failed.
* @return Return the value with given name in type T if conversion succeeded,
otherwise, if T is a simple type (e.g. bool, int, double, std::string, etc), 
return initial value of T(i.e. by statement `T{}`), if T is a container
type, the result will contain all non-nil elements whose conversion
succeeded and discard elements who are nil or elements whose conversion
failed.
```C++
template <typename T> T get(@NAME_TYPE@ name, bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
```

Example:
```C++
peacalm::lua_wrapper l;
l.dostring("a = 1 b = true v={1,2}, v2={1,2.5}, v3={1,nil,3}, m={p1={1,2},p2={3,4}}");
auto a = l.get<int>("a");          // 1
auto as = l.get<std::string>("a"); // "1"
auto b = l.get<bool>("b");         // true
auto bs = l.get<std::string>("b"); // "" (and this conversion fails)

auto v = l.get<std::vector<int>>("v");          // [1,2]
auto vs = l.get<std::vector<std::string>>("v"); // ["1","2"]
auto v2 = l.get<std::vector<int>>("v2");        // [1,2] (2.5->2)
auto v3 = l.get<std::vector<int>>("v3");        // [1,3] (ignore nil)

auto m = l.get<std::map<std::string, std::vector<int>>>("m"); // {"p1":[1,2],"p2":[3,4]}
```

### 2. Recursively Get Fields of Global Variables From Lua

This version of API can get a table's field recursively by a given path.

In the following API, `@PATH_TYPE@` could be any of: 
`const std::initializer_list<const char*>&`, 
`const std::initializer_list<std::string>&`, 
`const std::vector<const char*>&`,
`const std::vector<std::string>&`.

* @param [in] path The first key in path should be a Lua global variable,
the last key in path should be a value which can convert to expected type,
internal keys in path should be Lua table.

#### 2.1 Get Fields with Simple Type
```C++
bool               get_bool  (@PATH_TYPE@ path, const bool&               def = false, bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
int                get_int   (@PATH_TYPE@ path, const int&                def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
unsigned int       get_uint  (@PATH_TYPE@ path, const unsigned int&       def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
long               get_long  (@PATH_TYPE@ path, const long&               def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
unsigned long      get_ulong (@PATH_TYPE@ path, const unsigned long&      def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
long long          get_llong (@PATH_TYPE@ path, const long long&          def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
unsigned long long get_ullong(@PATH_TYPE@ path, const unsigned long long& def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
double             get_double(@PATH_TYPE@ path, const double&             def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
std::string        get_string(@PATH_TYPE@ path, const std::string&        def = "",    bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
```

#### 2.2 Get Fields with Complex Type
```C++
template <typename T> T get(@PATH_TYPE@ path, bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
```

Example:
```C++
peacalm::lua_wrapper l;
l.dostring("a = 1 p={x=10,y=20} m={p1={1,2},p2={3,4}}");
l.get_int({"a"});      // 1
l.get_int({"ax"}, -1); // -1
l.get<int>({"a"});     // 1
l.get<int>({"ax"});    // 0

l.get<int>({"p", "x"});    // 10
l.get_int({"p", "z"}, 30); // 30
l.get<int>({"p", "z"});    // 0

l.get<std::vector<int>>({"m", "p2"}); // [3,4]
```

### 3. Set Global Variables to Lua

In the following API, `@NAME_TYPE@` could be `const char*` or `const std::string&`.

```C++
void set_integer(@NAME_TYPE@ name, long long value);
void set_number(@NAME_TYPE@ name, double value);
void set_boolean(@NAME_TYPE@ name, bool value);
void set_nil(@NAME_TYPE@ name);
void set_string(@NAME_TYPE@ name, const char* value);
void set_string(@NAME_TYPE@ name, const std::string& value);
```

### 4. Execute Lua Scripts (File or String)
Just a simple wrapper of raw Lua API, nothing more added.
```C++
int loadstring(const char*        s);
int loadstring(const std::string& s)l
int dostring(const char*        s);
int dostring(const std::string& s);
int loadfile(const char*        fname);
int loadfile(const std::string& fname);
int dofile(const char*        fname);
int dofile(const std::string& fname);
```

### 5. Evaluator a Lua Expression and Get the Result

In the following API, `@EXPR_TYPE@` could be `const char*` or `const std::string&`.

For simple type, which have default value parameter:
```C++
bool               eval_bool  (@EXPR_TYPE@ expr, const bool&               def = false, bool disable_log = false, bool* failed = nullptr);
int                eval_int   (@EXPR_TYPE@ expr, const int&                def = 0,     bool disable_log = false, bool* failed = nullptr);
unsigned int       eval_uint  (@EXPR_TYPE@ expr, const unsigned int&       def = 0,     bool disable_log = false, bool* failed = nullptr);
long               eval_long  (@EXPR_TYPE@ expr, const long&               def = 0,     bool disable_log = false, bool* failed = nullptr);
unsigned long      eval_ulong (@EXPR_TYPE@ expr, const unsigned long&      def = 0,     bool disable_log = false, bool* failed = nullptr);
long long          eval_llong (@EXPR_TYPE@ expr, const long long&          def = 0,     bool disable_log = false, bool* failed = nullptr);
unsigned long long eval_ullong(@EXPR_TYPE@ expr, const unsigned long long& def = 0,     bool disable_log = false, bool* failed = nullptr);
double             eval_double(@EXPR_TYPE@ expr, const double&             def = 0,     bool disable_log = false, bool* failed = nullptr);
std::string        eval_string(@EXPR_TYPE@ expr, const std::string&        def = "",    bool disable_log = false, bool* failed = nullptr);
```

For complex type, which do not have default value parameter:
```C++
template <typename T> T eval(@EXPR_TYPE@ expr, bool disable_log = false, bool* failed = nullptr);
```

Example:
```C++
peacalm::lua_wrapper l;
l.set_integer("a", 10);
l.set_integer("b", 5);
l.set_integer("c", 2);
double ret = l.eval_double("return a^2 + b/c"); // 102.5
std::string s = l.eval_string("if a > b + c then return 'good' else return 'bad' end"); // "good"
auto si = l.eval<std::set<int>>("return {a, b, c}"); // {2,5,10}
```


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
