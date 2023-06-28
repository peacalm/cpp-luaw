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
* Bind C++ functions(also lambda, std::function or callable objects) to Lua
* Bind C++ classes to Lua
* Evaluate Lua expressions in C++ to get values
* If a variable provider is provided, it can automatically seek variabls from 
provider while evaluate expressions.

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
// Define a luaw instance, which following examples use
peacalm::luaw l;
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Get simple type values (bool/integer/float number/string) </li></ul></ul> </td>
  <td> ✅ get_xxx </td>
  <td>

```C++
l.dostring("i = 1; b = true; f = 2.5; s = 'luastring';");
int i = l.get_int("i");
bool b = l.get_bool("b");
double f = l.get_double("f");
std::string s = l.get_string("s");

// Or use alternative writings:
int i = l.get<int>("i");
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
l.dostring("a = {1,2,3}; b = {x=1,y=2}; c = {x={1,2},y={3,4}}; d = {true, 1, 'str'}");
auto a = l.get<std::vector<int>>("a");
auto b = l.get<std::map<std::string, int>>("b");
auto c = l.get<std::unordered_map<std::string, std::vector<int>>>("c");
auto d = l.get<std::tuple<bool, int, std::string>>("d");
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Tells whether the target exists or whether the operation failed </li></ul></ul> </td>
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
  <td> <ul><ul><li> Support a default value on target not exists or operation failed </li></ul></ul> </td>
  <td> ✅ Only for simple types (get_xxx) </td>
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
l.set("c", std::make_pair("c", true));
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
  <td> <ul><li> Get and call Lua functions in C++ </li></ul> </td>
  <td> ✅ </td>
  <td>

```Lua
-- functions defined in Lua
fadd = function(a, b) return a + b end
frem = function(a, b) return a // b, a % b end
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Call a Lua function directly </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
// should provide result type
int s = l.callf<int>("fadd", 1, 2); // c = 1 + 2
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Get a std::function object represents the Lua function </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
auto f = l.get<std::function<int(int, int)>>("fadd");
int s = f(1,2);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Tells whether the call to Lua function failed (Get a peacalm::luaw::function object represents the Lua function) </li></ul></ul> </td>
  <td> ✅ use luaw::function </td>
  <td>

```C++
auto f = l.get<peacalm::luaw::function<int(int, int)>>("fadd");
int s = f(1,2);
// after call, check status:
if (f.failed()) {
  // error handlers
}
// see more details using:
// f.function_failed();
// f.function_exists();
// f.result_failed();
// f.result_exists();
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Get/Call a Lua function with multiple results </li></ul></ul> </td>
  <td> ✅ use std::tuple </td>
  <td>

```C++
// call it directly
auto q = l.callf<std::tuple<int, int>>("frem", 7, 3); // q == make_tuple(2, 1)

// get a function object first
auto f = l.get<peacalm::luaw::function<std::tuple<int,int>(int, int)>>("frem");
auto q2 = f(7, 3);
```
  </td>
</tr>


<tr>
  <td> <ul><li> Bind C++ functions(also lambda, std::function or callable objects) to Lua </li></ul> </td>
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
// a function like printf
int vf(const char* s, ...) { /* some codes */ }
// l.set("vf", vf); // error! not supported
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
  // explicitly specialize the function
  l.set("tadd", tadd<int>);
  // or provide the function proto type as hint
  l.set<double(double, double)>("tadd", tadd);
}
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
  <td> ✅ Should provide hint type </td>
  <td>

```C++
int x = 1;
auto f = [&](int a) { return a + x; };
// provide a function type hint: int(int)
l.set<int(int)>("add", f);
// Alternative writing: could use luaw::function_tag as hint type
l.set<peacalm::luaw::function_tag>("add", f);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><ul><li> Set generic lambdas to Lua </li></ul></ul></ul> </td>
  <td> ✅ Should provide hint type </td>
  <td>

```C++
// generic lambda
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
l.set("f", std::move(f)); // or l.set("f", f);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Support C++ function with multiple returns </li></ul></ul> </td>
  <td> ✅ use std::tuple </td>
  <td>

```C++
std::tuple<int, int> rem(int a, int b) { return std::make_tuple(a / b, a % b); }
l.set("rem", rem);
l.dostring("q, r = rem(7, 3)"); // q == 2, r == 1
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
  int       i  = 1;
  const int ci = 1;

  Obj() {}
  Obj(int v, int cv = 1) : i(v), ci(cv) {}
  
  int geti() const { return i; }
  int plus() { return ++i; }
  int plus(int d) { i += d; return i; }
};
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Register constructor </li></ul></ul> </td>
  <td> ✅ should provide hint type </td>
  <td>

