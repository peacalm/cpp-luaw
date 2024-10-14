# Luaw : Lua Wrapper for C++

[![Build](https://github.com/peacalm/cpp-luaw/actions/workflows/ci.yml/badge.svg)](https://github.com/peacalm/cpp-luaw/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

This library is a simple wrapper of Lua which makes interaction between Lua and 
C++ easier.

It can be used as a binder, a config file parser or a dynamic expression evaluator.

Features:
* Get Lua values in C++
* Set C++ values to Lua
* Get and call Lua functions in C++
* Bind C++ functions to Lua (including C function, lambda, std::function and user defined callable objects)
* Bind C++ classes to Lua (can bind constructors, member variables and member functions, etc)
* Bind C++ copyable objects, movable objects, smart pointer of objects or raw pointer of objects to Lua
* Evaluate Lua expressions and get results in C++
* If a variable provider is provided, it can automatically seek variables from 
provider while evaluating expressions in C++.

This lib depends only on Lua:
* Lua version >= 5.4
* C++ version >= C++14

## Detailed Features

<table style="width: 200%; table-layout: fixed;">
<colgroup>
    <col span="1" style="width: 45%;">
    <col span="1" style="width: 10%;">
</colgroup>

<tr>
  <th> Feature ++++++++++++++++++++++++++++++++++++++++ </th>
  <th> Supported? </th>
  <th> Example </th>
</tr>


<tr>
  <td> <ul><li> Get Lua values in C++ </li></ul> </td>
  <td> ✅ </td>
  <td>

```C++
// Define a luaw instance, which is used by the following examples
peacalm::luaw l;
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Get simple type values (bool/integer/float number/string) </li></ul></ul> </td>
  <td> ✅ get_xxx </td>
  <td>

```C++
int retcode = l.dostring("i = 1; b = true; f = 2.5; s = 'luastring';");
if (retcode != LUA_OK) {
  // error handler...
}
int i = l.get_int("i");
long long ll = l.get_llong("i");
bool b = l.get_bool("b");
double f = l.get_double("f");
std::string s = l.get_string("s");

// Or use alternative writings:
int i = l.get<int>("i");
long long ll = l.get<long long>("i");
bool b = l.get<bool>("b");
double f = l.get<double>("f");
std::string s = l.get<std::string>("s");
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Get complex container type values 
  (std::pair, std::tuple, std::vector, std::map, std::unordered_map, std::set, std::unordered_set, std::deque, std::list, std::forward_list) </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
int retcode = l.dostring("a = {1,2,3}; b = {x=1,y=2}; c = {x={1,2},y={3,4}}; d = {true, 1, 'str'}");
if (retcode != LUA_OK) {
  // error handler...
}
auto a = l.get<std::vector<int>>("a");
auto b = l.get<std::map<std::string, int>>("b");
auto c = l.get<std::unordered_map<std::string, std::vector<int>>>("c");
auto d = l.get<std::tuple<bool, int, std::string>>("d");

// About set: only collect keys of a table in Lua into a C++ container set.
l.dostring("ss={a=true,b=true,c=true}; si={}; si[1]=true si[2]=true;");
auto ss = l.get<std::set<std::string>>("ss");   // ss == std::set<std::string>{"a", "b", "c"}
auto si = l.get<std::unordered_set<int>>("si"); // si == std::unordered_set<int>{1, 2}
assert(ss == (std::set<std::string>{"a", "b", "c"}));
assert(si == (std::unordered_set<int>{1, 2}));
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Tell whether the target exists or whether the operation failed 
  (we don't regard target's non-existence as fail) </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
bool failed, exists;
auto i = l.get<int>("i", /* disable log */ false, &failed, &exists);
if (failed) {
  // failure handers
}
if (!exists) {
  // ...
}
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Support a default value on target does not exist or operation fails </li></ul></ul> </td>
  <td> ✅ only for simple types (get_xxx) </td>
  <td>

```C++
// API supports default value is defined as get_xxx:
auto i = l.get_int("i", -1); // default value of i is -1
auto s = l.get_string("s", "def"); // default value of s is 'def'

// Donot support defult value for containers:
// auto a = l.get<std::vector<int>>("a", /* disable log */ false, &failed, &exists);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Recursively get element in table by path </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
peacalm::luaw l;
int retcode = l.dostring("a=true; m={p1={x=1,y=2},p2={x=3.2,y=4.5}}");
if (retcode != LUA_OK) {
  // error handlers...
}

bool a = l.get_bool({"a"});  // same to l.get_bool("a"), a == true
int p1x = l.get_int({"m", "p1", "x"}, -1); // p1x == 1
double p2y = l.get<double>({"m", "p2", "y"}); // p2y == 4.5
// p2 = std::map<std::string, double>{{"x", 3.2}, {"y", 4.5}}
auto p2 = l.get<std::map<std::string, double>>({"m", "p2"}); 
```
  </td>
</tr>

<tr>
  <td> <ul><li> Set C++ values to Lua</li></ul> </td>
  <td> ✅ </td>
  <td>
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Set simple type values (bool/integer/float number/string) </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
l.set("b", true);
l.set("i", 123);
l.set("f", 123.45);
l.set("s", "cstr");
l.set("s2", std::string("std::string"));
// Alternative writings may use: 
// set_boolean/set_integer/set_number/set_string
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Set complex container type values 
  (std::pair, std::tuple, std::vector, std::map, std::unordered_map, std::set, std::unordered_set, std::deque, std::list, std::forward_list) </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
// All containers map to Lua table
l.set("a", std::vector<int>{1,2,3});
l.set("b", std::map<std::string, int>{{"a",1},{"b",2}});
l.set("c", std::make_pair("s", true)); // c[1] == "s", c[2] == true

// Map set to table whose keys are from C++ set and values are boolean true.
l.set("si", std::set<int>{1,2,3});
l.set("ss", std::unordered_set<std::string>{"a", "b", "c"});
assert(l.dostring("assert(si[1] == true); assert(ss.a == true)") == LUA_OK);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Set nullptr to Lua (Equivalent to set nil to a variable in Lua) </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
// Equivalent to run "x = nil" in Lua
l.set("x", nullptr);
// Alternative writing:
l.set_nil("x");
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Set pointers to Lua (Array will be set as pointer too) </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
// All pointers not "const char*" will be set as lightuserdata.
// Only "const char*" will be set as string.

// ---- Set as lightuserdata
int a[3] = {1,2,3};
int *p = a;
auto t = a;  // t: type int *
assert((std::is_same<decltype(t), int*>::value) && t == p);
const int *cp = &a[0];
void *vp = (void*)(p);
l.set("a", a);  // setting C array is equivalent to set pointer
l.set("a0", &a[0]);
l.set("p", p);
l.set("cp", cp); // set pointers to lightuserdata by value(address), no matter whether it is const
l.set("vp", vp);
// All equal
assert(l.dostring("assert(a == a0 and a == p and a == cp and a == vp)") == LUA_OK);

// ---- Set as string
const char sa[6] = "hello";
const char* sp = sa;
l.set("sa", sa); // convert to pointer type const char*, so it is a string
l.set("sp", sp);
l.set("ss", "hello");
// All equal
assert(l.dostring("assert(sa == 'hello' and sa == sp and sa == ss)") == LUA_OK);
```
  </td>
</tr>


<tr>
  <td> <ul><ul><li> Recursively set element by path </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
// All name in path except the last one will be touched as a new table if it doesn't exist.
l.set({"a", "b", "c"}, 1);
assert(l.dostring("assert(a.b.c == 1)") == LUA_OK);
l.set({"a", "b", "d", "e"}, 2);
assert(l.dostring("assert(a.b.c == 1 and a.b.d.e == 2)") == LUA_OK);
```
  </td>
</tr>


<tr>
  <td> <ul><li> Get and call Lua functions in C++ </li></ul> </td>
  <td> ✅ </td>
  <td>

```Lua
-- Functions defined in Lua
fadd = function(a, b) return a + b end
fdiv = function(a, b) return a // b, a % b end
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Call a Lua function directly </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
// Should at least provide result type
int s = l.callf<int>("fadd", 1, 2); // s = 1 + 2
assert(s == 3);
// Or provide result type and some argument types
auto d = l.callf<double, double, double>("fadd", 1.25, 2.5);
assert(d == 3.75);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Get a std::function object to represent the Lua function </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
auto f = l.get<std::function<int(int, int)>>("fadd");
int s = f(1,2);
```
  </td>
</tr>

<tr>
  <td>
    <ul><ul>
      <li> Get a luaw::function object to represent the Lua function, which can: </li>
      <ul>
        <li> Tell whether call the Lua function and get result successfully </li>
        <li> Tell whether the function failed while running in Lua </li>
        <li> Tell whether the Lua function exists </li>
        <li> Tell whether converting the Lua function's result to C++ failed </li>
        <li> Tell whether the Lua function returns result (whether result exists) </li>
        <li> Tell whether got enough results (Lua could return more, couldn't less) </li>
        <li> Tell how many results the Lua function returned </li>
      </ul>
    </ul></ul> 
  </td>

  <td> ✅ use luaw::function </td>
  <td>

```C++
auto f = l.get<peacalm::luaw::function<int(int, int)>>("fadd");
int s = f(1,2);
// After call, check:
if (f.failed()) {
  if (f.function_failed()) {
    // ...
  } else if (!f.function_exists()) {
    // ...
  } else if (f.result_failed()) {
    // ...
  } else if (!f.result_enough()) {
    std::cout << "Result number of f " << f.real_result_size()
              << " less than " << f.expected_result_size() << std::endl;
  } else {
    // May never happen
  }
}
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Get/Call a Lua function with multiple results </li></ul></ul> </td>
  <td> ✅ use std::tuple </td>
  <td>

```C++
// Call it directly
auto q = l.callf<std::tuple<int, int>>("fdiv", 7, 3); // q == make_tuple(2, 1)
assert(std::get<0>(q) == 2 && std::get<1>(q) == 1);

// Get a function object first
auto f = l.get<peacalm::luaw::function<std::tuple<int,int>(int, int)>>("fdiv");
auto q2 = f(7, 3);
assert(q2 == std::make_tuple(2, 1));
```
  </td>
</tr>


<tr>
  <td> <ul><li> Bind C++ functions to Lua (also lambda, std::function or callable objects) </li></ul> </td>
  <td> ✅ </td>
  <td>
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Bind C style functions to Lua</li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
// The C function following examples may use
int fadd(int x, int y) { return a + b; }
```
  </td>
</tr>

<tr>
  <td> <ul><ul><ul><li> Set C function directly </li></ul></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
l.set("fadd", fadd);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><ul><li> Set C function pointer </li></ul></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
l.set("fadd", &fadd);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><ul><li> Set C function reference </li></ul></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
auto& ref = fadd;
l.set("fadd", ref);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><ul><li> Set C function with arbitrary number of arguments </li></ul></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
int add_many(int a, int b, int c, int d, /* define any number of arguments */) {
  // ...
}
int main() {
  peacalm::luaw l;
  l.set("add_many", add_many);
}
```
  </td>
</tr>

<tr>
  <td> <ul><ul><ul><li> Set C style variadic function </li></ul></ul></ul> </td>
  <td> ❌ </td>
  <td>

```C++
// A function like printf
int vf(const char* s, ...) { /* some codes */ }
// l.set("vf", vf); // error! not supported
```
  </td>
</tr>


<tr>
  <td> <ul><ul><li> Bind C++ overloaded functions to Lua </li></ul></ul> </td>
  <td> ✅ should provide hint type</td>
  <td>

```C++
// Overloaded functions f
int f(int i) { return i * 2; }
double f(double d) { return d / 2; }
double f(double a, double b) { return a * b; }
int main() {
  peacalm::luaw l;
  // Provide the function proto type as hint
  l.set<int(int)>("f1", f);
  l.set<double(double)>("f2", f);
  l.set<double(double, double)>("f3", f);
  // Or could use function pointer proto type as hint
  l.set<int(*)(int)>("f1", f);
  l.set<double(*)(double)>("f2", f);
  l.set<double(*)(double, double)>("f3", f);
}
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Bind C++ template functions to Lua </li></ul></ul> </td>
  <td> ✅ should specialize or provide hint type</td>
  <td>

```C++
template <typename T>
T tadd(T a, T b) { return a + b; }
int main() {
  peacalm::luaw l;
  // Explicitly specialize the function
  l.set("tadd", tadd<int>);
  // Or provide the function proto type as hint
  l.set<double(double, double)>("tadd", tadd);
}
```
  </td>
</tr>

<tr>
  <td> <ul><ul><ul><li> Bind C++ variadic template functions to Lua </li></ul></ul></ul> </td>
  <td> ✅ should specialize or provide hint type</td>
  <td>

```C++
template <typename T>
T add_many(const T& t) { return t; }
template <typename T, typename... Args>
T add_many(const T& t, const Args&...args) { return t + add_many(args...); }
//...
l.set("add_many", add_many<int, int, int /* as many as you want */>);
// or
l.set<int(const int&, const int& /* as many as you want */)>("add_many", add_many);

```
---
```C++
// after C++17
template <typename... Args>
auto add_many_cpp17(const Args&... args) { return (... + args); }
// ...
l.set("add_many_cpp17", add_many_cpp17<int, int, int /* as many as you want */>);
// or
l.set<int(const int&, const int& /* as many as you want */ )>("add_many_cpp17", add_many_cpp17);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Bind C++ lambdas to Lua </li></ul></ul> </td>
  <td> ✅ </td>
  <td>
  </td>
</tr>

<tr>
  <td> <ul><ul><ul><li> Set captureless lambdas to Lua </li></ul></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
// May not provide hint type
l.set("add", [](int a, int b) { return a + b; });
// Or alternative writings:
l.set<int(int, int)>("add", [](int a, int b) { return a + b; });
l.set<peacalm::luaw::function_tag>("add", [](int a, int b) { return a + b; });
```
  </td>
</tr>

<tr>
  <td> <ul><ul><ul><li> Set not-captureless lambdas to Lua </li></ul></ul></ul> </td>
  <td> ✅ should provide hint type </td>
  <td>

```C++
int x = 1;
auto f = [&](int a) { return a + x; };

// Provide a function type hint: int(int)
l.set<int(int)>("add", f);

// Alternative writing: could use luaw::function_tag as hint type
l.set<peacalm::luaw::function_tag>("add", f);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><ul><li> Set generic lambdas to Lua </li></ul></ul></ul> </td>
  <td> ✅ should provide hint type </td>
  <td>

```C++
// Generic lambda
auto f = [](auto a, auto b) { return a + b; }

// Should provide a function proto type as hint
l.set<int(int, int)>("add", f);

// The lambda object can be reused with other function proto types
l.set<double(double, double)>("add2", f);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Set std::function to Lua </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
std::function<int(int, int)> f = [](auto a, auto b) { return a + b; };

// Set f by copy:
l.set("f", f);

// Or set f by move:
l.set("f", std::move(f));

// Or provide hint type explicitly, these are equivalent:
l.set<int(int, int)>("f", f);
l.set<int(*)(int, int)>("f", f);
l.set<peacalm::luaw::function_tag>("f", f);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Set user defined callable objects as function to Lua </li></ul></ul> </td>
  <td> ✅ should provide hint type </td>
  <td>

```C++
struct Add {
    int operator()(int a, int b) const { return a + b; }
};
int main() {
  peacalm::luaw l;

  // Must provide hint type to clearly point out that set the object as a function.
  // These are equivalent:
  l.set<int(int, int)>("add", Add{});
  l.set<int(*)(int, int)>("add", Add{});
  l.set<peacalm::luaw::function_tag>("add", Add{});
}
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Support C++ functions with multiple returns </li></ul></ul> </td>
  <td> ✅ use std::tuple </td>
  <td>

```C++
std::tuple<int, int> fdiv(int a, int b) { return std::make_tuple(a / b, a % b); }

l.set("fdiv", fdiv);
l.dostring("q, r = fdiv(7, 3)"); // q == 2, r == 1
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Support C++ functions whose return is reference </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
int g = 1;
int& getg() { return g; }

int main() {
  peacalm::luaw l;
  l.set("getg", &getg);
  assert(l.eval_int("return getg()") == 1);
  // Can set the variable referenced by function's return in C++.
  getg() = 2;
  assert(l.eval_int("return getg()") == 2);
  assert(g == 2);

  // But can't set the variable referenced by function's return in Lua.
  retcode = l.dostring("getg() = 3");
  assert(retcode != LUA_OK);
  l.log_error_out(); // error log: syntax error near '='
  assert(g == 2);
}
```
  </td>
</tr>


<tr>
  <td> <ul><ul><li> Support default arguments provided in C++ when binding C++ functions 
  (Bind C++ default arguments to Lua when binding C++ functions) </li></ul></ul> </td>
  <td> ❌ argumengts(not given real parameter in Lua) will be default-initialized 
  no matter what default values provided in C++ </td>
  <td>

```C++
// The default argument values 3 and 4 have no effect in Lua
std::tuple<int,int> point(int x = 3, int y = 4) {
  return std::make_tuple(x, y);
}
int main() {
  peacalm::luaw l;
  l.set("point", point);
  l.dostring("x, y = point(1)");
  assert(l.get_int("x") == 1);  // ok, 1 is explicitly provided
  assert(l.get_int("y") == 0);  // y will be default-initialized to 0, not 4
}
```
  </td>
</tr>

<tr>
  <td> <ul><li> Bind C++ classes to Lua </li></ul> </td>
  <td> ✅ </td>
  <td>

```C++
// The C++ custom class to register
struct Obj {
  int        i  = 1;
  const int  ci = 1;
  static int si;

  Obj() {}
  Obj(int v) : i(v) {}
  Obj(int v, int cv) : i(v), ci(cv) {}
  
  int abs() const { return std::abs(i); }

  int plus() { return ++i; }
  int plus(int d) { i += d; return i; }

  static int sqr(int x) { return x * x; }
};
int Obj::si = 1;
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Register constructor </li></ul></ul> </td>
  <td> ✅ should provide hint type </td>
  <td>

```C++
l.register_ctor<Obj()>("NewObj");          // default constructor
l.register_ctor<Obj(int)>("NewObj1");      // constructor with 1 argument
l.register_ctor<Obj(int, int)>("NewObj2"); // constructor with 2 argument2

// Then can use ctor as a global function in Lua
l.dostring("a = NewObj(); b = NewObj1(1); c = NewObj2(1,2)");
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Register constructor for const object</li></ul></ul> </td>
  <td> ✅ should provide hint type </td>
  <td>

```C++
// The constructors will generate a const instance of Obj in Lua
using ConstObj = const Obj;  // or use std::add_const_t<Obj>
l.register_ctor<ConstObj()>("NewConstObj");          // default constructor
l.register_ctor<ConstObj(int)>("NewConstObj1");      // constructor with 1 argument
l.register_ctor<ConstObj(int, int)>("NewConstObj2"); // constructor with 2 argument2

// Then can use ctor as a global function in Lua
l.dostring("a = NewConstObj(); b = NewConstObj1(1); c = NewConstObj2(1,2)");
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Register member variable </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
l.register_member("i", &Obj::i);
l.register_member("ci", &Obj::ci);  // register const member
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Register member function </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
l.register_member("abs", &Obj::abs);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Register overloaded member function </li></ul></ul> </td>
  <td> ✅ should provide hint type </td>
  <td>

```C++
l.register_member<int (Obj::*)()>("plus", &Obj::plus);
l.register_member<int (Obj::*)(int)>("plusby", &Obj::plus);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Register static member variable </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
l.register_static_member<Obj>("si", &Obj::si);
// or
l.register_member<int Obj::*>("si", &Obj::si);

// register non-const static member "si" as a const member in Lua
l.register_member<const int Obj::*>("si", &Obj::si);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Register static member function </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
l.register_static_member<Obj>("sqr", &Obj::sqr);
// or
l.register_static_member<Obj>("sqr", Obj::sqr);
// or
l.register_member<int (Obj::*)(int) const>("sqr", &Obj::sqr);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Register dynamic members (members whose name are dynamically defined in Lua, like keys of a table) </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
// If types of dynamic members are unknown, use luaw::luavalueref
struct Foo {
  std::unordered_map<std::string, peacalm::luaw::luavalueref> m;
};
peacalm::luaw::luavalueref foo_dm_getter(const Foo* o, const std::string& k) {
  auto entry = o->m.find(k);
  if (entry != o->m.end()) { return entry->second; }
  return peacalm::luaw::luavalueref(); // default value is nil
}
void foo_dm_setter(Foo* o, const std::string& k, const peacalm::luaw::luavalueref& v) {
  o->m[k] = v;
}
int main() {
  peacalm::luaw l;
  l.register_dynamic_member(foo_dm_getter, foo_dm_setter);
  l.set("f", Foo{});
  int retcode = l.dostring("f.a = 1; f.b = true; f.c = 10.5; f.d = 'str'");
  // ...
}
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Register fake member variables (members who are not really member of the class) </li></ul></ul> </td>
  <td> ✅ should provide a member type as hint </td>
  <td>

```C++
// Fake a const member "id" whose value is the object's address
l.register_member<void* const Obj::*>(
    "id", [](const Obj* p) { return (void*)p; });

auto o = std::make_shared<Obj>();
l.set("o", o);
assert(l.eval<void*>("return o.id") == (void*)(o.get()));
```
---
```C++
int gi = 100;  // global i, member to be faked
int main() {
    peacalm::luaw l;
    // gi will be shared by all instances of Obj in Lua, like a static member
    l.register_member<int Obj::*>("gi", [&](const Obj*) -> int& { return gi; });
    l.set("o", Obj{});
    assert(l.eval<int>("return o.gi") == gi);
    assert(l.eval<int>("o.gi = 101; return o.gi") == 101);
    assert(gi == 101);
}
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Register fake member functions </li></ul></ul> </td>
  <td> ✅ should provide a member type as hint </td>
  <td>

```C++
// Fake a member function "seti" for class Obj
l.register_member<void (Obj ::*)(int)>(
  "seti", [](Obj* p, int v) { p->i = v; });
```
  </td>
</tr>


<tr>
  <td> <ul><ul><li> Register a member as another convertible type in Lua </li></ul></ul> </td>
  <td> ✅  </td>
  <td>

```C++
peacalm::luaw l;
// register int member Obj::i as const integer member
l.register_member<const int Obj::*>("i", &Obj::i);
// register int member Obj::i as boolean member
l.register_member<bool Obj::*>("b", &Obj::i); 

auto o = std::make_shared<Obj>();
l.set("o", o);

// o.i is const integer
assert(l.dostring("o.i = 2") != LUA_OK);
l.log_error_out(); // Const member cannot be modified: i

// o.b is a boolean converted by o.i
assert(l.dostring("assert(o.b == true and o.b ~= 1)") == LUA_OK);
o->i = 2;
assert(l.dostring("assert(o.b == true and o.b ~= 2)") == LUA_OK);

// set boolean value to Obj::i, convert boolean to int by C++ way
assert(l.dostring("o.b = true") == LUA_OK);
assert(o->i == 1);
assert(l.dostring("o.b = false") == LUA_OK);
assert(o->i == 0);

// Convert 2 to true then set to o.b
assert(l.dostring("o.b = 2") == LUA_OK);
assert(l.dostring("assert(o.b == true and o.b ~= 2)") == LUA_OK);
assert(o->i == 1);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Set class instance directly to Lua </li></ul></ul> </td>
  <td> ✅  </td>
  <td>

```C++
Obj o;
l.set("o", o); // by copy
// or
l.set("o", std::move(o)); // by move
// or
l.set("o", Obj{}); // by move
// The variable "o" can access all registered members of Obj

const Obj co;
l.set("co", co); // set const instance by copy
// or
l.set("co", std::move(co)); // set const instance by move
// or
l.set("co", std::add_const_t<Obj>{}); // set const instance by move
// The variable "co" is const, it can access all registered members variables 
// and registered const member functins of Obj.
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Set class instance by smart pointer </li></ul></ul> </td>
  <td> ✅  </td>
  <td>

```C++
// shared_ptr
auto o = std::make_shared<Obj>();
l.set("o", o); // by copy
// or
l.set("o", std::move(o)); // by move

// unique_ptr
l.set("o", std::make_unique<Obj>()); // by move

// The variable "o" can access all registered members of Obj
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Set class instance by raw pointer 
  (set a lightuserdata and set metatable simultaneously for not only it but for all lightuserdata) </li></ul></ul> </td>
  <td> ✅ (❗But NOT recommend) </td>
  <td>

```C++
Obj o;
l.set("o", &o); // set a lightuserdata which has metatable of class Obj
// The variable "o" can access all registered members of Obj
// Now all lightuserdata in Lua share the same metatable as "o"!
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Set class instance by raw pointer of smart pointer 
  (set a lightuserdata and set metatable simultaneously for not only it but for all lightuserdata) </li></ul></ul> </td>
  <td> ✅ (❗But NOT recommend) </td>
  <td>

```C++
auto o = std::make_shared<Obj>();
l.set("o", &o); // set a lightuserdata with metatable of class Obj
// The variable "o" can access all registered members of Obj
// Now all lightuserdata in Lua share the same metatable as "o"!
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Set raw pointer by wrapper
  (The wrapped pointer will be a full userdata, it has an exclusive metatable, and can access members like raw pointer too.) </li></ul></ul> </td>
  <td> ✅  </td>
  <td>

```C++
Obj o;
l.set_ptr_by_wrapper("o", &o); // set a full userdata which has metatable of class Obj
// The variable "o" can access all registered members of Obj

// set a (low-level) const pointer to "o" into Lua
l.set_ptr_by_wrapper("co", (const Obj*)(&o));
// "co" can only read members of "o", can't modify
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Set address of class instance (no setting metatable) </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
Obj o;
// Set address of o, which will be a lightuserdata without metatable if there 
// are no metatable installed for lightuserdata before!
l.set<void*>("o", &o);
// or alternative writing:
l.set("o", (void*)(&o));
// or alternative writing:
l.set<void*>("o", (void*)(&o));

// The variable "o" is a lightuserdata without metatable, so it can not 
// access members of Obj. But metatable can be installed later. 
// Then it can access members...
```
---
```C++
Obj o;
// Really set address of "o" as a number in Lua, not as lightuserdata
l.set("addr", reinterpret_cast<long long>(&o));
assert(l.get_llong("addr") == reinterpret_cast<long long>(&o));
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Set class instance by unique_ptr with custom deleter </li></ul></ul> </td>
  <td> ✅  </td>
  <td>

```C++
// Define deleter
struct ObjDeleter {
  void operator()(Obj* p) const { delete p; }
};

// ...

std::unique_ptr<Obj, ObjDeleter> o(new Obj, ObjDeleter{});
l.set("o", std::move(o));
// The variable "o" can access all registered members of Obj,
// just like that unique_ptr with default deleter.

l.close(); // ObjDeleter will be called when destructing "o"
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Const and volatile properties of objects, member variables and member functions are kept in Lua </li></ul></ul> </td>
  <td> ✅  </td>
  <td>

```C++
l.register_ctor<Obj()>("NewObj");
l.register_ctor<std::add_const_t<Obj>()>("NewConstObj"); // ctor for const object
l.register_member("i", &Obj::i);
l.register_member("ci", &Obj::ci); // const member
l.register_member<int (Obj::*)()>("plus", &Obj::plus); // nonconst member function

// Const property of member ci is kept in Lua
int retcode = l.dostring("o = NewObj(); o.ci = 3");
assert(retcode != LUA_OK);
l.log_error_out(); // error log: Const member cannot be modified: ci

l.set("o", std::add_const_t<Obj>{}); // "o" is const Obj
l.eval<void>("o:plus()"); // call a nonconst member function
// error log: Nonconst member function: plus

l.eval<void>("o = NewConstObj(); o:plus()");
// error log: Nonconst member function: plus

// Member of const object is also const
l.eval<void>("o = NewConstObj(); o.i = 3");
// error log: Const member cannot be modified: i
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Get object instance created in Lua </li></ul></ul> </td>
  <td> ✅  </td>
  <td>

```C++
l.register_member("i", &Obj::i);
l.register_ctor<Obj()>("NewObj");
l.dostring("a = NewObj(); a.i = 3;"); // creat a instance of Obj
Obj a = l.get<Obj>("a");
assert(a.i == 3);
assert(a.ci == 1);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Register nested class members (Register member of member) </li></ul></ul> </td>
  <td> ✅  </td>
  <td>

```C++
struct A { int i = 1; };
struct B { A a; };
int main() {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("a", &B::a);

  auto b = std::make_shared<B>();
  l.set("b", b);

  assert(l.dostring(R"(
    assert(b.a.i == 1)

    -- Wrong way of modifying member of member
    b.a.i = 2
    assert(b.a.i == 1)

    -- Correct way of modifying member of member
    t = b.a  -- Get a copy of b.a
    t.i = 2  -- Modify the copy
    b.a = t  -- Modify the whole member b.a by the copy
    assert(b.a.i == 2)
  )") == LUA_OK);
}
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Register pointer of member variables (Raw pointer, which is a light userdata) </li></ul></ul> </td>
  <td> ✅  </td>
  <td>

```C++
struct A { int i = 1; };
struct B { A a; };
int main() {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  // register pointer of B::a, can access and modify member
  l.register_member_ptr("aptr", &B::a);
  // register (low-level) const pointer of B::a, can only access member
  l.register_member_cptr("acptr", &B::a);

  auto b = std::make_shared<B>();
  l.set("b", b);

  assert(l.dostring(R"(
    assert(b.aptr.i == 1)
    assert(b.acptr.i == 1)

    b.aptr.i = 2  -- Can modify member of a by aptr
    assert(b.aptr.i == 2)
    assert(b.acptr.i == 2)
  )") == LUA_OK);

  // Can't modify member of a by acptr
  assert(l.dostring("b.acptr.i = 3") != LUA_OK);
  l.log_error_out(); // Const member cannot be modified: i
}
```
  </td>
</tr>


<tr>
  <td> <ul><ul><li> Register reference of member variables (which is a full userdata) </li></ul></ul> </td>
  <td> ✅  </td>
  <td>

```C++
struct A { int i = 1; };
struct B { A a; };
int main() {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  // register reference of B::a, can access and modify member
  l.register_member_ref("aref", &B::a);
  // register (low-level) const reference of B::a, can only access member
  l.register_member_cref("acref", &B::a);

  auto b = std::make_shared<B>();
  l.set("b", b);

  assert(l.dostring(R"(
    assert(b.aref.i == 1)
    assert(b.acref.i == 1)

    b.aref.i = 2  -- Can modify member of a by aref
    assert(b.aref.i == 2)
    assert(b.acref.i == 2)
  )") == LUA_OK);

  // Can't modify member of a by acref
  assert(l.dostring("b.acref.i = 3") != LUA_OK);
  l.log_error_out(); // Const member cannot be modified: i
}
```
  </td>
</tr>


<tr>
  <td> <ul><li> Evaluate a Lua script and get results </li></ul> </td>
  <td> ✅ </td>
  <td>

```C++
int ret = l.eval<int>("return 1");
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Return simple type result (bool/integer/float number/string) </li></ul></ul> </td>
  <td> ✅ eval_xxx </td>
  <td>

```C++
bool b = l.eval_bool("return 1 > 0");
int i = l.eval_int("return 1 + 2");
double d = l.eval_double("return 1.2 * 3.4");
std::string s = l.eval_string("return 'hello' .. ' world' ");

// Alternative writing:
bool = l.eval<bool>("return 1 > 0");
int i = l.eval<int>("return 1 + 2");
// ...
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Return complex type result (std::pair, std::tuple, std::vector, std::map, std::unordered_map, std::set, std::unordered_set, std::deque, std::list, std::forward_list) </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
auto v = l.eval<std::vector<int>>("return {1,2,3}");
auto m = l.eval<std::unordered_map<std::string, int>>("return {x=1,y=2}");
```
  </td>
</tr>


<tr>
  <td> <ul><ul><li> Return multiple values </li></ul></ul> </td>
  <td> ✅ use std::tuple </td>
  <td>

```C++
auto ret = l.eval<std::tuple<int, int, int>>("return 1,2,3");
```
  </td>
</tr>

<tr>
  <td> <ul><ul><ul><li> Return 0 value (no return) by std::tuple<> </li></ul></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
// No return: equivalent to l.eval<void>
l.eval<std::tuple<>>("a=1 b=2");
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Return void (no return) </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
// Equivalent to l.dostring, but this will print error log automatically
l.eval<void>("a=1 b=2");
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Tell whether the evaluation fails </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
bool failed;
int ret = l.eval<int>("return a + b",  /* disable log */ false, &failed);
if (failed) {
  // error handler
}
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Support a defult value returned when evaluation fails </li></ul></ul> </td>
  <td> ✅ only supported by eval simple types (eval_xxx) </td>
  <td>

```C++
int ret = l.eval_int("return a + b", /* disable log */ false, /* default value */ -1);
```
  </td>
</tr>

</table>

## Value Conversions

**Notice**: Value conversions from Lua to C++ may be different with that in Lua!

Conversion process: Firstly convert value in Lua to C++'s value with
corresponding type, e.g. boolean to bool, integer to long long, number to
double, string to string, then cast it to target type by C++'s type
conversion strategy. Note that number in Lua is also string, and
number-literal-string is also number.

This lib mainly uses C++'s value conversion strategy, in addition, an implicitly
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
6. Integer's precision won't be lost if its value is representable by 64bit
   signed integer type, i.e. between [-2^63, 2^63 -1], which is
   [-9223372036854775808, 9223372036854775807]

Examples:
* number 2.5 -> string "2.5" (By Lua)
* number 3 -> string "3.0" (By Lua)
* integer 3 -> string "3" (By Lua)
* string "2.5" -> double 2.5 (By Lua)
* number 2.5 -> int 2 (By C++)
* string "2.5" -> int 2 (Firstly "2.5"->2.5 by lua, then 2.5->2 by C++)
* boolean true -> int 1 (By C++)
* boolean false -> int 0 (By C++)
* integer 0 -> bool false (By C++)
* number 2.5 -> bool true (By C++)
* string "2.5" -> bool true ("2.5"->2.5 by Lua, then 2.5->true by C++)
* string "0" -> bool false ("0"->0 by Lua, then 0->false by C++)

## Introduction

### 1. Get Lua values in C++

#### 1.1 Get simple type values, default value supported!

API:

```C++
bool               get_bool   (@PATH_TYPE@ path, const bool&               def = false, bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
int                get_int    (@PATH_TYPE@ path, const int&                def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
unsigned int       get_uint   (@PATH_TYPE@ path, const unsigned int&       def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
long               get_long   (@PATH_TYPE@ path, const long&               def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
unsigned long      get_ulong  (@PATH_TYPE@ path, const unsigned long&      def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
long long          get_llong  (@PATH_TYPE@ path, const long long&          def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
unsigned long long get_ullong (@PATH_TYPE@ path, const unsigned long long& def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
float              get_float  (@PATH_TYPE@ path, const float&              def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
double             get_double (@PATH_TYPE@ path, const double&             def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
long double        get_ldouble(@PATH_TYPE@ path, const long double&        def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
std::string        get_string (@PATH_TYPE@ path, const std::string&        def = "",    bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
```

`@PATH_TYPE@` could be:

1. `const char*` or `const std::string&`.

Then the formal parameter `path` is a global variable's name in Lua.

2. `const std::initializer_list<const char*>&`, `const std::initializer_list<std::string>&`, `const std::vector<const char*>&` or`const std::vector<std::string>&`.

Then the formal parameter `path` is a path to subfield of a table.

* @param [in] path The target variable's name or path.
* @param [in] def The default value returned if failed or target does not exist.
* @param [in] disable_log Whether print a log when exception occurs.
* @param [out] failed Will be set whether the operation is failed if this
pointer is not nullptr.
* @param [out] exists Set whether the variable exists. Regard none and nil as 
not exists.
* @return Return the variable's value if the variable exists and conversion 
succeeded.


Example:

```C++
peacalm::luaw l;
l.dostring("a = 1 b = true c = 2.5 d = 'good'");
int a = l.get_int("a");            // 1
bool b = l.get_bool("b");          // true
bool b2 = l.get_bool("b2", false); // false
bool b3 = l.get_bool("b2", true);  // true
double c = l.get_double("c");      // 2.5
std::string d = l.get_string("d"); // "good"

bool dfailed, dexists;
long dlong = l.get_long("d", -1, false, &dfailed, &dexists);
// dlong == -1, dfailed == true, dexists == true

bool dfailed2, dexists2;
long dlong2 = l.get<long>("d", false, &dfailed2, &dexists2);
// dlong2 == 0, dfailed2 == true, dexists2 == true
```

```C++
peacalm::luaw l;
l.dostring("a = 1 p={x=10,y=20} m={p1={x=1,y=2},p2={x=3.2,y=4.5}}");
l.get_int({"a"});      // 1
l.get_int({"ax"}, -1); // -1 (return user given default value)
l.get_long({"p", "x"});      // 10
l.get_long({"p", "z"}, 30);  // 30 (return user given default value)
l.get_double({"m", "p2", "x"}); // 3.2
```

#### 1.2 Get complex type values, no default value support!

API:

```C++
/// Get values, functions, userdatas, pointers etc.
/// @param path Could be single string or a list of string.
template <typename T>
T get(@PATH_TYPE@ path, bool disable_log = false, 
      bool* failed = nullptr, bool* exists = nullptr);


/// Options used as first parameter for "lget".
struct lgetopt {
  bool  disable_log;
  bool *failed, *exists;
  lgetopt(bool d = false, bool* f = nullptr, bool* e = nullptr)
      : disable_log(d), failed(f), exists(e) {}
};

/// Long get. Args is the path to get value.
template <typename T, typename... Args>
T lget(const lgetopt& o, Args&&... args) 
```

Also, `@PATH_TYPE@` could be single string or string list.

`lget` not only support single string or string list as path, but support list 
of any type if only it can be used as a key of a table, 
such as string, integer, void*, and `luaw::metatable_tag`.

Using this API we can get:

1. Can get a C++ data type.

`T` could be any type composited by bool, integer types, float number types,
C string, std::string, std::pair, std::tuple, std::vector, std::set,
std::unordered_set, std::map, std::unordered_map, std::deque, std::list,
std::forward_list.

Also, `T` could be cv-qualified, but cannot be a reference.

Note that using C string "const char*" as key type of set or map is
forbidden, and anytime using "const char*" as a member of a container is not 
recommended, should better use std::string instead.

2. Can get a user defined class type.

3. Can get a callable function-like type which represents a Lua function.

`T` could be std::function or luaw::function.

4. Can get a pointer type.
`T` could be pointer of data type.

When getting a container type and the variable exists, the result will contain 
elements who are successfully converted, and discard who are not or who are nil.
Regard the operation as failed if any element's conversion failed.

* @tparam T The result type user expected. 
* @param [in] path The target variable's name or path.
* @param [in] disable_log Whether print a log when exception occurs.
* @param [out] failed Will be set whether the operation is failed if this
pointer is not nullptr. If T is a container type, it regards the operation
as failed if any element failed.
* @return Return the value with given name in type T if conversion succeeds,
otherwise, if T is a simple type (e.g. bool, int, double, std::string, etc), 
return initial value of T(i.e. by statement `T{}`), if T is a container
type, the result will contain all non-nil elements whose conversion
succeeded and discard elements who are nil or elements whose conversion
failed.


Example:

```C++
peacalm::luaw l;
l.dostring("a = 1 b = true v={1,2} v2={1,2.5} v3={1,nil,3} m={p1={1,2},p2={3,4}}");
auto a = l.get<int>("a");          // 1
auto as = l.get<std::string>("a"); // "1"
auto b = l.get<bool>("b");         // true
auto bs = l.get<std::string>("b"); // "" (this conversion fails)

auto v = l.get<std::vector<int>>("v");          // [1,2]
auto vs = l.get<std::vector<std::string>>("v"); // ["1","2"]
auto v2 = l.get<std::vector<int>>("v2");        // [1,2] (2.5->2)
auto v3 = l.get<std::vector<int>>("v3");        // [1,3] (ignore nil)

auto m = l.get<std::map<std::string, std::vector<int>>>("m"); // {"p1":[1,2],"p2":[3,4]}

auto p1 = l.get<std::pair<int, int>>({"m", "p1"}); // (1,2)

// example for lget:
l.dostring("g={gg={{a=1,b=2},{a=10,b=20,c='s'}}}");
int b = l.lget<int>({}, "g", "gg", 2, "b"); // 20
```

Another example, could use lget to get a userdata's metatable name:
```C++
struct Obj {};

int main() {
  peacalm::luaw l;
  l.register_member<void* const Obj::*>(
    "id", [](const Obj* p) { return (void*)p; });

  auto s = std::make_shared<Obj>();
  l.set("s", s);

  void* sid = l.eval<void*>("return s.id");
  void* sid2 = l.get<void*>({"s", "id"});
  assert(sid == sid2);
  assert(sid == (void*)(s.get()));

  void* saddr = l.get<void*>("s"); // the shared ptr s itself's address
  assert(saddr != sid);

  // Use lget to get metatable name
  bool disable_log, failed, exists;
  auto metatbname = l.lget<std::string>({disable_log, &failed, &exists},
    "s", peacalm::luaw::metatable_tag{}, "__name");
  std::cout << "metatable name of s: " << metatbname << std::endl;
}
```

### 2. Set C++ values to Lua
API:

```C++
/// Set value as a global variable or a subfield of a table
template <typename T>
void set(@PATH_TYPE@ path, T&& value);

/// Set with a hint type
template <typename Hint, typename T>
std::enable_if_t<!std::is_same<Hint, T>::value>
void set(@PATH_TYPE@ path, T&& value);


/// Long set. The last argument is value, the rest arguments are indexes and
/// sub-indexes, where could contain luaw::metatable_tag.
template <typename... Args>
void lset(Args&&... args);

/// Long set with a hint type.
template <typename Hint, typename... Args>
void lset(Args&&... args)


// Other very simple API to set global variables to Lua.
// @NAME_TYPE@ could be const char* or const std::string&.
void set_integer(@NAME_TYPE@ name, long long value);
void set_number(@NAME_TYPE@ name, double value);
void set_boolean(@NAME_TYPE@ name, bool value);
void set_nil(@NAME_TYPE@ name);
void set_string(@NAME_TYPE@ name, const char* value);
void set_string(@NAME_TYPE@ name, const std::string& value);
```

Also, `@PATH_TYPE@` could be a single string or a string list.
If it is a single string, this API sets a global variable to Lua;
If it is a string list, this API sets a subfield of a Lua table, and if keys 
in path doesn't exist or it is not a table 
(or not indexable and newindexable userdata), 
it will creat a new table for it.

So the set operation will always succeed.

`lset` not only support string list as path, but support any type if only 
it can be used as a key of a table, such as string, integer, void*, and 
`luaw::metatable_tag`.


The value type can be:
1. C++ data type: simple types or container types
2. C function or lambda or std::function
3. Custom class types
4. smart pointers
5. raw pointers
6. nullptr
7. luaw::newtable_tag

Especially to say:

`nullptr` means nil in Lua. e.g. `set(name, nullptr)` means setting nil to "name".

`set(name, luaw::newtable_tag{})` means setting a new empty table to "name".

Setting a raw pointer means setting a lightuserdata to Lua.

Example:

```C++
l.set("b", true);
l.set("i", 123);
l.set({"g", "f"}, 123.45);  // set g.f = 123.45, make g as a global table
l.set({"g", "s"}, "cstr");
l.set("s2", std::string("std::string"));

l.set("a", std::vector<int>{1,2,3});
l.set("b", std::map<std::string, int>{{"a",1},{"b",2}});
l.set({"g", "c"}, std::make_pair("c", true));

l.set("x", nullptr); // set nil to "x"
l.set("t", peacalm::luaw::newtable_tag); // set "t" as a new empty table

l.lset("g", "b", "v", 1); // set g.b.v = 1, and make g, b as table

// These are equivalent to: l.dostring("nums = { en = {'one', 'two', 'three'}}");
l.lset("nums", "en", 1, "one");
l.lset("nums", "en", 2, "two");
l.dostring("nums.en[3] = 'three'");
```


### 3. Get and call Lua functions in C++

In C++ we use std::tuple to represent multiple returns in Lua.

We recommend getting a `luaw::function` object to represent a Lua function first,
then calling it to implement the call to the Lua function.

#### 3.1 Call a Lua function directly:

API:

```C++
/// Call Lua function specified by path using C++ parameters.
/// Should at least provide return type.
template <typename Return, typename... Args>
Return callf(@PATH_TYPE@ path, const Args&... args);
```

`@PATH_TYPE@` could be ether a single string or a list of string.

Example:

```C++
l.dostring("f1 = function(a, b) return a + b end");
l.dostring("f2 = function(a, b) return a + b, a - b end");
l.dostring("g = {f1=f1, f2=f2}");

assert(l.callf<int>("f1", 1, 1) == 2);
assert(l.callf<int>({"g", "f1"}, 1, 1) == 2);
assert(l.callf<std::tuple<int, int>>({"g", "f2"}, 1, 1) == std::tuple<int, int>(2, 0));
```

In this case we can call a Lua function with C++ arguments directly and conveniently, 
but can't know whether it works correctly. If you want to know, see the following.

#### 3.2 Get a callable object to represent the Lua function

We can get a callable object of type `std::function` or `luaw::function` 
to represent the Lua function, then we can call it to implement the calling to 
the Lua function. But using `luaw::function` is recommended.

Using `std::function` is simple, but it provide nothing information about whether the function works correctly:

```C++
peacalm::luaw l;
int retcode = l.dostring("f = function(a, b) local s = a + b return s end;");
if (retcode != LUA_OK) { /* error handlers*/ }

// Test 1
{
  auto f = l.get<std::function<int(int, int)>>("f");
  int s = f(1, 1);
  assert(s == 2);  // works correctly
}

// Test 2
{
  // Typo of function name, getting an inexistent function
  auto f = l.get<std::function<int(int, int)>>("fnx");
  int s = f(1, 1);
  // Log info: Lua: calling an inexistent function
  assert(s == 0);  // works wrong
}

// Test 3
// The function f returns nothing, maybe forget
retcode = l.dostring("f = function(a, b) local s = a + b end;");
if (retcode != LUA_OK) { /* error handlers*/ }
{
  auto f = l.get<std::function<int(int, int)>>("f");
  int s = f(1, 1);
  assert(s == 0);  // works wrong
  // It returns a wrong result quietly, this is dangerous!
}
```

So, using `std::function` is not recommended.

Instance of `luaw::function` can be called like `std::function` 
(actually the latter is a simple wrapper of the former), 
but it provide more information after it was called, 
and these information are very userful to let us make sure whether the function works correctly as we expect, 
and we can write error handlers if there are exceptions, 
and it can help us to debug in which step the exceptions happen. 


These informations are:

A total state about the whole process:

- whether call the Lua function and get result successfully

Detailed states for each step:

- whether the function failed while running in Lua
- whether the Lua function exists
- whether converting the Lua function's result to C++ failed
- whether the Lua function returns result (whether result exists)
- whether got enough results (Lua could return more, couldn't less)
- how many results the Lua function returned
- how many results we expect the Lua function should at least return

Part of `luaw::function`'s implementation looks like:
```C++
template <typename>
class luaw::function;

template <typename Return, typename... Args>
class luaw::function<Return(Args...)> {
public:
  /// Set log on-off.
  void disable_log(bool v);

  /// Return a C string text message to indicate current state.
  const char* state_msg() const;

  /// Whether the whole process failed.
  /// Any step fails the whole process fails, including running the function in
  /// Lua and converting results to C++, and whether get enough results.
  bool failed() const;

  /// Whether the function failed while running in Lua (not including function
  /// doesn't exist)
  bool function_failed() const;

  /// Whether the function exists in Lua
  bool function_exists() const;

  /// Whether converting the Lua function's results to C++ failed
  bool result_failed() const;

  /// Whether the Lua function returns results (whether results exist)
  bool result_exists() const;

  /// Whether Lua function returns enough results.
  /// Could return more than expect, but couldn't less.
  bool result_enough() const;

  /// Result number the Lua function returned
  int real_result_size() const;

  /// Result number we expect the Lua function should return.
  /// Could return more, couldn't less.
  constexpr int expected_result_size() const;

  // ...
};
```

Example:

```C++
peacalm::luaw l;
int retcode = l.dostring("f = function(a, b) local s = a + b end;");
if (retcode != LUA_OK) { /* error handlers*/ }

auto f = l.get<peacalm::luaw::function<int(int, int)>>("f");
int s = f(1, 1);
if (f.failed()) {
  if (f.function_failed()) {
    // ...
  } else if (!f.function_exists()) {
    // ...
  } else if (f.result_failed()) {
    // ...
  } else if (!f.result_enough()) {
    std::cout << "Result number of f " << f.real_result_size()
              << " less than " << f.expected_result_size() << std::endl;
  } else {
    // May never happen
  }
}
```

### 4. Bind C++ functions to Lua (also lambda, std::function or callable objects)

Uses same API as method "set". 

It supports:
* C style functions (not variadic)
* C++ overloaded functions
* template functions
* lambda
* std::function
* Any callable classes: should provide concrete hint type or luaw::function_tag

It doesn't support:
* C style variadic functions (such as function "printf")
* Unspecialized template functions
* Unspecialized generic lambda
* Unspecialized callable objects
* Default arguments defined in C++: will be default-initialized if no real parameters given in Lua


#### 4.1 Bind C/C++ functions

* The functions could have arbitary number of arguments
* Could use either C function reference or C function pointer
* Use std::tuple to represent multiple return values.
* C++ template functions should be explicitly specialized, or provide function proto type as hint
* When binding C++ overloaded funtions: should provide function proto type or its pointer type as hint

Example:

```C++
int fadd(int x, int y) { return a + b; }
int fadd_many(int a, int b, int c, int d, int e, int f, int g 
              /* define as many as you want */) {
  return a + b + c + d + e + f + g;
}

int main {
  peacalm::luaw l;

  // These are equivalent:
  l.set("fadd", fadd);  // set function directly
  l.set("fadd", &fadd); // set function address
  auto pfadd = &fadd;
  l.set("fadd", pfadd); // set function pointer
  auto& ref = fadd;
  l.set("fadd", ref);   // set function reference

  assert(l.eval<int>("return fadd(1, 2)") == 3);

  l.set("fadd_many", fadd_many);
  // Arguments not given real parameter will be default-initialized to zero,
  // which do not affect the function's result.
  assert(l.eval<int>("return fadd_many(1, 2, 3, 4)") == 10);
}
```

Multiple returns: use std::tuple!

```C++
std::tuple<int, int> fdiv(int x, int y) { return std::make_tuple(x / y, x % y); }

int main() {
  peacalm::luaw l;
  l.set("fdiv", fdiv);
  int retcode = l.dostring("q, r = fdiv(7, 3)");
  assert(retcode == LUA_OK);
  assert(l.get_int("q") == 2);
  assert(l.get_int("r") == 1);
}
```

C++ template functions: should specialize or provide function proto type as hint.

```C++
template <typename T>
T tadd(T a, T b) { return a + b; }
int main() {
  peacalm::luaw l;

  // Explicitly specialize the function
  l.set("tadd", tadd<int>);

  // Or provide the function proto type as hint
  l.set<double(double, double)>("tadd", tadd);
}
```

C++ overloaded functions: should provide function proto type or its pointer type as hint.

```C++
// Overloaded functions f
int f(int i) { return i * 2; }
double f(double d) { return d / 2; }
double f(double a, double b) { return a * b; }

int main() {
  peacalm::luaw l;

  // Provide the function proto type as hint
  l.set<int(int)>("f1", f);
  l.set<double(double)>("f2", f);
  l.set<double(double, double)>("f3", f);

  // Or could use function pointer proto type as hint
  l.set<int(*)(int)>("f1", f);
  l.set<double(*)(double)>("f2", f);
  l.set<double(*)(double, double)>("f3", f);

  assert(l.eval_bool("return f1(2) == 4"));
  assert(l.eval_bool("return f2(5) == 2.5"));
  assert(l.eval_bool("return f3(1.25, 10) == 12.5"));
  return 0;
}
```
#### 4.2 Bind lambdas


* Captureless lambda: May not provide hint type, like C function.

```C++
l.set("add", [](int a, int b) { return a + b; });
```

* Lambda with captured variables: should provide concrete hint type or use luaw::function_tag as hint.

```C++
int x = 1;
auto f = [&](int a) { return a + x; };

// Provide a function type hint: int(int)
l.set<int(int)>("add", f);

// Alternative writing: could use luaw::function_tag as hint type
l.set<peacalm::luaw::function_tag>("add", f);
```

* Generic lambda: should provide concrete hint type.

```C++
// Generic lambda
auto f = [](auto a, auto b) { return a + b; }

// Should provide a function proto type as hint
l.set<int(int, int)>("add", f);

// The lambda object can be reused with other function proto types
l.set<double(double, double)>("add2", f);
```

#### 4.3 Bind std::function

std::function has already contained function proto type, so we could set it directly.

```C++
std::function<int(int, int)> f = [](auto a, auto b) { return a + b; };

// Set f by copy:
l.set("f", f);

// Or set f by move:
l.set("f", std::move(f));
```

#### 4.4 Bind callable classes

Actually, lambda or std::function, is just a callable class.
So we can write other callable classes and bind it to Lua to be used as a function.

```C++
struct Plus {
  int operator()(int a, int b) { return a + b; }
};
int main() {
  peacalm::luaw l;

  // function proto type as hint:
  l.set<int(int, int)>("plus", Plus{});

  // Or use function_tag:
  l.set<peacalm::luaw::function_tag>("plus", Plus{});
}
```


### 5. Bind C++ classes to Lua

Suppose here is a C++ class to bind:

```C++
// The C++ custom class to register
struct Obj {
  int        i  = 1;
  const int  ci = 1;
  static int si;

  Obj() {}
  Obj(int v) : i(v) {}
  Obj(int v, int cv) : i(v), ci(cv) {}
  
  int abs() const { return std::abs(i); }

  int plus() { return ++i; }
  int plus(int d) { i += d; return i; }

  static int sqr(int x) { return x * x; }
};
int Obj::si = 1;
```

#### 5.1 Register constructors

API:

```C++
/**
 * @brief Register a global function to Lua who can create object.
 * 
 * It creates a userdata in Lua, whose C++ type is specified by Return type
 * of Ctor. e.g. `register_ctor<Object(int)>("NewObject")`. Then can run 
 * `o = NewObject(1)` in Lua.
 * 
 * @tparam Ctor Should be a function type of "Return(Args...)". 
 * @param fname The global function name registered.
 */
template <typename Ctor>
void register_ctor(const char* fname);
template <typename Ctor>
void register_ctor(const std::string& fname);
```

Equivalent to setting a global function to Lua who can create an instance of the 
corresponding class using the class's constructor.

For example:
```C++
peacalm::luaw l;
l.register_ctor<Obj()>("NewObj");          // default constructor
l.register_ctor<Obj(int)>("NewObj1");      // constructor with 1 argument
l.register_ctor<Obj(int, int)>("NewObj2"); // constructor with 2 argument

// Then can use ctor as a global function in Lua
l.dostring("a = NewObj(); b = NewObj1(2); c = NewObj2(3, 4)");
```

We can register constructors for const instances, then the instance created in Lua
will be a const variable, and all members registered will be its conster member.

Example:
```C++
// the constructors will generate a const instance of Obj in Lua
using ConstObj = const Obj;
l.register_ctor<ConstObj()>("NewConstObj");          // default constructor
l.register_ctor<ConstObj(int)>("NewConstObj1");      // constructor with 1 argument
l.register_ctor<ConstObj(int, int)>("NewConstObj2"); // constructor with 2 argument

// Then can use ctor as a global function in Lua
l.dostring("a = NewConstObj(); b = NewConstObj1(1); c = NewConstObj2(1,2)");
```

#### 5.2 Register member variables

API:

```C++
/**
 * @brief Register member variables or member functions
 *
 * Register a real member for class, not fake members.
 * 
 * To register overloaded member functions, the template parameter
 * `MemberPointer` must be provided explicitly.
 * Otherwise, when registering member variables or non-overloaded member
 * functions, `MemberPointer` can be automatically deduced.
 *
 * Example:
 * 
 * Register int member "i" for class "Obj":
 *   `register_member("i", &Obj::i)`
 * or explicitly provide member type:
 *   `register_member<int Obj::*>("i", &Obj::i)`
 * 
 * Register member function "abs" for class "Obj":
 *   `register_member("abs", &Obj::abs)`
 * or explicitly provide member type:
 *   `register_member<int (Obj::*)()>("abs", &Obj::abs)`
 *
 * @tparam MemberPointer Member pointer type.
 * @param name Member name to be registered.
 * @param mp Pointer to the member to be registered.
 * @return void
 */
template <typename MemberPointer>
std::enable_if_t<std::is_member_pointer<MemberPointer>::value>
register_member(const char* name, MemberPointer mp);
template <typename MemberPointer>
std::enable_if_t<std::is_member_pointer<MemberPointer>::value>
register_member(const std::string& name, MemberPointer mp);
```

Member's const/volatile property is kept to Lua.
That is the const member's value can not be modified in Lua,
just like it does in C++.

Example:
```C++
l.register_member("i", &Obj::i);
l.register_member("ci", &Obj::ci); // const member

// Assume ctor is registered like that shows in above examples
l.dostring("a = NewObj(); a.i = 2");      // OK
l.dostring("a = NewObj(); a.ci = 2");     // Error: Const member cannot be modified: ci
l.dostring("a = NewConstObj(); a.i = 2"); // Error: Const member cannot be modified: i
```

#### 5.3 Register member functions

Uses same API as registering member variables.

If a member function is overloaded, then must explicitly provide the member 
function's type as hint.

Same as member variable, the const/volatile property of member function is also kept to Lua.
That is, we can't call it's non-const member function by a const object.

Example:

```C++
l.register_member("abs", &Obj::abs);

// Register overloaded member functions.
l.register_member<int (Obj::*)()>("plus", &Obj::plus);
l.register_member<int (Obj::*)(int)>("plusby", &Obj::plus);

l.dostring("a = NewObj(); a:plus()"); // OK
l.dostring("b = NewConstObj(); b:plus()"); // Error: Nonconst member function: plus
```

#### 5.4 Register static member variables

API:

```C++
/**
 * @brief Register a static member by providing class type.
 *
 * Can register static member variables or functions.
 * Also can register global functions or global variables or local variables
 * (should better be static) to be members of an object in Lua.
 *
 * When registering a function it will register it as a const member function
 * of object in Lua.
 * When registering a variable it will register it as an usual member variable
 * of object in Lua. In particular, the member can not be modified by a const
 * object (this is different with that in C++).
 *
 * Should register the member by it's name and address.
 *
 * For example:
 *
 * Register static member `Obj::si` of type `int` to Obj in Lua:
 * `register_static_member<Obj>("si", &Obj::si)`
 *
 * Register static member function `static int sf(int)` to Obj:
 * `register_static_member<Obj>("sf", &Obj::sf)`
 * or do not use "&" to get function's address is also ok:
 * `register_static_member<Obj>("sf", Obj::sf)`
 *
 * @tparam Class The class whom the static member will belong to.
 * @tparam T The static member's type.
 * @param name The static member's name.
 * @param m The static member's pointer.
 * @return void
 */
template <typename Class, typename T>
std::enable_if_t<std::is_class<Class>::value> register_static_member(
    const char* name, T* m);


/**
 * @brief Register a static member by providing full fake member pointer type.
 *
 * This API can add const/volatile property to member, which the origin static
 * member may not have.
 *
 * Others all same as that API providing class.
 *
 * For example:
 *
 * Register static member `Obj::si` of type `int` to be a const member of Obj
 * in Lua with type `const int`:
 * `register_static_member<const int Obj::*>("si", &Obj::si)`
 *
 * Register static member function `static int sf(int)` as a const member
 * function to Obj:
 * `register_static_member<int(Obj::*)(int) const>("sf", &Obj::sf)`
 *
 * @tparam MemberPointer What kind of member type to let the static member
 * behaves like in Lua.
 * @tparam T The static member's type.
 * @param name The static member's name.
 * @param m The static member's pointer.
 * @return void
 */
template <typename MemberPointer, typename T>
std::enable_if_t<std::is_member_pointer<MemberPointer>::value>
register_static_member(const char* name, T* m);
```

There are two kind of API:
* Register a static member by providing class type.
* Register a static member by providing full fake member pointer type.

We register static member variables like usual member variables, 
which **can not be modified by const objects**
(But in C++, it allows to modify a non-const static member variable by a const 
object. Here we do not follow the C++ way).
But not like usual member variables, the static member variable will be shared 
by all objects of same type.

By providing full fake member pointer type, we can add const/volatile property
for the static member.

Actually, except for registering real static members of a class,
these API can also register C++ global variables/functions or local variables
(should better be static) to be static members in Lua. 
They'll behave equivalently to real static members.

Example of that by providing class type:

```C++
peacalm::luaw l;
l.register_static_member<Obj>("si", &Obj::si);
l.set("o", Obj{});
assert(l.eval<int>("return o.si") == 1);
Obj::si = 2;
assert(l.eval<int>("return o.si") == 2);

int retcode = l.dostring("o.si = 3");  // modify
assert(retcode == LUA_OK);
assert(l.eval<int>("return o.si") == 3);
assert(Obj::si == 3);

// Const object
const Obj c;
l.set("c", c);
assert(l.eval<int>("return c.si") == 3);
assert(l.eval<bool>("return o.si == c.si")); // share same si

// Can not modify static member by const object in Lua
retcode = l.dostring("c.si = 4");
assert(retcode != LUA_OK);
l.log_error_out();
// Lua: [string "c.si = 4"]:1: Const member cannot be modified: si
assert(l.eval<int>("return c.si") == 3);
assert(Obj::si == 3);

// But can modify it in C++ by const object
c.si = 5;
assert(l.eval<int>("return c.si") == 5);
```

Example of that by providing full fake member pointer type:

```C++
peacalm::luaw l;
// Register non-static si as a const member in Lua
l.register_static_member<const int Obj::*>("si", &Obj::si);

l.set("o", Obj{});
Obj::si = 1;
assert(l.eval<int>("return o.si") == 1);

int retcode = l.dostring("o.si = 2");
assert(retcode != LUA_OK);
l.log_error_out();
// Lua: [string "o.si = 4"]:1: Const member cannot be modified: si

assert(l.eval<int>("return o.si") == 1);
Obj::si = 2;
assert(l.eval<int>("return o.si") == 2);
```


#### 5.5 Register static member functions

Uses same API as registering static member variables.

We register static functions as a const member function of an object in Lua,
so it can be called by const objects.


Example:

```C++
peacalm::luaw l;
// By providing class type
l.register_static_member<Obj>("sqr", &Obj::sqr);

l.set("o", Obj{});
assert(l.eval<int>("return o:sqr(2)") == 4);

l.set("c", std::add_const_t<Obj>{});
assert(l.eval<int>("return c:sqr(3)") == 9);

l.set("s", std::make_shared<Obj>());
assert(l.eval<int>("return s:sqr(4)") == 16);

l.set("sc", std::make_shared<const Obj>());
assert(l.eval<int>("return sc:sqr(5)") == 25);
```

Or, in the last example, we can register the static function "sqr" 
by providing full fake member pointer. 
But remember to add "const" for the member function pointer type. Otherwise
it cannot be called by a const object.


Example:

```C++
peacalm::luaw l;
// By providing full fake member pointer type
l.register_static_member<int (Obj::*)(int) const>("sqr", &Obj::sqr);

// ... (Same as previous example)
```

#### 5.6 Register dynamic member variables

API:

```C++
/**
 * @brief Register dynamic members
 *
 * Dynamic members are members whose names (and also values) are dynamically
 * defined at run time, such as keys of a table in Lua.
 * So we can't register them in advance.
 *
 * To register dynamic members for a C++ class, it should have a place to
 * store the members first, usually we can define a map in the class to store
 * the dynamic members.
 *
 * Then we should provide two callable objects, getter and setter, which
 * introduces Lua how to get and set a dynamic member separately.
 *
 * The getter's proto type must be: `Value(const Class*, Key)`
 * The setter's proto type must be: `void(Class*, Key, Value)`
 *
 * Where `Key` type could be `const std::string&` or 'const char*', the former
 * is recommended. `Value` type could be number, string, bool,
 * luaw::luavalueidx, luaw::luavalueref, etc.
 *
 * @tparam Getter Must be a callable type, e.g. C function or lambda.
 * @tparam Setter Must be a callable type, e.g. C function or lambda.
 * @param getter A method to get a member by name.
 * @param setter A method to set a member's value.
 */
template <typename Getter, typename Setter>
void register_dynamic_member(Getter&& getter, Setter&& setter);
template <typename Getter>
void register_dynamic_member_getter(Getter&& getter);
template <typename Setter>
void register_dynamic_member_setter(Setter&& setter);
```

Dynamic members are members whose name is dynamically defined in Lua script, 
just like keys of a table, we can't know its name and value in advance.

So we register two functions for a class, dynamic member getter and setter,
to support this.

For example, if we want to support dynamic members for class `Foo` 
with string type key, any type of value, then we can use `luaw::luavalueref` as 
value type.

```C++
struct Foo {
  // If value types of dynamic members are unknown, use luaw::luavalueref
  std::unordered_map<std::string, peacalm::luaw::luavalueref> m;
};

peacalm::luaw::luavalueref foo_dm_getter(const Foo* o, const std::string& k) {
  auto entry = o->m.find(k);
  if (entry != o->m.end()) { return entry->second; }
  return peacalm::luaw::luavalueref(); // default value is nil
}

void foo_dm_setter(Foo* o, const std::string& k, const peacalm::luaw::luavalueref& v) {
  o->m[k] = v;
}

int main() {
  peacalm::luaw l;
  l.register_dynamic_member(foo_dm_getter, foo_dm_setter);
  l.register_ctor<Foo()>("NewFoo");
  l.dostring("a = NewFoo(); a.name = 'foo'; a.x = 1; a.y = 2; print(a.name, a.x, a.y, a.z)");
  // ...
}
```

If we expect the dynamic members' value type is float number, then directly 
use C++ float number type, e.g. `double`, as value type is more efficient:

```C++
struct Foo {
  // All the dynamic members' values are float number.
  std::unordered_map<std::string, double> m;
};

double foo_dm_getter(const Foo* o, const std::string& k) {
  auto entry = o->m.find(k);
  if (entry != o->m.end()) { return entry->second; }
  return 0; // suppose member's default value is 0
}

void foo_dm_setter(Foo* o, const std::string& k, double v) {
  o->m[k] = v;
}

int main() {
  peacalm::luaw l;
  l.register_dynamic_member(foo_dm_getter, foo_dm_setter);
  l.register_ctor<Foo()>("NewFoo");
  l.dostring("a = NewFoo(); a.x = 1; a.y = 2;");
  // ...
}
```

#### 5.7 Register fake member variables

API:

```C++
/**
 * @brief Register a fake member variable or fake member function.
 *
 * Fake members are members who are not directly explicitly defined as members
 * in a C++ class, but used in Lua and by some way they can reach to values or
 * functions in C++.
 *
 * The values reached by fake members could be real members, or temporary
 * values generated by some method, or even variables outside the calss, e.g.
 * some global variables.
 * 
 * In particular, we can fake members by class's static members.
 *
 * To register a fake member, we must provide a Hint type to indicate the
 * mermber's type to be faked, and a callable object to describe the member's
 * behavior. The first argument of the callable object must represent the
 * class which the members faked belong to, usually we use a pointer to the
 * class.
 *
 * For example:
 *
 * Register a const member "id" with type void* for class Obj to get its
 * instance's address:
 *     `register_member<void* const Obj::*>("id", [](const Obj* p) {
 *         return (void*)p; });`
 *
 * Register a member function "plus" for class Obj:
 *     `register_member<void (Obj::*)(int)>("plus", [](Obj* p, int d) {
 *         p->value += d; })`
 *
 * Register a const member function "getvalue" for class Obj:
 *     `register_member<int (Obj::*)() const>("getvalue", [](const Obj* p) {
 *         return p->value; })`
 *
 * @tparam Hint The member type wanted to fake.
 * @tparam F C function type or lambda or std::function or any callable type.
 * @param name The member name.
 * @param f The function whose first parameter is a pointer to the class.
 * @return void.
 */
template <typename Hint, typename F>
std::enable_if_t<std::is_member_pointer<Hint>::value && !std::is_same<Hint, F>::value>
register_member(const char* name, F&& f);
template <typename Hint, typename F>
std::enable_if_t<std::is_member_pointer<Hint>::value && !std::is_same<Hint, F>::value>
register_member(const std::string& name, F&& f);
```

Except real members of a class, we can also register fake members 
(both variables and functions) by some special function or lambda, 
and with a hint type which indicates the member's type that we want to fake.

When faking a member variable, the function's first and only argument must be 
declared as one of the two kind:
* a pointer to const qualified class, e.g. `const Obj* o`,
* or `auto*` when registering a fake member variable by lambda.

And in function/lambda body, sometimes a `const_cast` to remove the pointer's 
underlying cv- property is needed to avoid compilation errors.

Why `auto*` not `auto` in the second kind? Of course using `auto` is also ok,
but here we want to emphasize that the argument must be a pointer type,
so we explicitly define it as a pointer using `auto*`.


For example, fake a const member "id" for class Obj:

```C++
// Fake a const member "id" whose value is the object's address
l.register_member<void* const Obj::*>(
    "id", [](const Obj* o) { return (void*)o; });
// Or use `auto*` as argument type
l.register_member<void* const Obj::*>(
    "id2", [](auto* o) { return (void*)o; });

Obj o;
l.set("o", &o); // lightuserdata
assert(l.eval<void*>("return o.id") == (void*)(&o));
assert(l.eval<void*>("return o.id2") == (void*)(&o));
assert(l.eval_bool("return o.id == o.id2"));

// For smart pointer of Obj, id also returns the underlying's address
auto so = std::make_shared<Obj>();
l.set("so", so);
assert(l.eval<void*>("return so.id") == (void*)(so.get()));
l.set("so2", so);
assert(l.eval_bool("return so.id == so2.id"));
assert(l.eval_bool("return so ~= so2"));
```

Or we can fake const members generated by operations on other members:

```C++
// Fake a const member 'sum' which is the sum of member i and member ci
l.register_member<const int Obj::*>(
  "sum", [](const Obj* o) { return o->i + o->ci; });

// Fake a const member 'q' which is the quotient of member i and member ci
l.register_member<const double Obj::*>(
  "q", [](const Obj* o) { return double(o->i) / double(o->ci); });

l.set("o", Obj(3, 2));
assert(l.eval<int>("return o.sum") == 5);
assert(l.eval<double>("return o.q") == 1.5);
```

If want to fake a mutable member, then the special function must return lvalue 
reference to a real variable in C++. For example:

```C++
int  gi = 100;  // global variable i
int& getgi(const Obj* o) { return gi; }

int main() {
  peacalm::luaw l;

  l.set("o", Obj{});
  l.register_member<int Obj::*>("gi", &getgi);

  assert(l.eval<int>("return o.gi") == gi);
  assert(l.eval<int>("o.gi = 101; return o.gi") == 101);
  assert(gi == 101);
}
```

Or we can fake member variables using lambda's capture:

```C++
peacalm::luaw l;

l.set("o", Obj{});
int  li    = 100;  // local variable i
auto getli = [&](const Obj*) -> int& { return li; };
l.register_member<int Obj::*>("li", getli);

assert(l.eval<int>("return o.li") == li);
assert(l.eval<int>("o.li = 101; return o.li") == 101);
assert(li == 101);
```

Or we can fake member variables by dereference of pointer members of the class:

```C++
class Foo {
public:
  ~Foo() { delete pi; }
  int* pi = new int(1);
};

int main() {
  peacalm::luaw l;

  // Fake a member 'i' for Foo in Lua by dereference the pointer member 'pi'
  // The member 'i' is mutable in Lua
  l.register_member<int Foo::*>(
      "i", [](const Foo* o) -> int& { return *o->pi; });
  
  auto o = std::make_shared<Foo>();
  l.set("o", o);

  assert(l.eval<int>("return o.i") == 1);
  assert(l.eval<int>("return o.i") == *o->pi);

  *o->pi = 2;
  assert(l.eval<int>("return o.i") == 2);

  assert(l.eval<int>("o.i = 3; return o.i") == 3);
  assert(*o->pi == 3);

  // const instance of Foo can also access 'i', but cannot modify it
  auto c = std::make_shared<const Foo>();
  l.set("c", c);
  assert(l.eval<int>("return c.i") == 1);
  assert(l.dostring("c.i = 2") != LUA_OK);
  l.log_error_out(); // Const member cannot be modified: i
  return 0;
}
```

Sometimes we want to fake a mutable member variable by real member variable of
the class, but if we declare the first formal parameter as cv-qualified,
it will make a compilation error when binding reference of non-const variable 
to const object's member.
In this case we have two choices:
* Use a `const_cast` in function body to remove the pointer's underlying cv- property.
* Declare the first argument as `auto*`, and declare the return as `auto&`, when faking by lambda.

Example:

```C++
struct Foo {
  int a[3];
};

int main() {
  peacalm::luaw l;

  // 1. use const_cast
  l.register_member<int Foo::*>("a0", [](const Foo* o) -> int& {
    // use const_cast to remove const property forcely
    return const_cast<Foo*>(o)->a[0];
  });
  // 2. use `auto*` in argument, use `auto&` in return
  l.register_member<int Foo::*>("a1", [](auto* o) -> auto& {
    return o->a[1];
  });
  // 3. mix the two usage together
  l.register_member<int Foo::*>("a2", [](auto* o) -> int& {
    // use const_cast to ensure it is not cv-qualifed
    return const_cast<Foo*>(o)->a[2];
  });

  auto o = std::make_shared<Foo>();
  l.set("o", o);

  assert(l.dostring("o.a0 = 1; o.a1 = 2; o.a2 = 3") == LUA_OK);
  assert(o->a[0] == 1);
  assert(o->a[1] == 2);
  assert(o->a[2] == 3);

  return 0;
}
```

#### 5.8 Register fake member functions

Uses same API as registering fake member variables.

Use a function whose first parameter is a pointer to the class, and provide a
hint type to indicate what type of member want to fake. 
Example:

```C++
// functions to fake as members of Obj
void seti(Obj* p, int v) { p->i = v; }
int  geti(const Obj* p) { return p->i; }

int main() {
  peacalm::luaw l;
  l.register_member<void (Obj::*)(int)>("seti", &seti);
  l.register_member<int (Obj::*)() const>("geti", geti);
  // use lambda
  l.register_member<int (Obj::*)()>("isqr", [](const Obj* p){ return p->i * p->i;});

  auto o = std::make_shared<Obj>();
  l.set("o", o);
  o->i = 1;
  assert(l.eval<int>("return o:geti()") == 1);
  l.dostring("o:seti(5)");
  assert(l.eval<int>("return o:geti()") == 5);
  assert(o->i == 5);
  assert(l.eval<int>("return o:isqr()") == 25);
}
```


#### 5.9 Set class instances to Lua

Use "set" method, we can set a class's instance which is defined in C++ to Lua,
just like set other type values.

Use "set_ptr_by_wrapper", we can a raw pointer as a full userdata into Lua.
The raw pointer should not be pointer to smart pointers. 
In this case, It's user's responsibility to make sure the raw pointer is valid while using it in Lua. Otherwize the behavior is undefined.


API for "set_ptr_by_wrapper":

```C++
/**
 * @brief Set a raw pointer by a wrapper (full userdata)
 *
 * A raw pointer in Lua is a light userdata, and it doesn't have per-value
 * metatable. But a pointer wrapper is a full userdata, and it has per-value
 * metatable.
 *
 * This method makes a wrapper for a raw pointer first, then set the wrapper
 * into Lua. The pointer wrapper can access all members registered just like
 * the raw pointer.
 *
 * It's user's responsibility to make sure the raw pointer is valid while
 * using it in Lua. Otherwize the behavior is undefined.
 *
 * @tparam T The type that the raw pointer points to. Can't be smart pointers.
 * @param name The pointer wrapper's name used in Lua.
 * @param p The raw pointer.
 */
template <typename T>
void set_ptr_by_wrapper(const char* name, T* p);
template <typename T>
void set_ptr_by_wrapper(const std::string& name, T* p);
```


Here are multiple ways to set a class's instance to Lua, 
all the object setted to Lua by these ways can access members that have been registered.

These 5 ways make a full userdata to Lua:

* Set instance by copy
* Set instance by move
* Set smart pointer to instance by copy
* Set smart pointer to instance by move
* Set raw pointer by wrapper

These 2 ways make a light userdata to Lua:

* Set raw pointer to instance
* Set raw pointer to smart pointer to instance

For example:

```C++
// ----- set full userdata

Obj a;
l.set("o1", a);            // by copy
l.set("o2", std::move(a)); // by move
l.set("o3", Obj{});        // by move

auto s = std::make_shared<Obj>();
l.set("o4", s);                       // by copy of shared_ptr
l.set("o5", std::move(s));            // by move of shared_ptr
l.set("o6", std::make_shared<Obj>()); // by move of shared_ptr

auto u = std::make_unique<Obj>();
l.set("o7", std::move(u));            // by move of unique_ptr
l.set("o8", std::make_unique<Obj>()); // by move of unique_ptr

/* Suppose a deleter has been defined like:
struct ObjDeleter {
  void operator()(Obj* p) const { delete p; }
};
*/

auto ud = std::unique_ptr<Obj, ObjDeleter>(new Obj, ObjDeleter{});
// by move of unique_ptr with user defined deleter
l.set("o9", std::move(ud));
// by move of unique_ptr with user defined deleter
l.set("o10", std::unique_ptr<Obj, ObjDeleter>(new Obj, ObjDeleter{}));

// set raw pointer by wrapper
l.set_ptr_by_wrapper("o11", &a);

// ----- set light userdata

Obj a2;
l.set("o12", &a2);           // by pointer

auto s2 = std::make_shared<Obj>();
auto u2 = std::make_unique<Obj>();
auto ud2 = std::unique_ptr<Obj, ObjDeleter>(new Obj, ObjDeleter{});
l.set("o13", &s2);   // by raw pointer to shared_ptr
l.set("o14", &u2);   // by raw pointer to unique_ptr
l.set("o15", &ud2);  // by raw pointer to unique_ptr with user defined deleter

```

Then all variables "o1" ~ "o15" setted to Lua can access members of Obj that have been registered.

But "o1" ~ "o11" are full userdata, "o12" ~ "o15" are light userdata, 
their metatables are different, although they have some same meta methods.
And the biggest difference is that full userdata have per-value metatables,
but light userdata don't. **All light userdata share the same metatable**, 
which by default is not set (nil).

So once you set a raw pointer of a class object to Lua, like "o12" ~ "o15", 
it will generate a metatable which will take effect on all light userdata!

This feature of Lua makes you can call a light userdata's meta methods with wrong metatable, whose behavior is undefined! 
This might make people very confused! And that's dangerous!

So, be careful if you want to set an object to Lua by light userdata!
Make sure you won't set objects with different types by light userdata at same time!

We recommend you to use "set_ptr_by_wrapper" if you really have to set an 
object's raw pointer into Lua.

##### Light userdata's metatable operations

API:

```C++
/**
 * @brief Set lightuserdata's metatable by a pointer type.
 *
 * Light userdata (unlike heavy userdata) have no per-value metatables. All
 * light userdata share the same metatable, which by default is not set (nil).
 *
 * This method builds a metatable by a pointer type then set it to all light
 * userdata.
 *
 * Behavior of light userdata with wrong type's metatable is undefined!
 *
 * @tparam T A pointer type indicates whose metatable lightuserdata use.
 */
template <typename T>
void set_lightuserdata_metatable();

/// Remove lightuserdata's metatable, i.e. set nil as metatable.
void clear_lightuserdata_metatable();

/// Tell whether lightuserdata has metatable (not nil).
bool lightuserdata_has_metatable();


/**
 * @brief Get lightuserdata's metatable name
 *
 * @param [in] def Value returned if lightuserdata doesn't have metatable or
 * "__name" doesn't exist in it's metatable or the value in metatable paired
 * to "__name" can't convert to string.
 * @param [out] has_metatable Will be set whether lightuserdata has metatable.
 * @param [in] disable_log Whether print a log when exception occurs.
 * @param [out] failed Will be set whether converting the value paired to
 * "__name" in lightuserdata's metatable to string failed.
 * @param [out] exists Will be set whether "__name" exists in lightuserdata's
 * metatable.
 * @return std::string
 */
std::string get_lightuserdata_metatable_name(const std::string& def = "",
                                             bool* has_metatable    = nullptr,
                                             bool  disable_log      = false,
                                             bool* failed           = nullptr,
                                             bool* exists = nullptr);

```

##### get metatable name (not only for light userdata, but for all variables)

API:

```C++
/// Get metatable name for value at given index.
std::string get_metatable_name(int                idx           = -1,
                               const std::string& def           = "",
                               bool*              has_metatable = nullptr,
                               bool               disable_log   = false,
                               bool*              failed        = nullptr,
                               bool*              exists        = nullptr);

/// Get a variable's metatable name by a given path.
std::string get_metatable_name(@PATH_TYPE@ path,
                               const std::string& def           = "",
                               bool*              has_metatable = nullptr,
                               bool               disable_log   = false,
                               bool*              failed        = nullptr,
                               bool*              exists        = nullptr);
```

where `@PATH_TYPE@` could be:

1. `const char*` or `const std::string&`.

Then the formal parameter `path` is a global variable's name in Lua.

2. `const std::initializer_list<const char*>&`, `const std::initializer_list<std::string>&`, `const std::vector<const char*>&` or`const std::vector<std::string>&`.

Then the formal parameter `path` is a path to a subfield/member of a variable.


Example:

```C++
struct Foo {
  int v = 1;
};

struct Bar {
  const int cv = 2;
};

int main() {
  peacalm::luaw l;

  // By default light userdata's metatable is nil
  assert(!l.lightuserdata_has_metatable());
  assert(l.get_lightuserdata_metatable_name().empty());

  /* Set light userdata without metatable! Raw pointer to non-class type. */
  
  int p = 0;
  // Set pointer to non-class type, won't generate metatable, 
  // though it's light user data too.
  l.set("p", &p);
  assert(!l.lightuserdata_has_metatable());
  assert(l.get_lightuserdata_metatable_name().empty());

  // Another way to get metatable of "p", 
  // this way could be used for full userdata too!
  std::string metaname0 = l.get_metatable_name("p");
  // "p" doesn't have metatable
  assert(metaname0.empty());

  /* Set light userdata with metatable! Raw pointer to class type. */

  Foo a;
  Bar b;

  l.register_member("v", &Foo::v);

  l.set("a", &a);
  assert(l.lightuserdata_has_metatable());
  std::string metaname1 = l.get_lightuserdata_metatable_name();

  assert(l.dostring("assert(a.v == 1)") == LUA_OK);

  l.set("b", &b);
  assert(l.lightuserdata_has_metatable());
  std::string metaname2 = l.get_lightuserdata_metatable_name();
 
  // Metatable for light userdata is changed!
  assert(metaname1 != metaname2);

  // metatable of "a" is changed so "a.v" doesn't work now.
  assert(l.dostring("assert(a.v == nil)") == LUA_OK);
  assert(l.dostring("assert(b.v == nil)") == LUA_OK);

  // Get metatable of "p"
  std::string metaname3 = l.get_metatable_name("p");
  // Now "p" has metatable too!
  assert(metaname3 == metaname2);

  /* How to clear light userdata's metatable */

  l.clear_lightuserdata_metatable();
  assert(!l.lightuserdata_has_metatable());
  assert(l.get_lightuserdata_metatable_name().empty());

  /* Set light userdata's metatable by class pointer type */

  l.set_lightuserdata_metatable<Foo*>(); // set same metatable as setting "a"
  std::string metaname4 = l.get_lightuserdata_metatable_name();
  assert(metaname4 == metaname1);

  assert(l.dostring("assert(a.v == 1)") == LUA_OK);
  // Now "b" has a metatable which should belong to "a".
  // "b.v" is likely to be 2, but this usage is not recommended.
  // So we don't make assert.
  l.dostring("print('b.v = ', b.v)");

  l.set_lightuserdata_metatable<Bar*>(); // set same metatable as setting "b"
  std::string metaname5 = l.get_lightuserdata_metatable_name();
  assert(metaname5 == metaname2);

  assert(l.dostring("assert(b.v == nil)") == LUA_OK);

  return 0;
}
```

##### Set const instances to Lua

Const property of an instance in C++ will be perfectly transfered to Lua using
"set" method.

Example:

```C++
const Obj co;
l.set("co", co); // set const instance by copy
// or
l.set("co", std::move(co)); // set const instance by move
// or
l.set("co", std::add_const_t<Obj>{}); // set const instance by move
// or
const Obj co2;
// set low-level const pointer to be a lightuserdata which is also const
l.set("co", &co2); 
// The variable "co" (set by any method above) is const in Lua, 
// it can access all registered member variables (but can't modify) and 
// registered const member functins of Obj.


// Set underlying const instance using smart pointer.
// "sco" is not const, by it's underlying object is const,
// so it behaves same as "co":
auto sco = std::make_shared<const Obj>();
l.set("sco", sco); // by copy
l.set("sco", std::move(sco)); // by move
l.set("sco", std::make_shared<const Obj>()); // by move
// or
auto sco2 = std::make_shared<const Obj>();
l.set("sco", &sco2); // set as lightuserdata, also underlying const
```

#### 5.10 Get instances from Lua

Use "get" method, we can get an instance or a pointer of an instance from Lua.

When getting instances (not pointer), it could only copy the object from Lua 
to C++, cannot move.

Example:

```C++
peacalm::luaw l;
l.register_ctor<Obj()>("NewObj");
l.register_member("i", &Obj::i);

int retcode = l.dostring("a = NewObj(); a.i = 2;");
assert(retcode == LUA_OK);

Obj a = l.get<Obj>("a");  // by copy
assert(a.i == 2);
assert(a.ci == 1);

Obj* p = l.get<Obj*>("a");  // get a pointer to "a"
assert(p->i == 2);
assert(p->ci == 1);

p->i = 3;
assert(p->i == 3);
assert(l.dostring("assert(a.i == 3)") == LUA_OK);

// for light userdata
Obj b;
l.set("b", &b); // set a light userdata
b.i = 4;
assert(l.dostring("assert(b.i == 4)") == LUA_OK);
Obj* pb = l.get<Obj*>("b");  // get the light userdata and convert to Obj*
assert(pb == &b);

Obj c = l.get<Obj>("b"); // copy "b" to "c"
assert(c.i == 4);
c.i = 5;
assert(l.dostring("assert(b.i == 4)") == LUA_OK); // no change in "b"

// set and get by shared_ptr
auto s = std::make_shared<Obj>();
l.set("s", s); // set s by copy
auto s2 = l.get<std::shared_ptr<Obj>>("s"); // get s by copy
assert(s.get() == s2.get());
```


#### 5.11 Const property

The const property of class instance or class member behaves same in Lua as that in C++.

* Const member variable cannot be modified.
* Cannot modify any member by const class instance or pointer to const class instance.
* Cannot call nonconst member functions by const class instance or pointer to const class instance.
* If the class instance is a smart pointer, only low level const property is concerned. (high level const, which is const about the smart pointer it self, has no effect)

Example:
```C++
l.register_member("i", &Obj::i);
l.register_member<int (Obj::*)()>("plus", &Obj::plus);


const Obj o;
l.set("o1", o); // set a const instance by copy
l.dostring("o1.i = 2"); // error

const auto s = std::make_shared<Obj>(); // high level const
l.set("o2", s);
l.dostring("o2.i = 2"); // OK

auto cs = std::make_shared<const Obj>(); // low level const
l.set("o3", cs);
l.dostring("o3.i = 2"); // error
```


#### 5.12 Register nested class members (Register member of member)

Class members can be registered in same way as simple type of members.

So we can access member of member in Lua, 
but each time when we access a class member, we'll get a copy of the member.
This is not efficient.

Also we can modify a class member, but we can only modify the whole class member,
can't simply modify the member's member.
This is not converient.


Example:

```C++
struct A { int i = 1; };
struct B { A a; };
int main() {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("a", &B::a);

  auto b = std::make_shared<B>();
  l.set("b", b);

  assert(l.dostring(R"(
    assert(b.a.i == 1)

    -- Wrong way of modifying member of member
    b.a.i = 2
    assert(b.a.i == 1)

    -- Correct way of modifying member of member
    t = b.a  -- Get a copy of b.a
    t.i = 2  -- Modify the copy
    b.a = t  -- Modify the whole member b.a by the copy
    assert(b.a.i == 2)
  )") == LUA_OK);
}
```


#### 5.13 Register pointer of member variables (Raw pointer, which is a light userdata)

To access and modify member of member efficiently, we can register a member's
pointer as a fake member in Lua. And if we only want to access member of member,
not modify, we also can register the member's (low-level) const pointer.

But the pointer is a raw pointer, which is a light userdata in Lua, 
it doesn't have per-value metatable. 
So the pointer should be used very carefully.

API:
``` C++
/**
 * @brief Register a member's pointer into Lua.
 *
 * Register a fake member into Lua, the fake member is a pointer of a class's
 * real member.
 *
 * Since when getting a member, it will return a copy of the member. So we
 * can't modify or access efficiently members of the member.
 *
 * This feature is used to modify or access efficiently members'
 * members by getting a member's pointer first then modify or access members
 * of the member by the pointer.
 *
 * The fake member, namely the member's pointer, is always top-level const.
 *
 * When using a member's pointer, it is the user's responsibility to make
 * sure the member is alive, which means the object which holds the member is
 * alive. Using a pointer to an already recycled member is dangerous, the
 * behavior is undefined.
 *
 *
 * @tparam Class Should be decayed class type.
 * @tparam Member Can't be raw pointer or smart pointer type.
 * @param name The member's pointer name used in Lua.
 * @param mp Member pointer value.
 */
template <typename Class, typename Member>
void register_member_ptr(const char* name, Member Class::*mp);
template <typename Class, typename Member>
void register_member_ptr(const std::string& name, Member Class::*mp);

/**
 * @brief Register a member's low-level const pointer into Lua.
 *
 * No matter whether the member is already const, this can always register a
 * low-level const pointer of the member into Lua. (Of course the pointer is
 * top-level const too.)
 *
 * This feature is used to access member of member, can't modify.
 * Others are similar to "register_member_ptr"
 *
 * @sa "register_member_ptr"
 */
template <typename Class, typename Member>
void register_member_cptr(const char* name, Member Class::*mp);
template <typename Class, typename Member>
void register_member_cptr(const std::string& name, Member Class::*mp);
```

Example:

```C++
struct A { int i = 1; };
struct B { A a; };
int main() {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  // register pointer of B::a, can access and modify member
  l.register_member_ptr("aptr", &B::a);
  // register (low-level) const pointer of B::a, can only access member
  l.register_member_cptr("acptr", &B::a);

  auto b = std::make_shared<B>();
  l.set("b", b);

  assert(l.dostring(R"(
    assert(b.aptr.i == 1)
    assert(b.acptr.i == 1)

    b.aptr.i = 2  -- Can modify member of a by aptr
    assert(b.aptr.i == 2)
    assert(b.acptr.i == 2)
  )") == LUA_OK);

  // Can't modify member of a by acptr
  assert(l.dostring("b.acptr.i = 3") != LUA_OK);
  l.log_error_out(); // Const member cannot be modified: i
}
```


#### 5.14 Register reference of member variables (which is a full userdata)

Like registering pointer of member variables, we can also register reference of 
member variables. The only difference is that this reference is a full userdata,
and it has an exclusive metatable, it won't be disturbed by other references.

Use the reference to access or modify member of member in same way as using 
pointer of member.


API:
```C++
/**
 * @brief Register a member's reference into Lua.
 *
 * When registering a member's pointer into Lua using "register_member_ptr",
 * it registers a member's raw pointer, which is a light userdata, so it does
 * not have an exclusive metatable, and it is very dangerous to use a light
 * userdata with a wrong metatable.
 *
 * So this feature is to register a member's reference which can access and
 * modify efficiently the member, just like "register_member_ptr", and have
 * an exclusive metatable.
 *
 * Currently the reference is implimented by a std::shared_ptr holding a raw
 * member's pointer and without deleter. So it is a full userdata and has
 * per-value metatables.
 *
 * The member's reference is also a fake member, and is also always top-level
 * const like member's pointer.
 *
 * When using a member's reference, it is the user's responsibility to make
 * sure the member is alive, which means the object which holds the member is
 * alive. Using a reference to an already recycled member is dangerous, the
 * behavior is undefined.
 *
 * @tparam Class Should be decayed class type.
 * @tparam Member Can't be raw pointer or smart pointer type.
 * @param name The member's reference name used in Lua.
 * @param mp Member pointer value.
 * @sa "register_member_ptr"
 */
template <typename Class, typename Member>
void register_member_ref(const char* name, Member Class::*mp);
template <typename Class, typename Member>
void register_member_ref(const std::string& name, Member Class::*mp);

/**
 * @brief Register a member's low-level const reference into Lua.
 *
 * Like "register_member_ref", but this will register a low-level const
 * reference (Of course it is also top-level const). The const reference can
 * only be used to access members, can not modify.
 *
 * @sa "register_member_ref"
 */
template <typename Class, typename Member>
void register_member_cref(const char* name, Member Class::*mp);
template <typename Class, typename Member>
void register_member_cref(const std::string& name, Member Class::*mp);
```

Example:

```C++
struct A { int i = 1; };
struct B { A a; };
int main() {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  // register reference of B::a, can access and modify member
  l.register_member_ref("aref", &B::a);
  // register (low-level) const reference of B::a, can only access member
  l.register_member_cref("acref", &B::a);

  auto b = std::make_shared<B>();
  l.set("b", b);

  assert(l.dostring(R"(
    assert(b.aref.i == 1)
    assert(b.acref.i == 1)

    b.aref.i = 2  -- Can modify member of a by aref
    assert(b.aref.i == 2)
    assert(b.acref.i == 2)
  )") == LUA_OK);

  // Can't modify member of a by acref
  assert(l.dostring("b.acref.i = 3") != LUA_OK);
  l.log_error_out(); // Const member cannot be modified: i
}
```


### 6. Evaluate a Lua expression and get the results

#### 6.1 Evaluate to get a simple type result. Default result supported.

In the following API, `@EXPR_TYPE@` could be `const char*` or `const std::string&`.

```C++
bool               eval_bool   (@EXPR_TYPE@ expr, const bool&               def = false, bool disable_log = false, bool* failed = nullptr);
int                eval_int    (@EXPR_TYPE@ expr, const int&                def = 0,     bool disable_log = false, bool* failed = nullptr);
unsigned int       eval_uint   (@EXPR_TYPE@ expr, const unsigned int&       def = 0,     bool disable_log = false, bool* failed = nullptr);
long               eval_long   (@EXPR_TYPE@ expr, const long&               def = 0,     bool disable_log = false, bool* failed = nullptr);
unsigned long      eval_ulong  (@EXPR_TYPE@ expr, const unsigned long&      def = 0,     bool disable_log = false, bool* failed = nullptr);
long long          eval_llong  (@EXPR_TYPE@ expr, const long long&          def = 0,     bool disable_log = false, bool* failed = nullptr);
unsigned long long eval_ullong (@EXPR_TYPE@ expr, const unsigned long long& def = 0,     bool disable_log = false, bool* failed = nullptr);
float              eval_float  (@EXPR_TYPE@ expr, const float&              def = 0,     bool disable_log = false, bool* failed = nullptr);
double             eval_double (@EXPR_TYPE@ expr, const double&             def = 0,     bool disable_log = false, bool* failed = nullptr);
long double        eval_ldouble(@EXPR_TYPE@ expr, const long double&        def = 0,     bool disable_log = false, bool* failed = nullptr);
std::string        eval_string (@EXPR_TYPE@ expr, const std::string&        def = "",    bool disable_log = false, bool* failed = nullptr);
```

The expresion must have one return value, if more than one returned,
only the first one is used. 

If the evaluation fails, the default value `def` is returned.

#### 6.2 Evaluate to get complex type results. No default result supported.
API: 

```C++
template <typename T>
T eval(@EXPR_TYPE@ expr, bool disable_log = false, bool* failed = nullptr);
```
Result type T is like to that of method "get", but more.
T can be void, if T is void or std::tuple<>, it represents the expression 
do not have to provide return value.
If T is std::tuple, the expression can provide multiple return values.

Example:

```C++
peacalm::luaw l;
l.set("a", 10);
l.set("b", 5);
l.set("c", 2);

double ret = l.eval_double("return a^2 + b/c"); // 102.5
std::string s = l.eval_string("if a > b + c then return 'good' else return 'bad' end"); // "good"
auto si = l.eval<std::set<int>>("return {a, b, c}"); // {2,5,10}

auto t = l.eval<std::tuple<int, int, int>>("return a, b, c");  // multiple returns

l.eval<void>("print(a, b, c)"); // no returns
```

#### 6.3 Evaluate with custom variable provider (custom_luaw)

```C++
template <typename VariableProviderPointer>
class custom_luaw;
```

The class `custom_luaw` is derived from `luaw`, it can contain
a user defined variable provider.
When a global variable used in some expression does not 
exist in Lua, then it will seek the variable from the provider.

The template parameter `VariableProviderPointer` could 
be either a raw pointer type or a smart pointer type, i.e. std::shared_ptr or 
std::unique_ptr.
The underlying provider type should implement a member function:
* `bool provide(peacalm::luaw& l, const char* vname);`

In this member function, it should push exactly one value whose name is vname 
onto the stack then return true. Otherwise return false if vname is 
illegal or vname doesn't have a correct value.

Example:

```C++
struct provider {
  bool provide(peacalm::luaw& l, const char *vname) {
    if (strcmp(vname, "a") == 0) {
      l.push(1);
    } else if (strcmp(vname, "b") == 0) {
      l.push(2);
    } else if (strcmp(vname, "c") == 0) {
      l.push(3);
    } else {
      return false;
    }
    // If variables won't change, could set them to global,
    // which may improve performance:
    // l.copy_to_global(vname);
    return true;
  }
};

using provider_type = std::unique_ptr<provider>;
int main() {
  peacalm::custom_luaw<provider_type> l;
  l.provider(std::make_unique<provider>()); // Install provider
  double ret = l.eval_double("return a*10 + b^c");
  std::cout << ret << std::endl;  // 18
}
```


### 7. Low level operatioins: seek/to/touchtb/setkv/push

#### 7.1 The seek functions

**Notice**: Caller is responsible for popping the stack after calling the seek 
functions.

The seek functions push the global value or field of a table onto stack:
```C++
// Push the global environment onto the stack.
// Equivalent to gseek("_G") if "_G" is not modified.
luaw& gseek_env();

// Global Seek: Get a global value by name and push it onto the stack, or 
// push a nil if the name does not exist.
luaw& gseek(const char* name);
luaw& gseek(const std::string& name);

// Push t[name] onto the stack where t is the value at the given index `idx`,
// or push a nil if the operation fails.
luaw& seek(const char* name, int idx = -1);
luaw& seek(const std::string& name, int idx = -1);

// Push t[n] onto the stack where t is the value at the given index `idx`, or
// push a nil if the operation fails.
// Note that index of list in Lua starts from 1.
luaw& seek(int n, int idx = -1);

// Push t[p] onto the stack where t is the value at the given index `idx`,
// or push a nil if the operation fails.
self_t& seek(void* p, int idx = -1);

// Push the metatable of the value at the given index onto the stack if it
// has a metatable, otherwise push a nil.
self_t& seek(metatable_tag, int idx = -1);

// Long Seek: Call gseek() for the first parameter, then call seek() for the 
// rest parameters.
template <typename T, typename... Ts>
luaw& lseek(const T& t, const Ts&... ts);
```

#### 7.2 The type conversion functions
Type conversion functions convert a value in Lua stack to C++ type.

##### 7.2.1 To simple type
* @param [in] idx Index of Lua stack where the value in.
* @param [in] def The default value returned if conversion fails.
* @param [in] disable_log Whether print a log when exception occurs.
* @param [out] failed Will be set whether the convertion is failed if this 
pointer is not nullptr.
* @param [out] exists Will be set whether the value at given index exists if 
this pointer is not nullptr. Regard none and nil as not exists.
```C++
// To simple type
bool               to_bool   (int idx = -1, const bool&               def = false, bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
int                to_int    (int idx = -1, const int&                def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
unsigned int       to_uint   (int idx = -1, const unsigned int&       def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
long               to_long   (int idx = -1, const long&               def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
unsigned long      to_ulong  (int idx = -1, const unsigned long&      def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
long long          to_llong  (int idx = -1, const long long&          def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
unsigned long long to_ullong (int idx = -1, const unsigned long long& def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
float              to_float  (int idx = -1, const float&              def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
double             to_double (int idx = -1, const double&             def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
long double        to_ldouble(int idx = -1, const long double&        def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
std::string        to_string (int idx = -1, const std::string&        def = "",    bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
```

##### 7.2.2 To complex type
Conversion to complex C++ type.
Note that there are no default value parameters in this function:
```C++
// To complex type, without default value parameter
template <typename T> T to(int idx = -1, bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
```

Example:
```C++
peacalm::luaw l;
l.dostring("g={a=1, gg={a=11,ggg={a='s'}}, list={1,2,3}, m={{a=1},{a=2}}}");

int a = l.gseek("g").seek("a").to_int(); // g.a : 1
std::cout << l.gettop() << std::endl;    // 2

// g.a on top
l.to<int>(); // 1
l.to<std::string>(); // "1"

l.pop(); // Now g on top of stack
l.seek("gg").seek("a").to<int>(); // g.gg.a : 11

l.settop(0); // Clear stack

// Note that list index starts from 1 in Lua
l.gseek("g").seek("list").seek(3).to_int(); // g.list[3] : 3
// Start with gseek, ignore existing values in stack
l.gseek("g").seek("gg").seek("ggg").seek("a").to_string(); // g.gg.ggg.a : s
std::cout << l.gettop() << std::endl; // 7 (3 for first line, 4 for second)

l.pop(); // Now ggg on top of stack
l.to<std::unordered_map<std::string, std::string>>(); // g.gg.ggg : {"a":"s"}
l.settop(0);

// The followings are equivalent ways of writing:
l.gseek("g").seek("m").seek(2).seek("a").to_int();            // g.m[2].a : 2
l.gseek_env().seek("g").seek("m").seek(2).seek("a").to_int(); // g.m[2].a : 2
l.gseek("_G").seek("g").seek("m").seek(2).seek("a").to_int(); // g.m[2].a : 2
l.lseek("g", "m", 2, "a").to_int();                           // g.m[2].a : 2
l.lseek("_G", "g", "m", 2, "a").to_int();                     // g.m[2].a : 2

// Don't forget to clear the stack at last.
l.settop(0);
```


#### 7.3 touchtb: touch table

API:

```C++
/// Push the table (or value indexable and newindexable) with given name onto
/// stack. If not exists, create one.
self_t& gtouchtb(const char* name);

/// Push the table (or value indexable and newindexable) t[name] onto stack,
/// where t is a table at given index. If t[name] is not a table, create a new
/// one.
self_t& touchtb(const char* name, int idx = -1);


/// Push the table (or value indexable and newindexable) t[n] onto stack,
/// where t is a table at given index. If t[n] is not a table, create a new
/// one.
self_t& touchtb(int n, int idx = -1);

/// Push the table (or value indexable and newindexable) t[p] onto stack,
/// where t is a table at given index. If t[p] is not a table, create a new
/// one.
self_t& touchtb(void* p, int idx = -1);

/// Push the metatable of the value at the given index onto the stack.
/// If the value does not have a metatable, create a new metatable for it then
/// push the metatable onto stack.
/// The way to create new metatable: If m.tname is empty, create an empty
/// metatable, else create a new metatable using `luaL_newmetatable(L_,
/// m.tname)`.
self_t& touchtb(metatable_tag m, int idx = -1);

/// Long touchtb: Call gtouchtb() for the first parameter, then call touchtb()
/// for the rest parameters.
template <typename T, typename... Ts>
self_t& ltouchtb(const T& t, const Ts&... ts);
```

Ensure a subfield of a table is a table.
If the subfield doesn't exist or is not a table, make a new table then overwrite it.


#### 7.4 setkv

API:

```C++
/// Set t[key] = value, where t is a table at given index.
template <typename T>
void setkv(const char* key, T&& value, int idx = -1);
template <typename T>
void setkv(const std::string& key, T&& value, int idx = -1);
template <typename T>
void setkv(int key, T&& value, int idx = -1);
template <typename T>
void setkv(void* key, T&& value, int idx = -1);


/// Set field with an user given hint type.
template <typename Hint, typename T>
std::enable_if_t<!std::is_same<Hint, T>::value> setkv(const char* key,
                                                      T&&         value,
                                                      int         idx = -1);
template <typename Hint, typename T>
std::enable_if_t<!std::is_same<Hint, T>::value> setkv(const std::string& key,
                                                      T&& value,
                                                      int idx = -1);
template <typename Hint, typename T>
std::enable_if_t<!std::is_same<Hint, T>::value> setkv(int key,
                                                      T&& value,
                                                      int idx = -1);
template <typename Hint, typename T>
std::enable_if_t<!std::is_same<Hint, T>::value> setkv(void* key,
                                                      T&&   value,
                                                      int   idx = -1);
                                                    

/// Set the parameter value as metatable for the value at given index.
/// Setting nullptr as metatable means setting nil to metatable.
template <typename T>
void setkv(metatable_tag, T&& value, int idx = -1);
template <typename Hint, typename T>
std::enable_if_t<!std::is_same<Hint, T>::value> setkv(metatable_tag,
                                                      T&& value,
                                                      int idx = -1);
```

To set a pair of key-value member for the value at given index.

Using "push" API to push value, "Hint" is used for value in "push".

`luaw::metatable_tag` could be used as key to set a value's metatable.


#### 7.5 push

API:

```C++
/// Push a value onto stack.
template <typename T>
int push(T&& value);

/// Push with an user given hint type.
template <typename Hint, typename T>
std::enable_if_t<!std::is_same<Hint, T>::value, int> push(T&& value);
```

The const/volatile property of the value is also pushed into Lua.

Pushing `nullptr` means pushing `nil`.

`luaw::function_tag`, `luaw::class_tag`, `luaw::newtable_tag` can be used as hint type.


### 8. Execute Lua Scripts (File or String)

Just a simple wrapper of raw Lua API, nothing more added:

```C++
int loadstring(const char*        s);
int loadstring(const std::string& s);
int dostring(const char*        s);
int dostring(const std::string& s);
int loadfile(const char*        fname);
int loadfile(const std::string& fname);
int dofile(const char*        fname);
int dofile(const std::string& fname);
```

`dostring` is equivalent to `eval<void>` or `eval<std::tuple<>>`, 
the only difference is that the former will keep error info in stack, 
and the latter will print error info to stderr.

Example:

```C++
int main() {
  peacalm::luaw l;
  if (l.dofile("conf.lua") != LUA_OK) {
    l.log_error_in_stack();
    return 1;
  }
  // ...

  std::string expr = "a = 1 b = 2";
  if (l.dostring(expr) != LUA_OK) {
    l.log_error_in_stack();
    return 1;
  }
  // ...

  return 0;
}
```

### 9. Initialization options

On default, the `luaw` instance will load all standard Lua libs, but it is costly,
and not all libs are always needed.
So here is a initialization option class to guide how to initialize a luaw instance.

```C++
/// Initialization options for luaw.
class opt {
public:
  opt() {}

  /// Ignore all standard libs.
  opt& ignore_libs();
  /// Load all standard libs.
  opt& load_libs();
  /// Preload all standard libs.
  opt& preload_libs();

  /// Register extended functions.
  opt& register_exfunctions(bool r);

  /// Use already existed lua_State.
  opt& use_state(lua_State* L);

  /// Load user specified libs.
  opt& custom_load(const std::vector<luaL_Reg>& l);
  opt& custom_load(std::vector<luaL_Reg>&& l);

  /// Preload user specified libs.
  opt& custom_preload(const std::vector<luaL_Reg>& l);
  opt& custom_preload(std::vector<luaL_Reg>&& l);
}
```

What's more, this lib provides some useful extended functions.
The option `opt::register_exfunctions` indicates whether register these:

```C++
/// Short writing for if-elseif-else statement.
/// The number of arguments should be odd and at least 3.
/// Usage: IF(expr1, result_if_expr1_is_true,
///           expr2, result_if_expr2_is_true,
///           ...,
///           result_if_all_exprs_are_false)
/// Example: return IF(a > b, 'good', 'bad')
inline int IF(lua_State* L);

/// Convert multiple arguments or a list to a set, where key's mapped value is
/// boolean true.
inline int SET(lua_State* L);

/// Convert multiple arguments or a list to a dict, where key's mapped value is
/// the key's appearance count. Return nil if key not exists.
inline int COUNTER(lua_State* L);

/// Like COUNTER but return 0 if key not exists.
inline int COUNTER0(lua_State* L);
```


Example:

```C++
using namespace peacalm;

// Default initialization: load all standard libs, register extended functions
luaw l1;

// Preload libs, register extended functions
luaw l2(luaw::opt{}.preload_libs());

// Do not load libs, do not register extended functions
luaw l3(luaw::opt{}.ignore_libs().register_exfunctions(false));

// Load and preload some specific libs
luaw l4(luaw::opt()
            .ignore_libs()
            .custom_load({{LUA_GNAME, luaopen_base},
                          {LUA_LOADLIBNAME, luaopen_package},
                          {LUA_OSLIBNAME, luaopen_os}})
            .custom_preload({{LUA_MATHLIBNAME, luaopen_math},
                            {LUA_STRLIBNAME, luaopen_string}}));
```


## Build and Install
Install Lua first (here use lua-5.4.4 just as an example, of course can use later versions):
```bash
curl -R -O http://www.lua.org/ftp/lua-5.4.4.tar.gz
tar zxf lua-5.4.4.tar.gz
cd lua-5.4.4
make all test
sudo make install
```

Build then install:
```bash
git clone https://github.com/peacalm/cpp-luaw.git
cd cpp-luaw
mkdir build
cd build
cmake .. 
sudo make install
```

Test is developed using GoogleTest, if GoogleTest is installed, then can run 
test like:
```bash
cd cpp-luaw/build
cmake .. -DBUILD_TEST=TRUE
make
ctest
```