```C++
l.register_ctor<Obj()>("NewObj");     // default constructor
l.register_ctor<Obj(int)>("NewObj1"); // constructor with 1 argument
l.register_ctor<Obj(int, int)>("NewObj2"); // constructor with 2 argument

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
// the constructors will generate a const instance of Obj in Lua
using ConstObj = const Obj;
l.register_ctor<ConstObj()>("NewConstObj");     // default constructor
l.register_ctor<ConstObj(int)>("NewConstObj1"); // constructor with 1 argument
l.register_ctor<ConstObj(int, int)>("NewConstObj2"); // constructor with 2 argument

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
l.register_member("ci", &Obj::ci);
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Register member function </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
l.register_member("geti", &Obj::geti);
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
  <td> <ul><ul><li> Register dynamic members (members whose name is dynamically defined in Lua, like keys of a table) </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
// if types of dynamic members are unknown, use luaw::luavalueref
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
// fake a const member "id" whose value is the object's address
l.register_member<void* const Obj::*>(
    "id", [](const volatile Obj* p) { return (void*)p; });

Obj o;
l.set("o", &o);
assert(l.eval<void*>("return o.id") == (void*)(&o));
```

```C++
int  gi = 100;  // global i, member to be faked
int& getgi(const volatile Obj* o) { return gi; }
int main() {
    luaw l;
    l.register_member<int Obj::*>("gi", getgi);
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
// fake a member function "seti" for class Obj
l.register_member<void (Obj ::*)(int)>(
  "seti", [](Obj* p, int v) { p->i = v; });
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
l.set("o", Obj{});
// The variable "o" can access all registered members of Obj
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Set class instance by raw pointer </li></ul></ul> </td>
  <td> ✅  </td>
  <td>

```C++
Obj o;
l.set("o", &o);
// The variable "o" can access all registered members of Obj
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
l.set("o", o);
// unique_ptr
l.set("o", std::make_unique<Obj>());
// The variable "o" can access all registered members of Obj
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Set class instance by pointer of smart pointer </li></ul></ul> </td>
  <td> ✅  </td>
  <td>

```C++
// pointer of shared_ptr
auto o = std::make_shared<Obj>();
l.set("o", &o);
// pointer of unique_ptr
auto o = std::make_unique<Obj>();
l.set("o", &o);
// The variable "o" can access all registered members of Obj
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Set class instance by unique_ptr with custom deleter (will share the same metatable with that unique_ptr with default deleter) </li></ul></ul> </td>
  <td> ✅  </td>
  <td>

```C++
struct ObjDeleter {
  void operator()(Obj* p) const { delete p; }
};
std::unique_ptr<Obj, ObjDeleter> o(new Obj, ObjDeleter{});
l.set("o", &o);
// or
l.set("o", std::move(o));
// The variable "o" can access all registered members of Obj
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

// const property of member ci is kept
int retcode = l.dostring("o = NewObj(); o.ci = 3");
assert(retcode != LUA_OK);
l.log_error_out(); // error log: Const member cannot be changed: ci

const Obj o{};
l.set("o", &o); // "o" is pointer of const Obj
l.eval<void>("o:plus()"); // call a nonconst member function
// error log: Nonconst member function: plus

l.eval<void>("o = NewConstObj(); o:plus()");
// error log: Nonconst member function: plus
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
  <td> <ul><ul><li> Return void </li></ul></ul> </td>
  <td> ✅ </td>
  <td>

```C++
// Equivalent to l.dostring, but this will print error log automatically
l.eval<void>("a=1 b=2");
```
  </td>
</tr>

<tr>
  <td> <ul><ul><li> Tells whether the eval operation failed </li></ul></ul> </td>
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
  <td> <ul><ul><li> Support a defult value used for operation failed </li></ul></ul> </td>
  <td> ✅ Only supported by eval simple types (eval_xxx) </td>
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

## Introduction

### 1. Get Global Variables From Lua

In the following API, `@NAME_TYPE@` could be `const char*` or `const std::string&`,
they are overloaded.

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
bool               get_bool   (@NAME_TYPE@ name, const bool&               def = false, bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
int                get_int    (@NAME_TYPE@ name, const int&                def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
unsigned int       get_uint   (@NAME_TYPE@ name, const unsigned int&       def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
long               get_long   (@NAME_TYPE@ name, const long&               def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
unsigned long      get_ulong  (@NAME_TYPE@ name, const unsigned long&      def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
long long          get_llong  (@NAME_TYPE@ name, const long long&          def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
unsigned long long get_ullong (@NAME_TYPE@ name, const unsigned long long& def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
float              get_float  (@NAME_TYPE@ name, const float&              def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
double             get_double (@NAME_TYPE@ name, const double&             def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
long double        get_ldouble(@NAME_TYPE@ name, const long double&        def = 0,     bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
std::string        get_string (@NAME_TYPE@ name, const std::string&        def = "",    bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);

// Caller is responsible for popping the stack after calling this API. You'd better use get_string unless you know the difference.
const char*        get_c_str (@NAME_TYPE@ name, const char*                def = "",    bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
```

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

#### 1.2 Get Global Variables with Complex Type

This version of API support get container types from Lua, 
and it doesn't support the parameter default value.
When getting a container type and the variable exists, the result will contain 
elements who are successfully converted, and discard who are not or who are nil.
Regard the operation failed if any element failed.

* @tparam T The result type user expected. T can be any type composited by 
bool, integer types, float number types, std::string, std::vector, std::set, 
std::unordered_set, std::map, std::unordered_map, std::pair. 
Note that here const char* is not supported, which is unsafe.
* @param [out] failed Will be set whether the operation is failed if this
pointer is not nullptr. If T is a container type, it regards the operation
as failed if any element failed.
* @return Return the value with given name in type T if conversion succeeds,
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
peacalm::luaw l;
l.dostring("a = 1 b = true v={1,2}, v2={1,2.5}, v3={1,nil,3}, m={p1={1,2},p2={3,4}}");
auto a = l.get<int>("a");          // 1
auto as = l.get<std::string>("a"); // "1"
auto b = l.get<bool>("b");         // true
auto bs = l.get<std::string>("b"); // "" (this conversion fails)

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
excpet for the last key, all other keys in path should be Lua table.

#### 2.1 Get Fields with Simple Type
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

#### 2.2 Get Fields with Complex Type
```C++
template <typename T> T get(@PATH_TYPE@ path, bool disable_log = false, bool* failed = nullptr, bool* exists = nullptr);
```

Example:
```C++
peacalm::luaw l;
l.dostring("a = 1 p={x=10,y=20} m={p1={1,2},p2={3,4}}");
l.get_int({"a"});      // 1
l.get_int({"ax"}, -1); // -1 (return user given default value)
l.get<int>({"a"});     // 1
l.get<int>({"ax"});    // 0  (return given type's initial value)

l.get<int>({"p", "x"});    // 10
l.get_int({"p", "z"}, 30); // 30 (return user given default value)
l.get<int>({"p", "z"});    // 0  (return given type's initial value)

l.get<std::vector<int>>({"m", "p2"}); // [3,4]
```

### 3. Seek Fields then Convert to C++ Type

#### 3.1 The Seek Functions
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

// Long Seek: Call gseek() for the first parameter, then call seek() for the 
// rest parameters.
template <typename T, typename... Ts>
luaw& lseek(const T& t, const Ts&... ts);
```

#### 3.2 The Type Conversion Functions
Type conversion functions convert a value in Lua stack to C++ type.

##### 3.2.1 To Simple Type
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

##### 3.2.2 To Complex Type
Conversion to complex C++ type.
Note that there are no default value parameters in this function:
* @sa [1.2 Get Global Variables with Complex Type](https://github.com/peacalm/cpp-luaw#12-get-global-variables-with-complex-type)
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


### 4. Set Global Variables to Lua

In the following API, `@NAME_TYPE@` could be `const char*` or `const std::string&`.

```C++
void set_integer(@NAME_TYPE@ name, long long value);
void set_number(@NAME_TYPE@ name, double value);
void set_boolean(@NAME_TYPE@ name, bool value);
void set_nil(@NAME_TYPE@ name);
void set_string(@NAME_TYPE@ name, const char* value);
void set_string(@NAME_TYPE@ name, const std::string& value);
```

### 5. Execute Lua Scripts (File or String)
Just a simple wrapper of raw Lua API, nothing more added.
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

### 6. Evaluate a Lua Expression and Get the Result

The expresion must have a return value, if more than one returned,
only the first one is used. Evaluate the expression then convert the returned 
value to expected C++ type.

In the following API, `@EXPR_TYPE@` could be `const char*` or `const std::string&`.

For simple type, which have default value parameter:
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

For complex type, which do not have default value parameter:
```C++
template <typename T> T eval(@EXPR_TYPE@ expr, bool disable_log = false, bool* failed = nullptr);
```

Example:
```C++
peacalm::luaw l;
l.set_integer("a", 10);
l.set_integer("b", 5);
l.set_integer("c", 2);
double ret = l.eval_double("return a^2 + b/c"); // 102.5
std::string s = l.eval_string("if a > b + c then return 'good' else return 'bad' end"); // "good"
auto si = l.eval<std::set<int>>("return {a, b, c}"); // {2,5,10}
```

### 7. Lua Wrapper with Custom Variable Provider

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
* `bool provide(peacalm::luaw* l, const char* vname);`

In this member function, it should push exactly one value whose name is vname 
onto the stack of L then return true. Otherwise return false if vname is 
illegal or vname doesn't have a correct value.

Example:
```C++
struct provider {
  bool provide(peacalm::luaw& l, const char *vname) {
    if (strcmp(vname, "a") == 0)
      l.push(1);
    else if (strcmp(vname, "b") == 0)
      l.push(2);
    else if (strcmp(vname, "c") == 0)
      l.push(3);
    else
      return false;
    // If variables won't change, could set them to global:
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
