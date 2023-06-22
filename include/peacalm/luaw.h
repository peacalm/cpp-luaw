// Copyright (c) 2023 Shuangquan Li. All Rights Reserved.
//
// Licensed under the MIT License (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License
// at
//
//   http://opensource.org/licenses/MIT
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.

#ifndef PEACALM_LUAW_H_
#define PEACALM_LUAW_H_

#include <array>
#include <cassert>
#include <cstring>
#include <deque>
#include <forward_list>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>
// This comment to avoid clang-format mix includes before sort
#include <lua.hpp>

static_assert(LUA_VERSION_NUM >= 504, "Lua version at least 5.4");

#ifdef PEACALM_LUAW_ASSERT_OFF
#define PEACALM_LUAW_ASSERT(x)
#else
#define PEACALM_LUAW_ASSERT(x) assert(x)
#endif

#ifdef PEACALM_LUAW_INDEXABLE_ASSERT_OFF
#define PEACALM_LUAW_INDEXABLE_ASSERT(x)
#else
#define PEACALM_LUAW_INDEXABLE_ASSERT(x) assert(x)
#endif

namespace peacalm {

namespace luaexf {  // Useful extended functions for Lua

// Short writing for if-elseif-else statement.
// The number of arguments should be odd and at least 3.
// Usage: IF(expr1, result_if_expr1_is_true,
//           expr2, result_if_expr2_is_true,
//           ...,
//           result_if_all_exprs_are_false)
// Example: return IF(a > b, 'good', 'bad')
inline int IF(lua_State* L) {
  int n = lua_gettop(L);
  if (n < 3 || (~n & 1)) {
    const char* s = n < 3 ? "IF: At least 3 arguments"
                          : "IF: The number of arguments should be odd";
    return luaL_error(L, s);
  }
  int ret = n;
  for (int i = 1; i < n; i += 2) {
    if (lua_toboolean(L, i)) {
      ret = i + 1;
      break;
    }
  }
  lua_pushvalue(L, ret);
  return 1;
}

// Convert multiple arguments or a list to a set, where key's value is boolean
// true.
inline int SET(lua_State* L) {
  int n = lua_gettop(L);
  if (n <= 0) {
    lua_newtable(L);
    return 1;
  }
  // list to SET
  if (n == 1 && lua_istable(L, 1)) {
    lua_newtable(L);
    int sz = luaL_len(L, 1);
    for (int i = 1; i <= sz; ++i) {
      lua_geti(L, 1, i);
      if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        continue;
      }
      lua_pushboolean(L, true);
      lua_settable(L, 2);
    }
    return 1;
  }
  // multi input arguments to SET
  lua_newtable(L);
  for (int i = 1; i <= n; ++i) {
    if (lua_isnil(L, i)) continue;
    lua_pushvalue(L, i);
    lua_pushboolean(L, true);
    lua_settable(L, -3);
  }
  return 1;
}

// Convert multiple arguments or a list to a dict, where key's value is the
// key's appearance count.
// Return nil if key not exists.
inline int COUNTER(lua_State* L) {
  int n = lua_gettop(L);
  if (n <= 0) {
    lua_newtable(L);
    return 1;
  }
  // list to COUNTER
  if (n == 1 && lua_istable(L, 1)) {
    lua_newtable(L);
    int sz = luaL_len(L, 1);
    for (int i = 1; i <= sz; ++i) {
      lua_geti(L, 1, i);
      if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        continue;
      }
      lua_pushvalue(L, -1);
      lua_gettable(L, 2);
      int cnt = lua_tointeger(L, -1);
      lua_pop(L, 1);
      lua_pushinteger(L, cnt + 1);
      lua_settable(L, 2);
    }
    return 1;
  }
  // multi input arguments to COUNTER
  lua_newtable(L);
  for (int i = 1; i <= n; ++i) {
    if (lua_isnil(L, i)) continue;
    lua_pushvalue(L, i);
    lua_pushvalue(L, i);
    lua_gettable(L, -3);
    int cnt = lua_tointeger(L, -1);
    lua_pop(L, 1);
    lua_pushinteger(L, cnt + 1);
    lua_settable(L, -3);
  }
  return 1;
}

static inline int COUNTER0__index(lua_State* L) {
  lua_pushinteger(L, 0);
  return 1;
}

// Like COUNTER but return 0 if key not exists.
inline int COUNTER0(lua_State* L) {
  COUNTER(L);
  if (luaL_newmetatable(L, "COUNTER0_mt")) {
    lua_pushcfunction(L, COUNTER0__index);
    lua_setfield(L, -2, "__index");
  }
  lua_setmetatable(L, -2);
  return 1;
}

}  // namespace luaexf

/// Basic Lua wrapper class.
class luaw {
  using self_t = luaw;

  // The path where registered member operations stored:
  // LUA_REGISTRYINDEX -> (void*)(&typeid(T)) -> this enum field
  enum member_info_fields {
    member_function = 1,
    member_getter,
    member_setter,
    generic_member_getter,
    generic_member_setter,
    const_member,
    nonconst_member_function,
    nonvolatile_member_function
  };

  template <typename T, typename = void>
  struct pusher;
  template <typename T, typename = void>
  struct convertor;
  template <typename T>
  struct register_ctor_impl;
  template <typename T, typename = void>
  struct registrar;

  // A callable object adapter, which bahaves like the result of std::mem_fn.
  // The origin first argument should be reference or raw pointer,
  // then this object can accept reference, raw pointer, and also smart pointers
  // (std::shared_ptr and std::unique_ptr) as the first argument to call the
  // origin callable object.
  template <typename T>
  struct mock_mem_fn;

  lua_State* L_;

public:
  using lua_cfunction_t = lua_CFunction;  // int (*)(lua_State *L)
  using lua_number_t    = lua_Number;     // double as default
  using lua_integer_t   = lua_Integer;    // long long as default

  // To generate shared/exclusive metatable of type T.
  template <typename T, typename = void>
  struct metatable_factory;

  // A callable wrapper for Lua functions. Like std::function and can convert to
  // std::function, but contains more status information.
  // Used within method get.
  template <typename T>
  class function;

  // Used as hint type for set/push/setkv, indicate the value is a user defined
  // custom class object.
  struct class_tag {};

  // Used as hint type for set/push/setkv, indicate the value is a function or a
  // callable object used as a function.
  struct function_tag {};

  // Used as a value for set/push, indicate that should set/push a new empty
  // table to Lua.
  struct newtable_tag {};

  // Used as a key for seek/touchtb/setkv/lset, indicate that we're
  // getting/setting a value's metatable.
  struct metatable_tag {
    const char* tname;
    metatable_tag(const char* name = nullptr) : tname(name) {}
  };

  // Indicate that convert a Lua value to nothing, the Lua value is useless.
  // Any Lua value can convert to this.
  // Maybe used as a C++ function formal parameter.
  struct placeholder_tag {};

  // Represent a Lua value in stack by index.
  struct luavalueidx {
    lua_State* L;
    int        idx;

    luavalueidx(lua_State* s = nullptr, int i = 0) : L(s), idx(i) {
      if (L) {
        int sz = lua_gettop(L);
        idx    = i < 0 && -i <= sz ? sz + i + 1 : i;
      }
    }
  };

  // A reference of some Lua value in LUA_REGISTRYINDEX.
  struct luavalueref {
    lua_State* L;
    int        ref_id;

    luavalueref(lua_State* s = nullptr) : L(s), ref_id(0) {
      if (L) {
        PEACALM_LUAW_ASSERT(lua_gettop(L) > 0);
        // pops a value on top and returns its ref_id.
        ref_id = luaL_ref(L, LUA_REGISTRYINDEX);
      }
    }

    luavalueref(const luavalueref&) = delete;

    luavalueref(luavalueref&& r) : L(r.L), ref_id(r.ref_id) { r.L = nullptr; }

    ~luavalueref() {
      if (L) {
        luaL_unref(L, LUA_REGISTRYINDEX, ref_id);
        L = nullptr;
      }
    }

    luavalueref& operator=(const luavalueref&) = delete;

    luavalueref& operator=(luavalueref&& r) {
      if (L != r.L || ref_id != r.ref_id) {
        this->~luavalueref();
        L      = r.L;
        ref_id = r.ref_id;
      }
      r.L = nullptr;
      return *this;
    }

    // Push the value referenced on top of stack.
    void getvalue() const { lua_rawgeti(L, LUA_REGISTRYINDEX, ref_id); }
  };

  // Stack balance guarder.
  // Automatically set stack to a specific size when destruct.
  class guarder {
    lua_State* L_;
    int        topsz_;

  public:
    guarder(lua_State* L, int topsz) : L_(L), topsz_(topsz) {}
    ~guarder() { lua_settop(L_, topsz_); }
  };

  guarder make_guarder() const { return guarder(L_, gettop()); }
  guarder make_guarder(int sz) const { return guarder(L_, sz); }
  guarder make_guarder(lua_State* L, int sz) const { return guarder(L, sz); }

  // Initialization options for luaw
  class opt {
    enum libopt : char { ignore = 0, load = 1, preload = 2 };

  public:
    opt() {}
    // Ignore all standard libs
    opt& ignore_libs() {
      libopt_ = ignore;
      return *this;
    }
    // Load all standard libs
    opt& load_libs() {
      libopt_ = load;
      return *this;
    }
    // Preload all standard libs
    opt& preload_libs() {
      libopt_ = preload;
      return *this;
    }

    // Register extended functions
    opt& register_exfunctions(bool r) {
      exfunc_ = r;
      return *this;
    }

    opt& use_state(lua_State* L) {
      L_ = L;
      return *this;
    }

    // Load user specified libs
    opt& custom_load(const std::vector<luaL_Reg>& l) {
      libs_load_ = l;
      return *this;
    }
    opt& custom_load(std::vector<luaL_Reg>&& l) {
      libs_load_ = std::move(l);
      return *this;
    }

    // Preload user specified libs
    opt& custom_preload(const std::vector<luaL_Reg>& l) {
      libs_preload_ = l;
      return *this;
    }
    opt& custom_preload(std::vector<luaL_Reg>&& l) {
      libs_preload_ = std::move(l);
      return *this;
    }

  private:
    libopt                libopt_ = load;
    bool                  exfunc_ = true;
    lua_State*            L_      = nullptr;
    std::vector<luaL_Reg> libs_load_;
    std::vector<luaL_Reg> libs_preload_;
    friend class luaw;
  };

  luaw(const opt& o = opt{}) { init(o); }
  luaw(lua_State* L) : L_(L) {}
  luaw(const luaw&) = delete;
  luaw(luaw&& l) : L_(l.release()) {}
  luaw& operator=(const luaw&) = delete;
  luaw& operator=(luaw&& r) {
    if (this == &r) {
      // do nothing
    } else if (L_ == r.L()) {
      r.L(nullptr);
    } else {
      close();
      L(r.release());
    }
    return *this;
  }
  ~luaw() { close(); }

  void init(const opt& o = opt{}) {
    if (o.L_) {
      L_ = o.L_;
    } else {
      L_ = luaL_newstate();
    }

    if (o.libopt_ == opt::libopt::load) {
      luaL_openlibs(L_);
    } else if (o.libopt_ == opt::libopt::preload) {
      preload_libs();
    }

    if (o.exfunc_) { register_exfunctions(); }

    if (!o.libs_load_.empty()) {
      for (const luaL_Reg& l : o.libs_load_) {
        luaL_requiref(L_, l.name, l.func, 1);
        pop();
      }
    }

    if (!o.libs_preload_.empty()) {
      // Should load base and package first for any preload.
      luaL_requiref(L_, LUA_GNAME, luaopen_base, 1);
      luaL_requiref(L_, LUA_LOADLIBNAME, luaopen_package, 1);
      getfield(-1, "preload");
      for (const luaL_Reg& l : o.libs_preload_) {
        pushcfunction(l.func);
        setfield(-2, l.name);
      }
      pop(3);
    }
  }

  void close() {
    if (L_) {
      lua_close(L_);
      L_ = nullptr;
    }
  }

  void reset(const opt& o = opt{}) {
    close();
    init(o);
  }

  void preload_libs() {
    // Should load base and package first for any preload.
    luaL_requiref(L_, LUA_GNAME, luaopen_base, 1);
    luaL_requiref(L_, LUA_LOADLIBNAME, luaopen_package, 1);
    getfield(-1, "preload");
    static const luaL_Reg preloadlibs[] = {{LUA_COLIBNAME, luaopen_coroutine},
                                           {LUA_TABLIBNAME, luaopen_table},
                                           {LUA_IOLIBNAME, luaopen_io},
                                           {LUA_OSLIBNAME, luaopen_os},
                                           {LUA_STRLIBNAME, luaopen_string},
                                           {LUA_MATHLIBNAME, luaopen_math},
                                           {LUA_UTF8LIBNAME, luaopen_utf8},
                                           {LUA_DBLIBNAME, luaopen_debug},
                                           {NULL, NULL}};
    for (const luaL_Reg* p = preloadlibs; p->func; ++p) {
      pushcfunction(p->func);
      setfield(-2, p->name);
    }
    pop(3);
  }

  /// Register a global function. Equivalent to `set(fname, f)`.
  void register_gf(const char* fname, lua_cfunction_t f) {
    PEACALM_LUAW_ASSERT(fname);
    lua_register(L_, fname, f);
  }

  /// Register extended functions.
  void register_exfunctions() {
    register_gf("IF", luaexf::IF);
    register_gf("SET", luaexf::SET);
    register_gf("COUNTER", luaexf::COUNTER);
    register_gf("COUNTER0", luaexf::COUNTER0);
  }

  /// Release the ownership of contained Lua State.
  /// The caller is responsible for closing the Lua State.
  lua_State* release() {
    lua_State* ret = L_;
    L_             = nullptr;
    return ret;
  }

  // Get and set lua_State.
  lua_State* L() const { return L_; }
  void       L(lua_State* L) { L_ = L; }

  /// Convert given index to absolute index of stack. It won't change idx's
  /// value if abs(idx) > topsize, this is different with lua_absindex.
  /// e.g. abs_index(LUA_REGISTRYINDEX) -> LUA_REGISTRYINDEX.
  int abs_index(int idx) {
    return idx < 0 && -idx <= gettop() ? gettop() + idx + 1 : idx;
  }

  // Stack operations
  void pop(int n = 1) { lua_pop(L_, n); }
  int  gettop() const { return lua_gettop(L_); }
  void settop(int idx) { lua_settop(L_, idx); }
  void cleartop() { settop(0); }

  // clang-format off
  int loadstring(const char*        s)   { return luaL_loadstring(L_, s); }
  int loadstring(const std::string& s)   { return loadstring(s.c_str()); }
  int dostring(const char*        s)     { return luaL_dostring(L_, s); }
  int dostring(const std::string& s)     { return dostring(s.c_str()); }
  int loadfile(const char*        fname) { return luaL_loadfile(L_, fname); }
  int loadfile(const std::string& fname) { return loadfile(fname.c_str()); }
  int dofile(const char*        fname)   { return luaL_dofile(L_, fname); }
  int dofile(const std::string& fname)   { return dofile(fname.c_str()); }
  // clang-format on

  // clang-format off
  bool isstring(int idx = -1)        const { return lua_isstring(L_, idx); }
  bool isnumber(int idx = -1)        const { return lua_isnumber(L_, idx); }
  bool isinteger(int idx = -1)       const { return lua_isinteger(L_, idx); }
  bool isboolean(int idx = -1)       const { return lua_isboolean(L_, idx); }
  bool isnil(int idx = -1)           const { return lua_isnil(L_, idx); }
  bool isnone(int idx = -1)          const { return lua_isnone(L_, idx); }
  bool isnoneornil(int idx = -1)     const { return lua_isnoneornil(L_, idx); }
  bool istable(int idx = -1)         const { return lua_istable(L_, idx); }
  bool iscfunction(int idx = -1)     const { return lua_iscfunction(L_, idx); }
  bool isfunction(int idx = -1)      const { return lua_isfunction(L_, idx); }
  bool isuserdata(int idx = -1)      const { return lua_isuserdata(L_, idx); }
  bool islightuserdata(int idx = -1) const { return lua_islightuserdata(L_, idx); }
  bool isthread(int idx = -1)        const { return lua_isthread(L_, idx); }
  // clang-format on

  // getxxx: return the type of the value pushed.
  int gettable(int idx) { return lua_gettable(L_, idx); }
  int geti(int idx, lua_integer_t n) { return lua_geti(L_, idx, n); }
  int getp(int idx, const void* p) {
    pushlightuserdata(p);
    return gettable(idx);
  }
  int getfield(int idx, const char* k) {
    PEACALM_LUAW_ASSERT(k);
    return lua_getfield(L_, idx, k);
  }

  void settable(int idx) { lua_settable(L_, idx); }
  void seti(int idx, lua_integer_t n) { lua_seti(L_, idx, n); }
  void setp(int idx, const void* p) {
    int aidx = abs(idx);
    pushlightuserdata(p);
    pushvalue(-2);
    settable(aidx);
    pop();
  }
  void setfield(int idx, const char* k) {
    PEACALM_LUAW_ASSERT(k);
    lua_setfield(L_, idx, k);
  }

  int rawget(int idx) { return lua_rawget(L_, idx); }
  int rawgeti(int idx, lua_integer_t n) { return lua_rawgeti(L_, idx, n); }
  int rawgetp(int idx, const void* p) { return lua_rawgetp(L_, idx, p); }
  int rawgetfield(int idx, const char* k) {
    pushstring(k);
    return rawget(idx);
  }

  void rawset(int idx) { lua_rawset(L_, idx); }
  void rawseti(int idx, lua_integer_t n) { lua_rawseti(L_, idx, n); }
  void rawsetp(int idx, const void* p) { lua_rawsetp(L_, idx, p); }
  void rawsetfield(int idx, const char* k) {
    int aidx = abs(idx);
    pushstring(k);
    pushvalue(-2);
    rawset(aidx);
    pop();
  }

  /// Return whether the value at idx has a metatable, if true push the
  /// metatable on top, otherwise push nothing and return false.
  bool getmetatable(int idx) { return lua_getmetatable(L_, idx); }

  /// Pop a table or nil and set it as metatable for value at idx.
  void setmetatable(int idx) { lua_setmetatable(L_, idx); }

  void       newtable() { lua_newtable(L_); }
  void*      newuserdata(size_t size) { return lua_newuserdata(L_, size); }
  lua_State* newthread() { return lua_newthread(L_); }

  /// Whether the value at idx is indexable.
  bool indexable(int idx = -1) const {
    if (istable(idx)) return true;
    if (lua_getmetatable(L_, idx) == 0) return false;
    auto _g = make_guarder(gettop() - 1);
    lua_getfield(L_, -1, "__index");
    // if __index exists, then regard as indexable
    bool ret = !isnoneornil(-1);
    return ret;
  }

  /// Whether the value at idx is newindexable.
  bool newindexable(int idx = -1) const {
    if (istable(idx)) return true;
    if (!lua_getmetatable(L_, idx)) return false;
    auto _g = make_guarder(gettop() - 1);
    lua_getfield(L_, -1, "__newindex");
    // if __newindex exists, then regard as newindexable
    bool ret = !isnoneornil(-1);
    return ret;
  }

  /// Whether the value at idx is indexable and newindexable.
  bool indexable_and_newindexable(int idx = -1) const {
    if (istable(idx)) return true;
    if (!lua_getmetatable(L_, idx)) return false;
    auto _g = make_guarder(gettop() - 1);
    lua_getfield(L_, -1, "__index");
    lua_getfield(L_, -2, "__newindex");
    // if both __index and __newindex exists
    bool ret = !isnoneornil(-1) && !isnoneornil(-2);
    return ret;
  }

  /// Whether the value at idx is callable.
  bool callable(int idx = -1) const {
    if (isfunction(idx)) return true;
    if (!lua_getmetatable(L_, idx)) return false;
    auto _g = make_guarder(gettop() - 1);
    lua_getfield(L_, -1, "__call");
    bool ret = !isnoneornil(-1);
    return ret;
  }

  /// Push the metatable in regristry with name `tname` onto stack, if not
  /// exists, create one. Return whether created a new metatable.
  bool gtouchmetatb(const char* tname) { return luaL_newmetatable(L_, tname); }

  /// Pushes onto the stack the global value with given name.
  /// Returns the type of that value.
  int getglobal(const char* name) {
    PEACALM_LUAW_ASSERT(name);
    return lua_getglobal(L_, name);
  }

  /// Pops a value from the stack and sets it as the new value of global name.
  void setglobal(const char* name) {
    PEACALM_LUAW_ASSERT(name);
    lua_setglobal(L_, name);
  }

  int pcall(int n, int r, int f) { return lua_pcall(L_, n, r, f); }

  int         type(int idx) const { return lua_type(L_, idx); }
  const char* type_name(int idx) const { return lua_typename(L_, type(idx)); }

  ///////////////////////// type conversions ///////////////////////////////////

  // NOTICE: Value conversions from Lua to C++ may be different with that in
  // Lua!
  //
  // Conversion process: Firstly convert value in Lua to C++'s value with
  // corresponding type, e.g. boolean to bool, integer to long long, number to
  // double, string to string, then cast it to target type by C++'s type
  // conversion strategy. Note that number in Lua is also string, and
  // number-literal-string is also number.
  //
  // This lib mainly uses C++'s value conversion strategy, in addition, a
  // implicitly conversion strategy between number and number-literal-string,
  // which is supported by Lua.
  //
  // In total, this lib makes value conversions behave more like C++.
  //
  // Details:
  // 1. Implicitly conversion between integer, number, boolean using
  //    C++'s static_cast
  // 2. Implicitly conversion between number and number-literal-string by Lua
  // 3. When convert number 0 to boolean, will get false (not true as Lua does)
  // 4. NONE and NIL won't convert to any value, default value (user given or
  //    initial value of target type) will be returned
  // 5. Non-number-literal-string, including empty string, can't convert to any
  //    other types, default value will be returned (can't convert to true as
  //    Lua does)
  // 6. Integer's precision won't lost if it's value is representable by 64bit
  //    signed integer type, i.e. between [-2^63, 2^63 -1], which is
  //    [-9223372036854775808, 9223372036854775807]
  //
  // Examples:
  // * number 2.5 -> string "2.5" (By Lua)
  // * number 3 -> string "3.0" (By Lua)
  // * ingeger 3 -> string "3" (By Lua)
  // * string "2.5" -> double 2.5 (By Lua)
  // * number 2.5 -> int 2 (By C++)
  // * string "2.5" -> int 2 (Firstly "2.5"->2.5 by lua, then 2.5->2 by C++)
  // * boolean true -> int 1 (By C++)
  // * boolean false -> int 0 (By C++)
  // * integer 0 -> bool false (By C++)
  // * number 2.5 -> bool true (By C++)
  // * string "2.5" -> bool true ("2.5"->2.5 by Lua, then 2.5->true by C++)
  // * string "0" -> bool false ("0"->0 by Lua, then 0->false by C++)

  /**
   * @brief Convert a value in Lua stack to C++ simple type value.
   *
   * @param [in] idx Index of Lua stack where the value in.
   * @param [in] def The default value returned if conversion fails.
   * @param [in] disable_log Whether print a log when exception occurs.
   * @param [out] failed Will be set whether the convertion is failed if this
   * pointer is not nullptr.
   * @param [out] exists Will be set whether the value at given index exists if
   * this pointer is not nullptr. Regard none and nil as not exists.
   *
   * @{
   */

#define DEFINE_TYPE_CONVERSION(typename, type, default)        \
  type to_##typename(int   idx         = -1,                   \
                     type  def         = default,              \
                     bool  disable_log = false,                \
                     bool* failed      = nullptr,              \
                     bool* exists      = nullptr) {                 \
    if (exists) *exists = !isnoneornil(idx);                   \
    /* check integer before number to avoid precision lost. */ \
    if (isinteger(idx)) {                                      \
      if (failed) *failed = false;                             \
      return static_cast<type>(lua_tointeger(L_, idx));        \
    }                                                          \
    if (isnumber(idx)) {                                       \
      if (failed) *failed = false;                             \
      /* try integer first to avoid precision lost */          \
      long long t = lua_tointeger(L_, idx);                    \
      if (t != 0) return t;                                    \
      return static_cast<type>(lua_tonumber(L_, idx));         \
    }                                                          \
    if (isboolean(idx)) {                                      \
      if (failed) *failed = false;                             \
      return static_cast<type>(lua_toboolean(L_, idx));        \
    }                                                          \
    if (isnoneornil(idx)) {                                    \
      if (failed) *failed = false;                             \
      return def;                                              \
    }                                                          \
    if (failed) *failed = true;                                \
    if (!disable_log) log_type_convert_error(idx, #type);      \
    return def;                                                \
  }

  DEFINE_TYPE_CONVERSION(bool, bool, false)
  DEFINE_TYPE_CONVERSION(int, int, 0)
  DEFINE_TYPE_CONVERSION(uint, unsigned int, 0)
  DEFINE_TYPE_CONVERSION(long, long, 0)
  DEFINE_TYPE_CONVERSION(ulong, unsigned long, 0)
  DEFINE_TYPE_CONVERSION(llong, long long, 0)
  DEFINE_TYPE_CONVERSION(ullong, unsigned long long, 0)
  DEFINE_TYPE_CONVERSION(float, float, 0)
  DEFINE_TYPE_CONVERSION(double, double, 0)
  DEFINE_TYPE_CONVERSION(ldouble, double, 0)
#undef DEFINE_TYPE_CONVERSION

  // NOTICE: Lua will implicitly convert number to string
  // boolean can't convert to string
  const char* to_c_str(int         idx         = -1,
                       const char* def         = "",
                       bool        disable_log = false,
                       bool*       failed      = nullptr,
                       bool*       exists      = nullptr) {
    if (exists) *exists = !isnoneornil(idx);
    if (isstring(idx)) {  // include number
      if (failed) *failed = false;
      return lua_tostring(L_, idx);
    }
    if (isnoneornil(idx)) {
      if (failed) *failed = false;
      return def;
    }
    if (failed) *failed = true;
    if (!disable_log) log_type_convert_error(idx, "string");
    return def;
  }

  std::string to_string(int                idx         = -1,
                        const std::string& def         = "",
                        bool               disable_log = false,
                        bool*              failed      = nullptr,
                        bool*              exists      = nullptr) {
    return std::string{to_c_str(idx, def.c_str(), disable_log, failed, exists)};
  }

  /** @} */

  /**
   * @brief Convert a value in Lua stack to complex C++ type.
   *
   * @tparam T The result type user expected. T can be any type composited by
   * bool, integer types, float number types, std::string, std::vector,
   * std::set, std::unordered_set, std::map, std::unordered_map, std::pair.
   * Note that c_str "const char*" is not supported.
   * @param [in] idx value's index in stack.
   * @param [in] disable_log Whether print a log when exception occurs.
   * @param [out] failed Will be set whether the operation is failed if this
   * pointer is not nullptr. If T is a container type, it regards the operation
   * as failed if any element failed.
   * @param [out] exists Will be set whether the value at given index exists if
   * this pointer is not nullptr. Regard none and nil as not exists.
   * @return Return the value on given index in type T if conversion succeeds,
   * otherwise return initial value of T(i.e. by statement `T{}`) if T is a
   * simple type, e.g. bool, int, double, std::string, etc. If T is a container
   * type, the result will contain all non-nil elements whose conversion
   * succeeded and discard elements who are nil or elements whose conversion
   * failed.
   */
  template <typename T>
  T to(int   idx         = -1,
       bool  disable_log = false,
       bool* failed      = nullptr,
       bool* exists      = nullptr) {
    static_assert(!std::is_reference<T>::value, "Should not return reference");
    return convertor<std::decay_t<T>>::to(
        *this, idx, disable_log, failed, exists);
  }

  ///////////////////////// seek fields ////////////////////////////////////////

  /// Push the global environment onto the stack.
  /// Equivalent to gseek("_G") if "_G" is not modified.
  self_t& gseek_env() {
    pushglobaltable();
    return *this;
  }

  /// Global Seek: Get a global value by name and push it onto the stack, or
  /// push a nil if the name does not exist.
  self_t& gseek(const char* name) {
    if (name) {
      getglobal(name);
    } else {
      pushnil();
    }
    return *this;
  }
  self_t& gseek(const std::string& name) { return gseek(name.c_str()); }

  /// Push t[name] onto the stack where t is the value at the given index `idx`,
  /// or push a nil if the operation fails.
  self_t& seek(const char* name, int idx = -1) {
    if (name && (istable(idx) || indexable(idx))) {
      getfield(idx, name);
    } else {
      pushnil();
    }
    return *this;
  }
  self_t& seek(const std::string& name, int idx = -1) {
    return seek(name.c_str(), idx);
  }

  /// Push t[n] onto the stack where t is the value at the given index `idx`, or
  /// push a nil if the operation fails. Note that index of list in Lua starts
  /// from 1.
  self_t& seek(int n, int idx = -1) {
    if (istable(idx) || indexable(idx)) {
      lua_geti(L_, idx, n);
    } else {
      pushnil();
    }
    return *this;
  }

  /// Push t[p] onto the stack where t is the value at the given index `idx`,
  /// or push a nil if the operation fails.
  self_t& seek(void* p, int idx = -1) {
    // p could be 0
    if (istable(idx) || indexable(idx)) {
      int aidx = abs_index(idx);
      pushlightuserdata(p);
      gettable(aidx);
    } else {
      pushnil();
    }
    return *this;
  }

  /// Seeking by nullptr is meanless.
  self_t& seek(std::nullptr_t, int idx = -1) = delete;

  /// Push the metatable of the value at the given index onto the stack if it
  /// has a metatable, otherwise push a nil.
  self_t& seek(metatable_tag, int idx = -1) {
    if (!getmetatable(idx)) { pushnil(); }
    return *this;
  }

  /// Long Seek: Call gseek() for the first parameter, then call seek() for the
  /// rest parameters.
  template <typename T, typename... Ts>
  self_t& lseek(const T& t, const Ts&... ts) {
    gseek(t);
    __lseek(ts...);
    return *this;
  }

private:
  void __lseek() {}

  template <typename T, typename... Ts>
  void __lseek(const T& t, const Ts&... ts) {
    seek(t);
    __lseek(ts...);
  }

public:
  ///////////////////////// push variables ///////////////////////////////

  /// Push a value onto stack.
  template <typename T>
  int push(T&& value) {
    return pusher<std::decay_t<T>>::push(*this, std::forward<T>(value));
  }

  /// Push with an user given hint type.
  template <typename Hint, typename T>
  std::enable_if_t<!std::is_same<Hint, T>::value, int> push(T&& value) {
    return pusher<std::decay_t<Hint>>::push(*this, std::forward<T>(value));
  }

  /// Copy and push a value already in the stack.
  void pushvalue(int idx = -1) { lua_pushvalue(L_, idx); }

  /// Copy a value in stack to a global value with given name.
  void copy_to_global(const char* name, int idx = -1) {
    pushvalue(idx);
    setglobal(name);
  }

  /// Pushes the global environment onto the stack.
  void pushglobaltable() { lua_pushglobaltable(L_); }

  /// Pushes the thread represented by L onto the stack.
  /// Returns 1 if this thread is the main thread of its state.
  int pushthread() { return lua_pushthread(L_); }

  /// Push a C closure function with n upvalues.
  void pushcclosure(lua_cfunction_t f, int n) { lua_pushcclosure(L_, f, n); }

  /// Push lightuserdata. Equivalent to `push(f)`.
  void pushcfunction(lua_cfunction_t f) { lua_pushcfunction(L_, f); }

  /// Push lightuserdata. Equivalent to `push(p)`.
  void pushlightuserdata(const void* p) { lua_pushlightuserdata(L_, (void*)p); }

  /// Push nil. Equivalent to `push(nullptr)`.
  void pushnil() { lua_pushnil(L_); }

  /// Return a pointer to the internal copy of the string.
  const char* pushstring(const char* s) {
    PEACALM_LUAW_ASSERT(s);  // we forbit s to be NULL
    return lua_pushstring(L_, s);
  }

  ///////////////////////// touch table ////////////////////////////////////////

  /// Push the table (or value indexable and newindexable) with given name onto
  /// stack. If not exists, create one.
  self_t& gtouchtb(const char* name) {
    getglobal(name);
    if (istable() || indexable_and_newindexable()) return *this;
    pop();
    newtable();
    pushvalue(-1);  // make a copy
    setglobal(name);
    return *this;
  }
  self_t& gtouchtb(const std::string& name) { return gtouchtb(name.c_str()); }

  /// Push the table (or value indexable and newindexable) t[name] onto stack,
  /// where t is a table at given index. If t[name] is not a table, create a new
  /// one.
  self_t& touchtb(const char* name, int idx = -1) {
    PEACALM_LUAW_ASSERT(name);
    int aidx = abs_index(idx);
    PEACALM_LUAW_INDEXABLE_ASSERT(indexable_and_newindexable(aidx));
    getfield(aidx, name);
    if (istable() || indexable_and_newindexable()) return *this;
    pop();
    newtable();
    setfield(aidx, name);
    getfield(aidx, name);
    return *this;
  }
  self_t& touchtb(const std::string& name, int idx = -1) {
    return touchtb(name.c_str(), idx);
  }

  /// Push the table (or value indexable and newindexable) t[n] onto stack,
  /// where t is a table at given index. If t[n] is not a table, create a new
  /// one.
  self_t& touchtb(int n, int idx = -1) {
    int aidx = abs_index(idx);
    PEACALM_LUAW_INDEXABLE_ASSERT(indexable_and_newindexable(aidx));
    geti(aidx, n);
    if (istable() || indexable_and_newindexable()) return *this;
    pop();
    newtable();
    seti(aidx, n);
    geti(aidx, n);
    return *this;
  }

  /// Push the table (or value indexable and newindexable) t[p] onto stack,
  /// where t is a table at given index. If t[p] is not a table, create a new
  /// one.
  self_t& touchtb(void* p, int idx = -1) {
    int aidx = abs_index(idx);
    PEACALM_LUAW_INDEXABLE_ASSERT(indexable_and_newindexable(aidx));
    pushlightuserdata(p);
    gettable(aidx);
    if (istable() || indexable_and_newindexable()) return *this;
    pop();
    newtable();
    pushlightuserdata(p);
    pushvalue(-2);
    settable(aidx);
    return *this;
  }

  /// nullptr represents nil, using nil as key is invalid.
  self_t& touchtb(std::nullptr_t, int idx = -1) = delete;

  /// Push the metatable of the value at the given index onto the stack.
  /// If the value does not have a metatable, create a new metatable for it then
  /// push the metatable onto stack.
  /// The way to create new metatable: If m.tname is empty, create an empty
  /// metatable, else create a new metatable using `luaL_newmetatable(L_,
  /// m.tname)`.
  self_t& touchtb(metatable_tag m, int idx = -1) {
    if (!getmetatable(idx)) {
      int aidx = abs_index(idx);
      if (!m.tname) {
        newtable();
      } else {
        luaL_newmetatable(L_, m.tname);
      }
      setmetatable(aidx);
      bool t = getmetatable(aidx);
      PEACALM_LUAW_ASSERT(t);
    }
    return *this;
  }

  /// Long touchtb: Call gtouchtb() for the first parameter, then call touchtb()
  /// for the rest parameters.
  template <typename T, typename... Ts>
  self_t& ltouchtb(const T& t, const Ts&... ts) {
    gtouchtb(t);
    __ltouchtb(ts...);
    return *this;
  }

private:
  void __ltouchtb() {}

  template <typename T, typename... Ts>
  void __ltouchtb(const T& t, const Ts&... ts) {
    touchtb(t);
    __ltouchtb(ts...);
  }

public:
  ///////////////////////// set fields into a table ////////////////////////////

  /// Set t[key] = value, where t is a table at given index.
  template <typename T>
  void setkv(const char* key, T&& value, int idx = -1) {
    PEACALM_LUAW_ASSERT(key);
    PEACALM_LUAW_INDEXABLE_ASSERT(newindexable(idx));
    int aidx = abs_index(idx);
    push(std::forward<T>(value));
    setfield(aidx, key);
  }
  template <typename T>
  void setkv(const std::string& key, T&& value, int idx = -1) {
    setkv(key.c_str(), std::forward<T>(value), idx);
  }
  template <typename T>
  void setkv(int key, T&& value, int idx = -1) {
    PEACALM_LUAW_INDEXABLE_ASSERT(newindexable(idx));
    int aidx = abs_index(idx);
    push(std::forward<T>(value));
    seti(aidx, key);
  }
  template <typename T>
  void setkv(void* key, T&& value, int idx = -1) {
    PEACALM_LUAW_INDEXABLE_ASSERT(newindexable(idx));
    int aidx = abs_index(idx);
    pushlightuserdata(key);
    push(std::forward<T>(value));
    settable(aidx);
  }

  /// Set field with an user given hint type.
  template <typename Hint, typename T>
  std::enable_if_t<!std::is_same<Hint, T>::value> setkv(const char* key,
                                                        T&&         value,
                                                        int         idx = -1) {
    PEACALM_LUAW_ASSERT(key);
    PEACALM_LUAW_INDEXABLE_ASSERT(newindexable(idx));
    int aidx = abs_index(idx);
    push<Hint>(std::forward<T>(value));
    setfield(aidx, key);
  }
  template <typename Hint, typename T>
  std::enable_if_t<!std::is_same<Hint, T>::value> setkv(const std::string& key,
                                                        T&& value,
                                                        int idx = -1) {
    setkv<Hint>(key.c_str(), std::forward<T>(value), idx);
  }
  template <typename Hint, typename T>
  std::enable_if_t<!std::is_same<Hint, T>::value> setkv(int key,
                                                        T&& value,
                                                        int idx = -1) {
    PEACALM_LUAW_INDEXABLE_ASSERT(newindexable(idx));
    int aidx = abs_index(idx);
    push<Hint>(std::forward<T>(value));
    seti(aidx, key);
  }
  template <typename Hint, typename T>
  std::enable_if_t<!std::is_same<Hint, T>::value> setkv(void* key,
                                                        T&&   value,
                                                        int   idx = -1) {
    PEACALM_LUAW_INDEXABLE_ASSERT(newindexable(idx));
    int aidx = abs_index(idx);
    pushlightuserdata(key);
    push<Hint>(std::forward<T>(value));
    settable(aidx);
  }

  /// Set the parameter value as metatable for the value at given index.
  /// Setting nullptr as metatable means setting nil to metatable.
  template <typename T>
  void setkv(metatable_tag, T&& value, int idx = -1) {
    int aidx = abs_index(idx);
    push(std::forward<T>(value));
    setmetatable(aidx);
  }
  template <typename Hint, typename T>
  std::enable_if_t<!std::is_same<Hint, T>::value> setkv(metatable_tag,
                                                        T&& value,
                                                        int idx = -1) {
    int aidx = abs_index(idx);
    push<Hint>(std::forward<T>(value));
    setmetatable(aidx);
  }

  /// Using nullptr as key is invalid.
  template <typename T>
  void setkv(std::nullptr_t, T&& value, int idx = -1) = delete;
  template <typename Hint, typename T>
  void setkv(std::nullptr_t, T&& value, int idx = -1) = delete;

  ///////////////////////// set global variables ///////////////////////////////

  /**
   * @brief Set a global variable to Lua.
   *
   * set(name, nullptr) means set nil to 'name'.
   */
  template <typename T>
  void set(const char* name, T&& value) {
    push(std::forward<T>(value));
    setglobal(name);
  }
  template <typename T>
  void set(const std::string& name, T&& value) {
    set(name.c_str(), std::forward<T>(value));
  }

  /// Set a global variable with an user given hint type.
  template <typename Hint, typename T>
  std::enable_if_t<!std::is_same<Hint, T>::value> set(const char* name,
                                                      T&&         value) {
    push<Hint>(std::forward<T>(value));
    setglobal(name);
  }
  template <typename Hint, typename T>
  std::enable_if_t<!std::is_same<Hint, T>::value> set(const std::string& name,
                                                      T&& value) {
    set<Hint>(name.c_str(), std::forward<T>(value));
  }

  /// Recursively set fields. The last element in path is the key for value,
  /// other elements in path are nested tables.
  template <typename T>
  void set(const std::initializer_list<const char*>& path, T&& value) {
    __set<T>(path.begin(), path.end(), std::forward<T>(value));
  }
  template <typename T>
  void set(const std::initializer_list<std::string>& path, T&& value) {
    __set<T>(path.begin(), path.end(), std::forward<T>(value));
  }
  template <typename T>
  void set(const std::vector<const char*>& path, T&& value) {
    __set<T>(path.begin(), path.end(), std::forward<T>(value));
  }
  template <typename T>
  void set(const std::vector<std::string>& path, T&& value) {
    __set<T>(path.begin(), path.end(), std::forward<T>(value));
  }

  /// Recursively set fields with hint type
  template <typename Hint, typename T>
  std::enable_if_t<!std::is_same<Hint, T>::value> set(
      const std::initializer_list<const char*>& path, T&& value) {
    __set<Hint>(path.begin(), path.end(), std::forward<T>(value));
  }
  template <typename Hint, typename T>
  std::enable_if_t<!std::is_same<Hint, T>::value> set(
      const std::initializer_list<std::string>& path, T&& value) {
    __set<Hint>(path.begin(), path.end(), std::forward<T>(value));
  }
  template <typename Hint, typename T>
  std::enable_if_t<!std::is_same<Hint, T>::value> set(
      const std::vector<const char*>& path, T&& value) {
    __set<Hint>(path.begin(), path.end(), std::forward<T>(value));
  }
  template <typename Hint, typename T>
  std::enable_if_t<!std::is_same<Hint, T>::value> set(
      const std::vector<std::string>& path, T&& value) {
    __set<Hint>(path.begin(), path.end(), std::forward<T>(value));
  }

  /// Long set. The last argument is value, the rest arguments are indices and
  /// sub-indices, where could contain metatable_tag.
  template <typename... Args>
  void lset(Args&&... args) {
    constexpr size_t N = sizeof...(Args);
    static_assert(N >= 2, "lset needs at least two arguments");
    auto _g = make_guarder();
    gseek_env();
    using T = std::tuple_element_t<N - 1, std::tuple<Args...>>;
    __lset<T>(std::forward<Args>(args)...);
  }
  /// Long set with a hint type.
  template <typename Hint, typename... Args>
  void lset(Args&&... args) {
    static_assert(sizeof...(Args) >= 2, "lset needs at least two arguments");
    auto _g = make_guarder();
    gseek_env();
    __lset<Hint>(std::forward<Args>(args)...);
  }

private:
  template <typename Hint, typename First, typename Second>
  void __lset(First&& f, Second&& s) {
    setkv<Hint>(std::forward<First>(f), std::forward<Second>(s));
  }

  template <typename Hint, typename First, typename Second, typename... Rests>
  void __lset(First&& f, Second&& s, Rests&&... rs) {
    touchtb(std::forward<First>(f));
    __lset<Hint>(std::forward<Second>(s), std::forward<Rests>(rs)...);
  }

  template <typename Hint, typename Iterator, typename T>
  void __set(Iterator b, Iterator e, T&& value) {
    if (b == e) return;
    auto _g = make_guarder();
    gseek_env();

    auto it = b;
    while (true) {
      auto nx = std::next(it, 1);
      if (nx == e) {
        setkv<Hint>(*it, std::forward<T>(value));
        break;
      } else {
        touchtb(*it);
        it = nx;
      }
    }
  }

public:
  // set for simple types

  void set_integer(const char* name, long long value) {
    lua_pushinteger(L_, value);
    setglobal(name);
  }
  void set_integer(const std::string& name, long long value) {
    set_integer(name.c_str(), value);
  }

  void set_number(const char* name, double value) {
    lua_pushnumber(L_, value);
    setglobal(name);
  }
  void set_number(const std::string& name, double value) {
    set_number(name.c_str(), value);
  }

  void set_boolean(const char* name, bool value) {
    lua_pushboolean(L_, static_cast<int>(value));
    setglobal(name);
  }
  void set_boolean(const std::string& name, bool value) {
    set_boolean(name.c_str(), value);
  }

  void set_nil(const char* name) {
    pushnil();
    setglobal(name);
  }
  void set_nil(const std::string& name) { set_nil(name.c_str()); }

  void set_string(const char* name, const char* value) {
    lua_pushstring(L_, value);
    setglobal(name);
  }
  void set_string(const std::string& name, const char* value) {
    set_string(name.c_str(), value);
  }

  void set_string(const char* name, const std::string& value) {
    set_string(name, value.c_str());
  }
  void set_string(const std::string& name, const std::string& value) {
    set_string(name.c_str(), value.c_str());
  }

  ///////////////////////// get global variables ///////////////////////////////

  /**
   * @brief Get a global variable in Lua and convert it to simple C++ type.
   *
   * @param [in] name The variable's name.
   * @param [in] def The default value returned if failed or target does not
   * exist.
   * @param [in] disable_log Whether print a log when exception occurs.
   * @param [out] failed Will be set whether the operation is failed if this
   * pointer is not nullptr.
   * @param [out] exists Set whether the variable exists. Regard none and nil as
   * not exists.
   * @return Return the variable's value if the variable exists and conversion
   * succeeded.
   *
   * @{
   */

#define DEFINE_GLOBAL_GET(typename, type, default)                         \
  type get_##typename(const char* name,                                    \
                      const type& def         = default,                   \
                      bool        disable_log = false,                     \
                      bool*       failed      = nullptr,                   \
                      bool*       exists      = nullptr) {                            \
    getglobal(name);                                                       \
    type ret = to_##typename(-1, def, disable_log, failed, exists);        \
    pop();                                                                 \
    return ret;                                                            \
  }                                                                        \
  type get_##typename(const std::string& name,                             \
                      const type&        def         = default,            \
                      bool               disable_log = false,              \
                      bool*              failed      = nullptr,            \
                      bool*              exists      = nullptr) {                            \
    return get_##typename(name.c_str(), def, disable_log, failed, exists); \
  }

  DEFINE_GLOBAL_GET(bool, bool, false)
  DEFINE_GLOBAL_GET(int, int, 0)
  DEFINE_GLOBAL_GET(uint, unsigned int, 0)
  DEFINE_GLOBAL_GET(long, long, 0)
  DEFINE_GLOBAL_GET(ulong, unsigned long, 0)
  DEFINE_GLOBAL_GET(llong, long long, 0)
  DEFINE_GLOBAL_GET(ullong, unsigned long long, 0)
  DEFINE_GLOBAL_GET(float, float, 0)
  DEFINE_GLOBAL_GET(double, double, 0)
  DEFINE_GLOBAL_GET(ldouble, long double, 0)
  DEFINE_GLOBAL_GET(string, std::string, "")
#undef DEFINE_GLOBAL_GET

  // NO POP! Because the c_str body is in stack.
  // Caller is responsible for popping stack.
  // You'd better use get_string unless you know the difference.
  const char* get_c_str(const char* name,
                        const char* def         = "",
                        bool        disable_log = false,
                        bool*       failed      = nullptr,
                        bool*       exists      = nullptr) {
    getglobal(name);
    return to_c_str(-1, def, disable_log, failed, exists);
  }
  const char* get_c_str(const std::string& name,
                        const char*        def         = "",
                        bool               disable_log = false,
                        bool*              failed      = nullptr,
                        bool*              exists      = nullptr) {
    return get_c_str(name.c_str(), def, disable_log, failed, exists);
  }

  /** @} */

  /**
   * @brief Get a global variable in Lua and convert it to complex C++ type.
   *
   * @tparam T The result type user expected. T can be any type composited by
   * bool, integer types, float number types, std::string, std::vector,
   * std::set, std::unordered_set, std::map, std::unordered_map, std::pair. Note
   * that here const char* is not supported, which is unsafe.
   * @param [in] name The variable's name.
   * @param [in] disable_log Whether print a log when exception occurs.
   * @param [out] failed Will be set whether the operation is failed if this
   * pointer is not nullptr. If T is a container type, it regards the operation
   * as failed if any element failed.
   * @param [out] exists Set whether the variable exists. Regard none and nil as
   * not exists.
   * @return Return the value with given name in type T if conversion succeeds,
   * otherwise, if T is a simple type (e.g. bool, int, double, std::string,
   * etc), return initial value of T(i.e. by statement `T{}`), if T is a
   * container type, the result will contain all non-nil elements whose
   * conversion succeeded and discard elements who are nil or elements whose
   * conversion failed.
   *
   */
  template <typename T>
  T get(const char* name,
        bool        disable_log = false,
        bool*       failed      = nullptr,
        bool*       exists      = nullptr) {
    getglobal(name);
    auto ret = to<T>(-1, disable_log, failed, exists);
    pop();
    return ret;
  }
  template <typename T>
  T get(const std::string& name,
        bool               disable_log = false,
        bool*              failed      = nullptr,
        bool*              exists      = nullptr) {
    return get<T>(name.c_str(), disable_log, failed, exists);
  }

/**
 * @brief Recursively get values in Lua and convert it to simple C++ type.
 *
 * @param [in] path The first key in path should be a Lua global variable,
 * the last key in path should be a value which can convert to expected type,
 * excpet for the last key, all other keys in path should be Lua table.
 * @param [in] def The default value returned if failed or target does not
 * exist.
 * @param [in] disable_log Whether print a log when exception occurs.
 * @param [out] failed Will be set whether the operation is failed if this
 * pointer is not nullptr. If T is a container type, it regards the operation
 * as failed if any element failed.
 * @param [out] exists Set whether the variable exists. Regard none and nil as
 * not exists.
 *
 * @{
 */
#define DEFINE_RECURSIVE_GET_SIMPLE_TYPE(typename, type, default)              \
  type get_##typename(const std::initializer_list<const char*>& path,          \
                      const type&                               def = default, \
                      bool  disable_log                             = false,   \
                      bool* failed                                  = nullptr, \
                      bool* exists                                  = nullptr) {                                \
    return __get<type>(                                                        \
        path.begin(), path.end(), def, disable_log, failed, exists);           \
  }                                                                            \
  type get_##typename(const std::initializer_list<std::string>& path,          \
                      const type&                               def = default, \
                      bool  disable_log                             = false,   \
                      bool* failed                                  = nullptr, \
                      bool* exists                                  = nullptr) {                                \
    return __get<type>(                                                        \
        path.begin(), path.end(), def, disable_log, failed, exists);           \
  }                                                                            \
  type get_##typename(const std::vector<const char*>& path,                    \
                      const type&                     def         = default,   \
                      bool                            disable_log = false,     \
                      bool*                           failed      = nullptr,   \
                      bool*                           exists      = nullptr) {                                \
    return __get<type>(                                                        \
        path.begin(), path.end(), def, disable_log, failed, exists);           \
  }                                                                            \
  type get_##typename(const std::vector<std::string>& path,                    \
                      const type&                     def         = default,   \
                      bool                            disable_log = false,     \
                      bool*                           failed      = nullptr,   \
                      bool*                           exists      = nullptr) {                                \
    return __get<type>(                                                        \
        path.begin(), path.end(), def, disable_log, failed, exists);           \
  }

  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(bool, bool, false)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(int, int, 0)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(uint, unsigned int, 0)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(long, long, 0)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(ulong, unsigned long, 0)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(llong, long long, 0)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(ullong, unsigned long long, 0)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(float, float, 0)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(double, double, 0)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(ldouble, long double, 0)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(string, std::string, "")
#undef DEFINE_RECURSIVE_GET_SIMPLE_TYPE

  /** @} */

  /**
   * @brief Recursively get values in Lua and convert it to complex C++ type.
   *
   * No default value provided. If T is not a container type, it will return T's
   * initial value if operation fails or target does not exist. If T is a
   * container type, the result will contain elements that successfully
   * converted and discard elements fails or not exists. Regard none and nil as
   * not exists.
   *
   * @param [in] path The first key in path should be a Lua global variable,
   * the last key in path should be a value which can convert to expected type,
   * excpet for the last key, all other keys in path should be Lua table.
   * @param [in] disable_log Whether print a log when exception occurs.
   * @param [out] failed Will be set whether the operation is failed if this
   * pointer is not nullptr. If T is a container type, it regards the operation
   * as failed if any element failed.
   * @param [out] exists Set whether the variable exists. Regard none and nil as
   * not exists.
   *
   * @{
   */
  template <typename T>
  T get(const std::initializer_list<const char*>& path,
        bool                                      disable_log = false,
        bool*                                     failed      = nullptr,
        bool*                                     exists      = nullptr) {
    return __get<T>(path.begin(), path.end(), disable_log, failed, exists);
  }
  template <typename T>
  T get(const std::initializer_list<std::string>& path,
        bool                                      disable_log = false,
        bool*                                     failed      = nullptr,
        bool*                                     exists      = nullptr) {
    return __get<T>(path.begin(), path.end(), disable_log, failed, exists);
  }
  template <typename T>
  T get(const std::vector<const char*>& path,
        bool                            disable_log = false,
        bool*                           failed      = nullptr,
        bool*                           exists      = nullptr) {
    return __get<T>(path.begin(), path.end(), disable_log, failed, exists);
  }
  template <typename T>
  T get(const std::vector<std::string>& path,
        bool                            disable_log = false,
        bool*                           failed      = nullptr,
        bool*                           exists      = nullptr) {
    return __get<T>(path.begin(), path.end(), disable_log, failed, exists);
  }

  /** @} */

  struct lgetopt {
    bool  disable_log;
    bool *failed, *exists;
    lgetopt(bool d = false, bool* f = nullptr, bool* e = nullptr)
        : disable_log(d), failed(f), exists(e) {}
  };

  /// Long get. Args is the path to get value.
  template <typename T, typename... Args>
  T lget(const lgetopt& o, Args&&... args) {
    static_assert(sizeof...(Args) > 0, "lget needs at least one key in path");
    auto _g = make_guarder();
    lseek(std::forward<Args>(args)...);
    return to<T>(-1, o.disable_log, o.failed, o.exists);
  }

private:
  template <typename T, typename Iterator>
  T __get(Iterator b,
          Iterator e,
          bool     disable_log = false,
          bool*    failed      = nullptr,
          bool*    exists      = nullptr) {
    if (b == e) {
      if (failed) *failed = false;
      if (exists) *exists = false;
      return T();
    }
    auto _g = make_guarder();
    auto it = b;
    gseek(*it++);
    if (it == e) { return to<T>(-1, disable_log, failed, exists); }
    while (it != e) {
      if (isnoneornil()) {
        if (failed) *failed = false;
        if (exists) *exists = false;
        return T();
      }
      if (!(istable() || indexable())) {
        if (failed) *failed = true;
        if (exists) *exists = true;
        if (!disable_log)
          log_type_convert_error(-1, "table or indexable value");
        return T();
      }
      seek(*it++);
    }
    return to<T>(-1, disable_log, failed, exists);
  }

  template <typename T, typename Iterator>
  T __get(Iterator b,
          Iterator e,
          const T& def,
          bool     disable_log = false,
          bool*    failed      = nullptr,
          bool*    exists      = nullptr) {
    bool tfailed, texists;
    auto ret = __get<T>(b, e, disable_log, &tfailed, &texists);
    if (failed) *failed = tfailed;
    if (exists) *exists = texists;
    if (tfailed || !texists) return def;
    return ret;
  }

public:
  //////////////////////// register ctor/memeber for class /////////////////////

  /// Register a global function to Lua who can create object of type Return,
  /// where Return is the return type of Ctor.
  /// e.g. register_ctor<Object(int)>("NewObject").
  template <typename Ctor>
  void register_ctor(const char* fname) {
    static_assert(std::is_function<Ctor>::value,
                  "Ctor should be type of Return(Args...)");
    PEACALM_LUAW_ASSERT(fname);
    register_ctor_impl<std::decay_t<Ctor>>::register_ctor(*this, fname);
  }
  template <typename Ctor>
  void register_ctor(const std::string& fname) {
    register_ctor<Ctor>(fname.c_str());
  }

  /// Register a real member, ether member variable or member function.
  /// For overloaded member function, you can explicitly pass in the template
  /// parameter MemberPointer. e.g.
  /// `register_member<Return(Class::*)(Args)>("mf", &Class::mf)`
  template <typename MemberPointer>
  std::enable_if_t<std::is_member_pointer<MemberPointer>::value>
  register_member(const char* name, MemberPointer mp) {
    PEACALM_LUAW_ASSERT(name);
    registrar<std::decay_t<MemberPointer>>::register_member(
        *this, name, std::mem_fn(mp));
  }
  template <typename MemberPointer>
  std::enable_if_t<std::is_member_pointer<MemberPointer>::value>
  register_member(const std::string& name, MemberPointer mp) {
    register_member<MemberPointer>(name.c_str(), mp);
  }

  /// Register a fake member function, who has proto type
  /// `Return(Class*, Args...)`.
  template <typename Hint, typename F>
  std::enable_if_t<std::is_member_pointer<Hint>::value &&
                   !std::is_same<Hint, F>::value>
  register_member(const char* name, F&& f) {
    PEACALM_LUAW_ASSERT(name);
    registrar<std::decay_t<Hint>>::register_member(
        *this, name, mock_mem_fn<F>(std::forward<F>(f)));
  }
  template <typename Hint, typename F>
  std::enable_if_t<std::is_member_pointer<Hint>::value &&
                   !std::is_same<Hint, F>::value>
  register_member(const std::string& name, F&& f) {
    register_member<Hint, F>(name.c_str(), std::forward<F>(f));
  }

  /// Register generic members by provide generic member getter and setter.
  /// getter/setter could be C function or lambda object.
  /// getter proto type: Member(const Class*, Key)
  /// setter proto type: void(Class*, Key, Member)
  /// where Key to be `const std::string&` is recommended, Member could be
  /// number, string, bool, luaw::luavalueidx, etc.
  /// Besides, getter/setter could also be nullptr, indicate unregister the
  /// generic member getter/setter, or no need to register the getter/setter.
  template <typename Class, typename Getter, typename Setter>
  void register_generic_member(Getter&& getter, Setter&& setter) {
    registrar<std::decay_t<Class>>::register_generic_member_getter(
        *this, std::forward<Getter>(getter));
    registrar<std::decay_t<Class>>::register_generic_member_setter(
        *this, std::forward<Setter>(setter));
  }

  //////////////////////// evaluate expression /////////////////////////////////

  /**
   * @brief Evaluate a Lua expression and get the result in simple C++ type.
   *
   * @param [in] expr Lua expression, which must have a return value. Only the
   * first one is used if multiple values returned.
   * @param [in] def The default value returned if failed.
   * @param [in] disable_log Whether print a log when exception occurs.
   * @param [out] failed Will be set whether the operation is failed if this
   * pointer is not nullptr.
   *
   * @{
   */

#define DEFINE_EVAL(typename, type, default)                        \
  type eval_##typename(const char* expr,                            \
                       const type& def         = default,           \
                       bool        disable_log = false,             \
                       bool*       failed      = nullptr) {                    \
    int  sz = gettop();                                             \
    auto _g = make_guarder();                                       \
    if (dostring(expr) != LUA_OK) {                                 \
      if (failed) *failed = true;                                   \
      if (!disable_log) log_error_in_stack();                       \
      return def;                                                   \
    }                                                               \
    PEACALM_LUAW_ASSERT(gettop() >= sz);                            \
    if (gettop() <= sz) {                                           \
      if (failed) *failed = true;                                   \
      if (!disable_log) log_error("No return");                     \
      return def;                                                   \
    }                                                               \
    type ret = to_##typename(sz + 1, def, disable_log, failed);     \
    return ret;                                                     \
  }                                                                 \
  type eval_##typename(const std::string& expr,                     \
                       const type&        def         = default,    \
                       bool               disable_log = false,      \
                       bool*              failed      = nullptr) {                    \
    return eval_##typename(expr.c_str(), def, disable_log, failed); \
  }

  DEFINE_EVAL(bool, bool, false)
  DEFINE_EVAL(int, int, 0)
  DEFINE_EVAL(uint, unsigned int, 0)
  DEFINE_EVAL(long, long, 0)
  DEFINE_EVAL(ulong, unsigned long, 0)
  DEFINE_EVAL(llong, long long, 0)
  DEFINE_EVAL(ullong, unsigned long long, 0)
  DEFINE_EVAL(float, float, 0)
  DEFINE_EVAL(double, double, 0)
  DEFINE_EVAL(ldouble, long double, 0)
  DEFINE_EVAL(string, std::string, "")
#undef DEFINE_EVAL

  // Caller is responsible for popping stack if succeeds
  const char* eval_c_str(const char* expr,
                         const char* def         = "",
                         bool        disable_log = false,
                         bool*       failed      = nullptr) {
    int sz = gettop();
    if (dostring(expr) != LUA_OK) {
      if (failed) *failed = true;
      if (!disable_log) log_error_in_stack();
      settop(sz);
      return def;
    }
    PEACALM_LUAW_ASSERT(gettop() >= sz);
    if (gettop() <= sz) {
      if (failed) *failed = true;
      if (!disable_log) log_error("No return");
      return def;
    }
    return to_c_str(sz + 1, def, disable_log, failed);
  }
  const char* eval_c_str(const std::string& expr,
                         const char*        def         = "",
                         bool               disable_log = false,
                         bool*              failed      = nullptr) {
    return eval_c_str(expr.c_str(), def, disable_log, failed);
  }

  /** @} */

  /**
   * @brief Evaluate a Lua expression and get result in complex C++ type.
   *
   * @tparam T The result type user expected. T can be any type composited by
   * bool, integer types, float number type, std::string, std::vector, std::set,
   * std::unordered_set, std::map, std::unordered_map, std::pair, std::tuple.
   * What's more, T can be void or std::tuple<> to represent a expression who
   * has no results or we don't use the results.
   * @param [in] expr Lua expression, which can have no return, one return or
   * multiple returns.
   * @param [in] disable_log Whether print a log when exception occurs.
   * @param [out] failed Will be set whether the operation is failed if this
   * pointer is not nullptr. If T is a container type, it regards the operation
   * as failed if any element failed.
   * @return The expression's result in type T. If T is not void or
   * std::tuple<>, the expression must provide results.
   */
  template <typename T>
  T eval(const char* expr, bool disable_log = false, bool* failed = nullptr) {
    auto _g = make_guarder();
    int  sz = gettop();
    if (dostring(expr) != LUA_OK) {
      if (failed) *failed = true;
      if (!disable_log) log_error_in_stack();
      return T();
    }
    PEACALM_LUAW_ASSERT(gettop() >= sz);
    if (gettop() <= sz && !std::is_same<std::decay_t<T>, std::tuple<>>::value &&
        !std::is_same<std::decay_t<T>, void>::value) {
      if (failed) *failed = true;
      if (!disable_log) log_error("No return");
      return T();
    }
    return to<T>(sz + 1, disable_log, failed);
  }
  template <typename T>
  T eval(const std::string& expr,
         bool               disable_log = false,
         bool*              failed      = nullptr) {
    return eval<T>(expr.c_str(), disable_log, failed);
  }

  ///////////////////////// error log //////////////////////////////////////////

  void log_error(const char* s) const {
    std::cerr << "Lua: " << s << std::endl;
  }

  void log_error_in_stack(int idx = -1) const {
    std::cerr << "Lua: " << lua_tostring(L_, idx) << std::endl;
  }

  void log_error_out(int idx = -1) {
    log_error_in_stack(idx);
    pop();
  }

  void log_type_convert_error(int idx, const char* to) {
    std::cerr << "Lua: Can't convert to " << to << " by ";
    if (isnumber(idx) || isstring(idx) || isboolean(idx) || isinteger(idx)) {
      std::cerr << type_name(idx) << ": ";
    }
    if (isstring(idx)) {
      std::cerr << lua_tostring(L_, idx) << std::endl;
    } else {
      std::cerr << luaL_tolstring(L_, idx, NULL) << std::endl;
      pop();
    }
  }
};

// This wrapper won't close state when destruct.
class luaw_fake : public luaw {
  using base_t = luaw;

public:
  luaw_fake(lua_State* L) : base_t(L) {}

  ~luaw_fake() { base_t::release(); }
};

////////////////////////////////////////////////////////////////////////////////

namespace luaw_detail {

// void_t is defined in std since c++17
template <typename T>
using void_t = void;

// is std::shared_ptr

template <typename T>
struct is_std_shared_ptr : std::false_type {};

template <typename T>
struct is_std_shared_ptr<std::shared_ptr<T>> : std::true_type {};

// is std::unique_ptr

template <typename T>
struct is_std_unique_ptr : std::false_type {};

template <typename T, typename D>
struct is_std_unique_ptr<std::unique_ptr<T, D>> : std::true_type {};

// get element_type

template <typename T, typename = void>
struct get_element_type {
  using type = void;
};

template <typename T>
struct get_element_type<T, void_t<typename T::element_type>> {
  using type = typename T::element_type;
};

// is_stdfunction

template <typename T>
struct is_stdfunction : std::false_type {};

template <typename Return, typename... Args>
struct is_stdfunction<std::function<Return(Args...)>> : std::true_type {};

template <typename Return, typename... Args>
struct is_stdfunction<std::function<Return(Args..., ...)>> : std::true_type {};

template <typename T>
struct decay_is_stdfunction : is_stdfunction<std::decay_t<T>> {};

// Get a C function pointer type by a member function pointer type

template <typename T>
struct detect_cfunction {
  using type = void;
};

template <typename Object, typename Return, typename... Args>
struct detect_cfunction<Return (Object::*)(Args...)> {
  using type = Return (*)(Args...);
};

template <typename Object, typename Return, typename... Args>
struct detect_cfunction<Return (Object::*)(Args...) const> {
  using type = Return (*)(Args...);
};

template <typename Object, typename Return, typename... Args>
struct detect_cfunction<Return (Object::*)(Args..., ...)> {
  using type = Return (*)(Args..., ...);
};

template <typename Object, typename Return, typename... Args>
struct detect_cfunction<Return (Object::*)(Args..., ...) const> {
  using type = Return (*)(Args..., ...);
};

template <typename T>
using detect_cfunction_t = typename detect_cfunction<T>::type;

// Detect type of callee, which is a pointer to member function operator()

template <typename T, typename = void>
struct detect_callee {
  using type = void;
};

template <typename T>
struct detect_callee<T, void_t<decltype(&T::operator())>> {
  using type = decltype(&T::operator());
};

template <typename T>
using detect_callee_t = typename detect_callee<T>::type;

// Detect C function style callee type

template <typename T>
using detect_c_callee_t = detect_cfunction_t<detect_callee_t<T>>;

// Detect whether T maybe a captureless and non-generic lambda

template <typename T>
struct maybe_lambda
    : std::integral_constant<
          bool,
          !std::is_same<T, void>::value &&
              std::is_convertible<T, detect_c_callee_t<T>>::value> {};

// Detect whether decay_t<T> maybe a captureless and non-generic lambda

template <typename T>
struct decay_maybe_lambda : maybe_lambda<std::decay_t<T>> {};

// Whether T has a non-template member operator()

template <typename T, typename = void>
struct is_callable_class : std::false_type {};

template <typename T>
struct is_callable_class<T, void_t<decltype(&T::operator())>> : std::true_type {
};

// whether T is C function

template <typename T>
struct is_cfunction : std::false_type {};

template <typename Return, typename... Args>
struct is_cfunction<Return(Args...)> : std::true_type {};

template <typename Return, typename... Args>
struct is_cfunction<Return(Args..., ...)> : std::true_type {};

// whether T is C function pointer

template <typename T>
struct is_cfunction_pointer
    : std::integral_constant<
          bool,
          std::is_pointer<T>::value &&
              is_cfunction<std::remove_pointer_t<T>>::value> {};

// Whether T is C style callable: C function or C function pointer

template <typename T>
struct is_callable_c
    : std::integral_constant<bool,
                             is_cfunction<T>::value ||
                                 is_cfunction_pointer<T>::value> {};

// whether T is callable

template <typename T>
struct is_callable : std::integral_constant<bool,
                                            is_callable_class<T>::value ||
                                                is_callable_c<T>::value> {};

// whether decay_t<T> is callable

template <typename T>
struct decay_is_callable : is_callable<std::decay_t<T>> {};

// detect C function pointer type by a C function or a callable class

template <typename T>
using detect_callable_cfunction_t = std::conditional_t<
    decay_is_callable<T>::value,
    std::conditional_t<is_callable_class<std::decay_t<T>>::value,
                       detect_c_callee_t<std::decay_t<T>>,
                       std::decay_t<T>>,
    void>;

// whether T is std::tuple

template <typename T>
struct __is_stdtuple : std::false_type {};

template <typename... Ts>
struct __is_stdtuple<std::tuple<Ts...>> : std::true_type {};

template <typename T>
using is_stdtuple = __is_stdtuple<std::decay_t<T>>;

}  // namespace luaw_detail

//////////////////// push impl ////////////////////////////////////////////////

// primary pusher. guess whether it may be a lambda, push as function if true,
// otherwise push as an user defined custom object.
template <typename T, typename>
struct luaw::pusher {
  // Pointers should be specialized elsewhere.
  static_assert(!std::is_pointer<T>::value, "Cannot be pointer");

  static const size_t size = 1;

  template <typename Y>
  static int push(luaw& l, Y&& v) {
    // Guess whether it may be a lambda object, if it is, then push as a
    // function, otherwise push as custom class.
    constexpr bool push_as_function = luaw_detail::decay_maybe_lambda<Y>::value;
    using Tag                       = std::
        conditional_t<push_as_function, luaw::function_tag, luaw::class_tag>;

    // Ensure push v as type T if not push as function.

    static_assert(std::is_same<T, std::decay_t<T>>::value,
                  "T should be decayed");

    using T0 = std::decay_t<T>;
    using T1 =
        std::conditional_t<std::is_const<std::remove_reference_t<Y>>::value,
                           std::add_const_t<T0>,
                           T0>;
    using T2 =
        std::conditional_t<std::is_volatile<std::remove_reference_t<Y>>::value,
                           std::add_volatile_t<T1>,
                           T1>;
    using TargetType = std::conditional_t<
        push_as_function,
        Y,
        std::conditional_t<std::is_lvalue_reference<Y>::value, T2&, T2>>;

    static_assert(
        (std::is_reference<TargetType>::value == std::is_reference<Y>::value) &&
            (std::is_lvalue_reference<TargetType>::value ==
             std::is_lvalue_reference<Y>::value) &&
            (std::is_rvalue_reference<TargetType>::value ==
             std::is_rvalue_reference<Y>::value) &&
            (std::is_const<std::remove_reference_t<TargetType>>::value ==
             std::is_const<std::remove_reference_t<Y>>::value) &&
            (std::is_volatile<std::remove_reference_t<TargetType>>::value ==
             std::is_volatile<std::remove_reference_t<Y>>::value),
        "TargetType should have same cvr- as Y");

    return luaw::pusher<Tag>::push(l, std::forward<TargetType>(v));
  }
};

// pointer to class
template <typename T>
struct luaw::pusher<T*,
                    std::enable_if_t<!std::is_function<T>::value &&
                                     !std::is_same<T*, const char*>::value &&
                                     std::is_class<std::decay_t<T>>::value>> {
  static const size_t size = 1;

  template <typename Y>
  static int push(luaw& l, Y* v) {
    l.pushlightuserdata(
        reinterpret_cast<void*>(const_cast<std::remove_cv_t<Y>*>(v)));

    luaw::metatable_factory<T*>::push_shared_metatable(l);
    l.setmetatable(-2);

    return 1;
  }
};

// non-class pointer, as lightuserdata
template <typename T>
struct luaw::pusher<T*,
                    std::enable_if_t<!std::is_function<T>::value &&
                                     !std::is_same<T*, const char*>::value &&
                                     !std::is_class<std::decay_t<T>>::value>> {
  static const size_t size = 1;

  template <typename Y>
  static int push(luaw& l, Y* v) {
    l.pushlightuserdata(
        reinterpret_cast<void*>(const_cast<std::remove_cv_t<Y>*>(v)));
    return 1;
  }
};

// class_tag: push as an user defined custom class type.
// only work on type class or pointer to class.
template <>
struct luaw::pusher<luaw::class_tag> {
  static const size_t size = 1;

  template <typename Y>
  static int push(luaw& l, Y&& v) {
    using SolidY = std::remove_reference_t<Y>;
    static_assert(std::is_class<std::remove_pointer_t<SolidY>>::value,
                  "Only class or it's pointer");

    __push<SolidY, Y>(l, std::forward<Y>(v), std::is_pointer<SolidY>{});

    return 1;
  }

private:
  template <typename SolidY, typename Y>
  static void __push(luaw& l, Y&& v, std::true_type) {
    l.pushlightuserdata(reinterpret_cast<void*>(
        const_cast<std::remove_cv_t<std::remove_pointer_t<SolidY>>*>(v)));
    luaw::metatable_factory<SolidY>::push_shared_metatable(l);
    l.setmetatable(-2);
  }

  template <typename SolidY, typename Y>
  static void __push(luaw& l, Y&& v, std::false_type) {
    void* p = l.newuserdata(sizeof(SolidY));
    new (p) SolidY(std::forward<Y>(v));
    luaw::metatable_factory<SolidY*>::push_shared_metatable(l);
    l.setmetatable(-2);
  }
};

// newtable_tag: push a new empty table
template <>
struct luaw::pusher<luaw::newtable_tag> {
  static const size_t size = 1;

  static int push(luaw& l, newtable_tag) {
    l.newtable();
    return 1;
  }
};

// function_tag: push as a function
template <>
struct luaw::pusher<luaw::function_tag> {
  static const size_t size = 1;

  template <typename F>
  static int push(luaw& l, F&& f) {
    using DecayF = std::decay_t<F>;
    static_assert(luaw_detail::is_callable<DecayF>::value,
                  "Should push a callable value");

    // C function pointer type
    using CFunctionPtr =
        std::conditional_t<luaw_detail::is_callable_class<DecayF>::value,
                           luaw_detail::detect_c_callee_t<DecayF>,
                           DecayF>;
    static_assert(luaw_detail::is_cfunction_pointer<CFunctionPtr>::value,
                  "Should get a C function pointer type");
    return luaw::pusher<CFunctionPtr>::push(l, std::forward<F>(f));
  }
};

// std::function
template <typename Return, typename... Args>
struct luaw::pusher<std::function<Return(Args...)>> {
  static const size_t size = 1;

  template <typename F>
  static int push(luaw& l, F&& f) {
    using CFunctionPtr = Return (*)(Args...);
    return luaw::pusher<CFunctionPtr>::push(l, std::forward<F>(f));
  }
};

// Lua C functions
template <>
struct luaw::pusher<luaw::lua_cfunction_t> {
  static const size_t size = 1;

  static int push(luaw& l, lua_cfunction_t f) {
    l.pushcfunction(f);
    return 1;
  }
};

// C funtions except lua_cfunction_t, also implementation for callable objects
template <typename Return, typename... Args>
struct luaw::pusher<Return (*)(Args...)> {
  static const size_t size = 1;

  // function object with non-trivially destructor
  template <typename F>
  static std::
      enable_if_t<!std::is_trivially_destructible<std::decay_t<F>>::value, int>
      push(luaw& l, F&& f) {
    using SolidF = std::remove_reference_t<F>;

    luaw::lua_cfunction_t __call = [](lua_State* L) -> int {
      PEACALM_LUAW_ASSERT(lua_gettop(L) >= 1);
      PEACALM_LUAW_ASSERT(lua_isuserdata(L, 1));
      auto callee = static_cast<SolidF*>(lua_touserdata(L, 1));
      PEACALM_LUAW_ASSERT(callee);
      luaw l(L);
      int  ret_num = callback(l, *callee, 2, std::is_void<Return>{});
      l.release();
      return ret_num;
    };

    luaw::lua_cfunction_t __gc = [](lua_State* L) -> int {
      PEACALM_LUAW_ASSERT(lua_gettop(L) == 1);
      PEACALM_LUAW_ASSERT(lua_isuserdata(L, 1));
      auto fptr = static_cast<SolidF*>(lua_touserdata(L, 1));
      PEACALM_LUAW_ASSERT(fptr);
      fptr->~SolidF();
      return 0;
    };

    // object
    auto faddr = static_cast<SolidF*>(l.newuserdata(sizeof(f)));
    new (faddr) SolidF(std::forward<F>(f));

    // build metatable
    l.newtable();

    l.pushstring("__call");
    l.pushcfunction(__call);
    l.rawset(-3);

    l.pushstring("__gc");
    l.pushcfunction(__gc);
    l.rawset(-3);

    l.setmetatable(-2);

    return 1;
  }

  // function object with trivially destructor
  template <typename F>
  static std::
      enable_if_t<std::is_trivially_destructible<std::decay_t<F>>::value, int>
      push(luaw& l, F&& f) {
    using SolidF = std::remove_reference_t<F>;

    auto closure = [](lua_State* L) -> int {
      auto callee =
          static_cast<SolidF*>(lua_touserdata(L, lua_upvalueindex(1)));
      PEACALM_LUAW_ASSERT(callee);
      luaw l(L);
      int  ret_num = callback(l, *callee, 1, std::is_void<Return>{});
      l.release();
      return ret_num;
    };

    auto faddr = static_cast<SolidF*>(l.newuserdata(sizeof(f)));
    new (faddr) SolidF(std::forward<F>(f));

    l.pushcclosure(closure, 1);

    return 1;
  }

  // function pointer
  static int push(luaw& l, Return (*f)(Args...)) {
    auto closure = [](lua_State* L) -> int {
      auto callee = reinterpret_cast<Return (*)(Args...)>(
          lua_touserdata(L, lua_upvalueindex(1)));
      PEACALM_LUAW_ASSERT(callee);
      luaw l(L);
      int  ret_num = callback(l, callee, 1, std::is_void<Return>{});
      l.release();
      return ret_num;
    };
    l.pushlightuserdata(reinterpret_cast<void*>(f));
    l.pushcclosure(closure, 1);
    return 1;
  }

private:
  template <typename>
  struct wrap {};

  template <typename Callee, typename FirstArg, typename... RestArgs>
  struct bind {
    Callee   callee;
    FirstArg first_arg;
    bind(Callee&& c, FirstArg&& f)
        : callee(std::forward<Callee>(c)),
          first_arg(std::forward<FirstArg>(f)) {}

    Return operator()(RestArgs&&... args) {
      return callee(std::move(first_arg), std::forward<RestArgs>(args)...);
    }
  };

  // Return is void
  template <typename Callee>
  static int callback(luaw& l, Callee&& c, int start_idx, std::true_type) {
    do_call(l, std::forward<Callee>(c), start_idx, 1, wrap<Args>{}...);
    return 0;
  }

  // Return is not void
  template <typename Callee>
  static int callback(luaw& l, Callee&& c, int start_idx, std::false_type) {
    return l.push(
        do_call(l, std::forward<Callee>(c), start_idx, 1, wrap<Args>{}...));
  }

  template <typename Callee, typename FirstArg, typename... RestArgs>
  static Return do_call(luaw&          l,
                        Callee&&       c,
                        int            i,
                        int            counter,
                        wrap<FirstArg> farg,
                        wrap<RestArgs>... args) {
    bool failed, exists;
    auto param = l.to<std::decay_t<FirstArg>>(i, false, &failed, &exists);
    if (failed) {
      luaL_error(l.L(), "The %dth argument conversion failed", counter);
      return Return();  // never runs here
    }
    bind<Callee, FirstArg, RestArgs...> b(std::forward<Callee>(c),
                                          std::move(param));
    return do_call(l, std::move(b), i + 1, counter + 1, args...);
  }

  template <typename Callee>
  static Return do_call(luaw& l, Callee&& c, int i, int counter) {
    return c();
  }
};

// variadic function, not supported
template <typename Return, typename... Args>
struct luaw::pusher<Return (*)(Args..., ...)> {
  // write nothing to let compile fail
};

// bool
template <>
struct luaw::pusher<bool> {
  static const size_t size = 1;

  static int push(luaw& l, bool v) {
    lua_pushboolean(l.L(), v);
    return 1;
  }
};

// integer types
template <typename IntType>
struct luaw::pusher<IntType,
                    std::enable_if_t<std::is_integral<IntType>::value>> {
  static const size_t size = 1;

  static int push(luaw& l, IntType v) {
    lua_pushinteger(l.L(), v);
    return 1;
  }
};

// float number types
template <typename FloatType>
struct luaw::pusher<
    FloatType,
    std::enable_if_t<std::is_floating_point<FloatType>::value>> {
  static const size_t size = 1;

  static int push(luaw& l, FloatType v) {
    lua_pushnumber(l.L(), v);
    return 1;
  }
};

// std::string
template <>
struct luaw::pusher<std::string> {
  static const size_t size = 1;

  static int push(luaw& l, const std::string& v) {
    lua_pushstring(l.L(), v.c_str());
    return 1;
  }
};

// const char*
template <>
struct luaw::pusher<const char*> {
  static const size_t size = 1;

  static int push(luaw& l, const char* v) {
    // lua_pushstring will push nil if v is NULL, we forbid this
    PEACALM_LUAW_ASSERT(v);
    lua_pushstring(l.L(), v);
    return 1;
  }
};

// nullptr
template <>
struct luaw::pusher<std::nullptr_t> {
  static const size_t size = 1;

  static int push(luaw& l, std::nullptr_t) {
    l.pushnil();
    return 1;
  }
};

// std::pair
template <typename T, typename U>
struct luaw::pusher<std::pair<T, U>> {
  static const size_t size = 1;

  static int push(luaw& l, const std::pair<T, U>& p) {
    l.newtable();
    l.push(p.first);
    l.rawseti(-2, 1);
    l.push(p.second);
    l.rawseti(-2, 2);
    return 1;
  }
};

namespace luaw_detail {

// Implementation for all list like containers
template <typename Container>
static int __push_list(luaw& l, const Container& v) {
  l.newtable();
  int cnt = 0;
  for (auto b = v.begin(), e = v.end(); b != e; ++b) {
    l.push(*b);
    l.rawseti(-2, ++cnt);
  }
  return 1;
}

// Implementation for all set like containers
// Make a Key-True table in Lua to represent set
template <typename Container>
static int __push_set(luaw& l, const Container& v) {
  l.newtable();
  for (auto b = v.begin(), e = v.end(); b != e; ++b) {
    l.push(*b);
    lua_pushboolean(l.L(), 1);
    l.rawset(-3);
  }
  return 1;
}

// Implementation for all map like containers
template <typename Container>
static int __push_map(luaw& l, const Container& v) {
  l.newtable();
  for (auto b = v.begin(), e = v.end(); b != e; ++b) {
    l.push(b->first);
    l.push(b->second);
    l.rawset(-3);
  }
  return 1;
}

}  // namespace luaw_detail

// std::array
template <typename T, std::size_t N>
struct luaw::pusher<std::array<T, N>> {
  static const size_t size = 1;

  static int push(luaw& l, const std::array<T, N>& v) {
    return luaw_detail::__push_list(l, v);
  }
};

// std::vector
template <typename T, typename Allocator>
struct luaw::pusher<std::vector<T, Allocator>> {
  static const size_t size = 1;

  static int push(luaw& l, const std::vector<T, Allocator>& v) {
    return luaw_detail::__push_list(l, v);
  }
};

// std::deque
template <typename T, typename Allocator>
struct luaw::pusher<std::deque<T, Allocator>> {
  static const size_t size = 1;

  static int push(luaw& l, const std::deque<T, Allocator>& v) {
    return luaw_detail::__push_list(l, v);
  }
};

// std::forward_list
template <typename T, typename Allocator>
struct luaw::pusher<std::forward_list<T, Allocator>> {
  static const size_t size = 1;

  static int push(luaw& l, const std::forward_list<T, Allocator>& v) {
    return luaw_detail::__push_list(l, v);
  }
};

// std::list
template <typename T, typename Allocator>
struct luaw::pusher<std::list<T, Allocator>> {
  static const size_t size = 1;

  static int push(luaw& l, const std::list<T, Allocator>& v) {
    return luaw_detail::__push_list(l, v);
  }
};

// std::set
template <typename Key, typename Compare, typename Allocator>
struct luaw::pusher<std::set<Key, Compare, Allocator>> {
  static const size_t size = 1;

  static int push(luaw& l, const std::set<Key, Compare, Allocator>& v) {
    return luaw_detail::__push_set(l, v);
  }
};

// std::unordered_set
template <typename Key, typename Hash, typename KeyEqual, typename Allocator>
struct luaw::pusher<std::unordered_set<Key, Hash, KeyEqual, Allocator>> {
  static const size_t size = 1;

  static int push(luaw&                                                     l,
                  const std::unordered_set<Key, Hash, KeyEqual, Allocator>& v) {
    return luaw_detail::__push_set(l, v);
  }
};

// std::map
template <typename Key, typename Compare, typename Allocator>
struct luaw::pusher<std::map<Key, Compare, Allocator>> {
  static const size_t size = 1;

  static int push(luaw& l, const std::map<Key, Compare, Allocator>& v) {
    return luaw_detail::__push_map(l, v);
  }
};

// std::unordered_map
template <typename Key, typename Hash, typename KeyEqual, typename Allocator>
struct luaw::pusher<std::unordered_map<Key, Hash, KeyEqual, Allocator>> {
  static const size_t size = 1;

  static int push(luaw&                                                     l,
                  const std::unordered_map<Key, Hash, KeyEqual, Allocator>& v) {
    return luaw_detail::__push_map(l, v);
  }
};

// std::tuple
// Push elements separately in order. One element takes one place in stack.
// Any element of the tuple should not be a tuple anymore.
template <typename... Ts>
struct luaw::pusher<std::tuple<Ts...>> {
  static const size_t size = std::tuple_size<std::tuple<Ts...>>::value;

  static int push(luaw& l, const std::tuple<Ts...>& v) {
    const size_t N = size;
    int ret_num    = __push<0, N>(l, v, std::integral_constant<bool, 0 < N>{});
    PEACALM_LUAW_ASSERT(ret_num == N);
    return ret_num;
  }

private:
  template <size_t I, size_t N, typename T>
  static int __push(luaw& l, const T& t, std::true_type) {
    static_assert(!luaw_detail::is_stdtuple<std::tuple_element_t<I, T>>::value,
                  "Recursive tuple is not allowed");
    int x = l.push(std::get<I>(t));
    int y = __push<I + 1, N>(l, t, std::integral_constant<bool, I + 1 < N>{});
    return x + y;
  }
  template <size_t I, size_t N, typename T>
  static int __push(luaw& l, const T& t, std::false_type) {
    return 0;
  }
};

template <>
struct luaw::pusher<luaw::luavalueidx> {
  static const size_t size = 1;

  static int push(luaw& l, const luaw::luavalueidx& r) {
    PEACALM_LUAW_ASSERT(l.L() == r.L);
    l.pushvalue(r.idx);
    return 1;
  }
};

template <>
struct luaw::pusher<luaw::luavalueref> {
  static const size_t size = 1;

  static int push(luaw& l, const luaw::luavalueref& r) {
    PEACALM_LUAW_ASSERT(l.L() == r.L);
    l.rawgeti(LUA_REGISTRYINDEX, r.ref_id);
    return 1;
  }
};

//////////////////// convertor impl ////////////////////////////////////////////

// NOTICE: this will return a copy of userdata with type T!
template <typename T, typename>
struct luaw::convertor {
  static T to(luaw& l,
              int   idx         = -1,
              bool  disable_log = false,
              bool* failed      = nullptr,
              bool* exists      = nullptr) {
    T* p = luaw::convertor<T*>::to(l, idx, disable_log, failed, exists);
    if (!p) return T{};
    return *p;
  }
};

// convert to all type of pointers including (cv-) void* but not const char*!
template <typename T>
struct luaw::
    convertor<T*, std::enable_if_t<!std::is_same<T*, const char*>::value>> {
  static T* to(luaw& l,
               int   idx         = -1,
               bool  disable_log = false,
               bool* failed      = nullptr,
               bool* exists      = nullptr) {
    if (l.isnoneornil(idx)) {
      if (exists) *exists = false;
      if (failed) *failed = false;
      return nullptr;
    }
    if (exists) *exists = true;
    if (l.isuserdata(idx)) {
      if (failed) *failed = false;
      void* p = lua_touserdata(l.L(), idx);
      return reinterpret_cast<T*>(p);
    }
    if (failed) *failed = true;
    if (!disable_log) { l.log_type_convert_error(idx, "userdata"); }
    return nullptr;
  }
};

// to const char*, NOT RECOMMENDED! Should better use to std::string instead!
// lua_tostring will change number to C string in stack, which is dangerous!
template <>
struct luaw::convertor<const char*> {
  static const char* to(luaw& l,
                        int   idx         = -1,
                        bool  disable_log = false,
                        bool* failed      = nullptr,
                        bool* exists      = nullptr) {
    if (l.isnoneornil(idx)) {
      if (exists) *exists = false;
      if (failed) *failed = false;
      return nullptr;
    }
    if (exists) *exists = true;
    if (l.isstring(idx)) {
      if (failed) *failed = false;
      const char* p = lua_tostring(l.L(), idx);
      return p;
    }
    if (failed) *failed = true;
    if (!disable_log) { l.log_type_convert_error(idx, "const char*"); }
    return nullptr;
  }
};

// to placeholder_tag
template <>
struct luaw::convertor<luaw::placeholder_tag> {
  static luaw::placeholder_tag to(luaw& l,
                                  int   idx         = -1,
                                  bool  disable_log = false,
                                  bool* failed      = nullptr,
                                  bool* exists      = nullptr) {
    if (failed) *failed = false;
    if (exists) *exists = !l.isnoneornil(idx);
    return luaw::placeholder_tag{};
  }
};

template <typename Return, typename... Args>
class luaw::function<Return(Args...)> {
  // component
  lua_State*           L_ = nullptr;
  std::shared_ptr<int> ref_sptr_;
  // parameters put in
  bool disable_log_ = false;
  // states put out
  mutable bool function_failed_ = false, function_exists_ = false,
               result_failed_ = false, result_exists_ = false;

public:
  function(lua_State* L           = nullptr,
           int        idx         = -1,
           bool       disable_log = false,
           bool*      failed      = nullptr,
           bool*      exists      = nullptr)
      : L_(L), disable_log_(disable_log) {
    if (!L) {
      if (failed) *failed = true;
      if (exists) *exists = false;
      return;
    }

    // states about convertion to function,
    // it is states before call the function.
    if (exists) *exists = !lua_isnoneornil(L_, idx);
    // noneornil is not callable, so we regard not-exists as failed for
    // function, which is not as same as usual.
    if (failed) {
      if (lua_isnoneornil(L_, idx)) {
        *failed = true;
      } else {
        // check more whether it's callable
        luaw_fake l(L);
        *failed = !l.callable(idx);
      }
    }

    lua_pushvalue(L_, idx);
    int ref_ = luaL_ref(L, LUA_REGISTRYINDEX);
    ref_sptr_.reset(new int(ref_), [L](int* p) {
      luaL_unref(L, LUA_REGISTRYINDEX, *p);
      delete p;
    });
  }

  // states after function call
  bool function_failed() const { return function_failed_; }
  bool function_exists() const { return function_exists_; }
  bool result_failed() const { return result_failed_; }
  bool result_exists() const { return result_exists_; }
  bool failed() const { return function_failed_ || result_failed_; }

  Return operator()(const Args&... args) const {
    if (!L_) {
      function_failed_ = true;
      function_exists_ = false;
      return Return();
    }

    luaw_fake l(L_);
    auto      _g = l.make_guarder();
    int       sz = l.gettop();
    l.rawgeti(LUA_REGISTRYINDEX, *ref_sptr_);
    if (l.isnoneornil()) {
      function_failed_ = true;
      function_exists_ = false;
      l.pop();
      return Return();
    } else {
      function_exists_ = true;
    }

    int narg      = push_args(l, args...);
    int nret      = luaw::pusher<std::decay_t<Return>>::size;
    int pcall_ret = l.pcall(narg, nret, 0);

    PEACALM_LUAW_ASSERT(l.gettop() >= sz);
    if (pcall_ret == LUA_OK) {
      function_failed_ = false;
    } else {
      function_failed_ = true;
      if (!disable_log_) { l.log_error_in_stack(); }
      l.pop();
      return Return();
    }

    return l.to<Return>(sz + 1, disable_log_, &result_failed_, &result_exists_);
  }

private:
  static int push_args(luaw& l) { return 0; }

  template <typename FirstArg, typename... RestArgs>
  static int push_args(luaw& l, FirstArg&& farg, RestArgs&&... rargs) {
    int x = l.push(std::forward<FirstArg>(farg));
    int y = push_args(l, std::forward<RestArgs>(rargs)...);
    return x + y;
  }
};

// to luaw::function
template <typename Return, typename... Args>
struct luaw::convertor<luaw::function<Return(Args...)>> {
  using result_t = luaw::function<Return(Args...)>;
  static result_t to(luaw& l,
                     int   idx         = -1,
                     bool  disable_log = false,
                     bool* failed      = nullptr,
                     bool* exists      = nullptr) {
    return result_t(l.L(), idx, disable_log, failed, exists);
  }
};

// to std::function
template <typename Return, typename... Args>
struct luaw::convertor<std::function<Return(Args...)>> {
  using result_t = std::function<Return(Args...)>;
  static result_t to(luaw& l,
                     int   idx         = -1,
                     bool  disable_log = false,
                     bool* failed      = nullptr,
                     bool* exists      = nullptr) {
    return result_t(luaw::convertor<luaw::function<Return(Args...)>>::to(
        l, idx, disable_log, failed, exists));
  }
};

// to bool
template <>
struct luaw::convertor<bool> {
  static bool to(luaw& l,
                 int   idx         = -1,
                 bool  disable_log = false,
                 bool* failed      = nullptr,
                 bool* exists      = nullptr) {
    return l.to_bool(idx, bool{}, disable_log, failed, exists);
  }
};

// to integers
template <typename T>
struct luaw::convertor<T, std::enable_if_t<std::is_integral<T>::value>> {
  static T to(luaw& l,
              int   idx         = -1,
              bool  disable_log = false,
              bool* failed      = nullptr,
              bool* exists      = nullptr) {
    auto ret = l.to_llong(idx, 0, disable_log, failed, exists);
    return static_cast<T>(ret);
  }
};

// to float numbers
template <>
struct luaw::convertor<float> {
  static float to(luaw& l,
                  int   idx         = -1,
                  bool  disable_log = false,
                  bool* failed      = nullptr,
                  bool* exists      = nullptr) {
    return l.to_float(idx, float{}, disable_log, failed, exists);
  }
};
template <>
struct luaw::convertor<double> {
  static double to(luaw& l,
                   int   idx         = -1,
                   bool  disable_log = false,
                   bool* failed      = nullptr,
                   bool* exists      = nullptr) {
    return l.to_double(idx, double{}, disable_log, failed, exists);
  }
};
template <>
struct luaw::convertor<long double> {
  static long double to(luaw& l,
                        int   idx         = -1,
                        bool  disable_log = false,
                        bool* failed      = nullptr,
                        bool* exists      = nullptr) {
    return l.to_ldouble(idx, (long double){}, disable_log, failed, exists);
  }
};

// to std::string, safe implementation
// To avoid implicitly modifying number to string in stack, which may cause
// panic while doing lua_next, we copy the value first before call lua_tostring.
template <>
struct luaw::convertor<std::string> {
  static std::string to(luaw& l,
                        int   idx         = -1,
                        bool  disable_log = false,
                        bool* failed      = nullptr,
                        bool* exists      = nullptr) {
    if (exists) *exists = !l.isnoneornil(idx);
    if (l.isstring(idx)) {
      l.pushvalue(idx);  // make a copy, so, it's safe
      std::string ret = lua_tostring(l.L(), -1);
      l.pop();
      if (failed) *failed = false;
      return ret;
    }
    if (l.isnoneornil(idx)) {
      if (failed) *failed = false;
      return "";
    }
    if (failed) *failed = true;
    if (!disable_log) l.log_type_convert_error(idx, "string");
    return "";
  }
};

// to std::pair by (t[1],t[2]) where t should be a table
template <typename T, typename U>
struct luaw::convertor<std::pair<T, U>> {
  using result_t = std::pair<T, U>;
  static result_t to(luaw& l,
                     int   idx         = -1,
                     bool  disable_log = false,
                     bool* failed      = nullptr,
                     bool* exists      = nullptr) {
    if (exists) *exists = !l.isnoneornil(idx);
    if (l.isnoneornil(idx)) {
      if (failed) *failed = false;
      return result_t{};
    }
    if (!l.istable(idx)) {
      if (failed) *failed = true;
      if (!disable_log) l.log_type_convert_error(idx, "pair");
      return result_t{};
    }
    // Allow elements do not exist, {} means a pair with initial values
    bool ffailed, sfailed;
    l.geti(idx, 1);
    auto first = luaw::convertor<T>::to(l, -1, disable_log, &ffailed);
    l.pop();
    l.geti(idx, 2);
    auto second = luaw::convertor<U>::to(l, -1, disable_log, &sfailed);
    l.pop();
    if (failed) *failed = (ffailed || sfailed);
    return result_t{first, second};
  }
};

// to std::vector
// NOTICE: Discard nil in list! e.g. {1,2,nil,4} -> vector<int>{1,2,4}
template <typename T, typename Allocator>
struct luaw::convertor<std::vector<T, Allocator>> {
  using result_t = std::vector<T, Allocator>;
  static result_t to(luaw& l,
                     int   idx         = -1,
                     bool  disable_log = false,
                     bool* failed      = nullptr,
                     bool* exists      = nullptr) {
    if (exists) *exists = !l.isnoneornil(idx);
    if (l.isnoneornil(idx)) {
      if (failed) *failed = false;
      return result_t{};
    }
    if (!l.istable(idx)) {
      if (failed) *failed = true;
      if (!disable_log) l.log_type_convert_error(idx, "vector");
      return result_t{};
    }
    result_t ret;
    if (failed) *failed = false;
    int sz = luaL_len(l.L(), idx);
    ret.reserve(sz);
    for (int i = 1; i <= sz; ++i) {
      l.geti(idx, i);
      bool subfailed, subexists;
      auto subret =
          luaw::convertor<T>::to(l, -1, disable_log, &subfailed, &subexists);
      // Only add elements exist and conversion succeeded
      if (!subfailed && subexists) ret.push_back(std::move(subret));
      if (subfailed && failed) *failed = true;
      l.pop();
    }
    return ret;
  }
};

// to std::set
template <typename Key, typename Compare, typename Allocator>
struct luaw::convertor<std::set<Key, Compare, Allocator>> {
  using result_t = std::set<Key, Compare, Allocator>;
  static result_t to(luaw& l,
                     int   idx         = -1,
                     bool  disable_log = false,
                     bool* failed      = nullptr,
                     bool* exists      = nullptr) {
    auto v =
        l.to<std::vector<Key, Allocator>>(idx, disable_log, failed, exists);
    return result_t(v.begin(), v.end());
  }
};

// to std::unordered_set
template <typename Key, typename Hash, typename KeyEqual, typename Allocator>
struct luaw::convertor<std::unordered_set<Key, Hash, KeyEqual, Allocator>> {
  using result_t = std::unordered_set<Key, Hash, KeyEqual, Allocator>;
  static result_t to(luaw& l,
                     int   idx         = -1,
                     bool  disable_log = false,
                     bool* failed      = nullptr,
                     bool* exists      = nullptr) {
    auto v =
        l.to<std::vector<Key, Allocator>>(idx, disable_log, failed, exists);
    return result_t(v.begin(), v.end());
  }
};

namespace luaw_detail {

template <typename T>
T __tom(luaw&       l,
        int         idx         = -1,
        bool        disable_log = false,
        bool*       failed      = nullptr,
        bool*       exists      = nullptr,
        const char* tname       = "map") {
  static_assert(!std::is_same<typename T::key_type, const char*>::value,
                "const char* as key type of map is forbidden");

  if (exists) *exists = !l.isnoneornil(idx);
  if (l.isnoneornil(idx)) {
    if (failed) *failed = false;
    return T{};
  }
  if (!l.istable(idx)) {
    if (failed) *failed = true;
    if (!disable_log) l.log_type_convert_error(idx, tname);
    return T{};
  }
  T ret;
  if (failed) *failed = false;
  int absidx = l.abs_index(idx);
  l.pushnil();
  while (lua_next(l.L(), absidx) != 0) {
    bool kfailed, kexists, vfailed, vexists;
    auto key = l.to<typename T::key_type>(-2, disable_log, &kfailed, &kexists);
    if (!kfailed && kexists) {
      auto val =
          l.to<typename T::mapped_type>(-1, disable_log, &vfailed, &vexists);
      if (!vfailed && vexists) ret.insert({std::move(key), std::move(val)});
    }
    if ((kfailed || vfailed) && failed) *failed = true;
    l.pop();
  }
  return ret;
}

}  // namespace luaw_detail

// to std::map
template <typename Key, typename Compare, typename Allocator>
struct luaw::convertor<std::map<Key, Compare, Allocator>> {
  using result_t = std::map<Key, Compare, Allocator>;
  static result_t to(luaw& l,
                     int   idx         = -1,
                     bool  disable_log = false,
                     bool* failed      = nullptr,
                     bool* exists      = nullptr) {
    return luaw_detail::__tom<result_t>(
        l, idx, disable_log, failed, exists, "map");
  }
};

// to std::unordered_map
template <typename Key, typename Hash, typename KeyEqual, typename Allocator>
struct luaw::convertor<std::unordered_map<Key, Hash, KeyEqual, Allocator>> {
  using result_t = std::unordered_map<Key, Hash, KeyEqual, Allocator>;
  static result_t to(luaw& l,
                     int   idx         = -1,
                     bool  disable_log = false,
                     bool* failed      = nullptr,
                     bool* exists      = nullptr) {
    return luaw_detail::__tom<result_t>(
        l, idx, disable_log, failed, exists, "unordered_map");
  }
};

// to std::tuple
// The result tuple shoule not contain any tuple any more.
template <typename... Ts>
struct luaw::convertor<std::tuple<Ts...>> {
  using result_t = std::tuple<Ts...>;
  static result_t to(luaw& l,
                     int   idx         = -1,
                     bool  disable_log = false,
                     bool* failed      = nullptr,
                     bool* exists      = nullptr) {
    constexpr size_t N = std::tuple_size<result_t>::value;
    result_t         ret;
    __to<0, N>(ret,
               std::integral_constant<bool, 0 < N>{},
               l,
               l.abs_index(idx),
               disable_log,
               failed,
               exists);
    return ret;
  }

private:
  template <size_t I, size_t N, typename T>
  static void __to(T& ret,
                   std::true_type,
                   luaw& l,
                   int   idx,
                   bool  disable_log = false,
                   bool* failed      = nullptr,
                   bool* exists      = nullptr) {
    static_assert(!luaw_detail::is_stdtuple<std::tuple_element_t<I, T>>::value,
                  "Recursive tuple is not allowed");

    bool thisfailed, thisexists;
    std::get<I>(ret) = l.to<std::tuple_element_t<I, T>>(
        idx, disable_log, &thisfailed, &thisexists);

    bool restfailed, restexists;
    __to<I + 1, N>(ret,
                   std::integral_constant<bool, I + 1 < N>{},
                   l,
                   idx + 1,
                   disable_log,
                   &restfailed,
                   &restexists);

    if (failed) *failed = thisfailed || restfailed;
    if (exists) *exists = thisexists || restexists;
  }

  template <size_t I, size_t N, typename T>
  static void __to(T& ret,
                   std::false_type,
                   luaw& l,
                   int   idx         = -1,
                   bool  disable_log = false,
                   bool* failed      = nullptr,
                   bool* exists      = nullptr) {
    if (failed) *failed = false;
    if (exists) *exists = false;
  }
};

// to void
template <>
struct luaw::convertor<void> {
  static void to(luaw& l,
                 int   idx         = -1,
                 bool  disable_log = false,
                 bool* failed      = nullptr,
                 bool* exists      = nullptr) {
    if (failed) *failed = false;
    if (exists) *exists = !l.isnoneornil(idx);
    return void();
  }
};

// to luaw::luavalueidx
template <>
struct luaw::convertor<luaw::luavalueidx> {
  static luaw::luavalueidx to(luaw& l,
                              int   idx         = -1,
                              bool  disable_log = false,
                              bool* failed      = nullptr,
                              bool* exists      = nullptr) {
    if (failed) *failed = false;
    if (exists) *exists = !l.isnone(idx);  // only none as not exists
    return luaw::luavalueidx(l.L(), idx);
  }
};

// to luaw::luavalueref
template <>
struct luaw::convertor<luaw::luavalueref> {
  static luaw::luavalueref to(luaw& l,
                              int   idx         = -1,
                              bool  disable_log = false,
                              bool* failed      = nullptr,
                              bool* exists      = nullptr) {
    if (failed) *failed = false;
    if (exists) *exists = !l.isnone(idx);  // only none as not exists
    l.pushvalue(idx);                      // make a copy
    return std::move(luaw::luavalueref(l.L()));
  }
};

//////////////////// metatable_factory impl ////////////////////////////////////

namespace luaw_detail {

template <typename T, typename Derived>
struct metatable_factory_base {
  static void push_shared_metatable(luaw& l) {
    bool first_create = l.gtouchmetatb(typeid(T).name());
    if (first_create) Derived::set_metamethods(l);
  }

  static void push_exclusive_metatable(luaw& l) {
    l.newtable();
    Derived::set_metamethods(l);
  }
};

}  // namespace luaw_detail

// T: The (class) type whose member is registered.
template <typename T>
struct luaw::metatable_factory<T*>
    : public luaw_detail::metatable_factory_base<T*,
                                                 luaw::metatable_factory<T*>> {
  // No restrict to class only anymore, user can use it if he knows what he is
  // doing.
  // static_assert(std::is_class<T>::value,
  //               "Only class and it's pointer could have metatable");

  static void set_metamethods(luaw& l) {
    l.setkv("__index", __index, -1);
    l.setkv("__newindex", __newindex, -1);
  }

  static int __index(lua_State* L) {
    luaw_fake l(L);
    PEACALM_LUAW_ASSERT(l.gettop() == 2);
    void* ti =
        reinterpret_cast<void*>(const_cast<std::type_info*>(&typeid(T*)));
    l.pushlightuserdata(ti);
    l.gettable(LUA_REGISTRYINDEX);

    if (!l.istable(-1)) {
      l.pop();
      l.pushnil();
      return 1;
    }

    // member function
    l.rawgeti(-1, luaw::member_info_fields::member_function);
    if (!l.istable(-1)) {
      l.pop();
    } else {
      l.pushvalue(2);  // push the key
      l.rawget(-2);
      if (!l.isnil(-1)) {  // found
        return 1;
      } else {
        l.pop(2);
      }
    }

    // member variable getter
    l.rawgeti(-1, luaw::member_info_fields::member_getter);
    if (!l.istable(-1)) {
      l.pop();
    } else {
      l.pushvalue(2);  // push the key
      l.rawget(-2);
      if (!l.isnil(-1)) {  // found
        l.pushvalue(1);    // push the userdata
        int retcode = l.pcall(1, 1, 0);
        if (retcode == LUA_OK) {
          return 1;
        } else {
          return lua_error(l.L());  // getter failed
        }
      } else {
        l.pop(2);
      }
    }

    // whether calling nonconst member function by a const object
    if (std::is_const<T>::value ||
        ((luaw_detail::is_std_shared_ptr<T>::value ||
          luaw_detail::is_std_unique_ptr<T>::value) &&
         std::is_const<
             typename luaw_detail::get_element_type<T>::type>::value)) {
      l.rawgeti(-1, luaw::member_info_fields::nonconst_member_function);
      if (!l.istable(-1)) {
        l.pop();
      } else {
        l.pushvalue(2);  // push the key
        l.rawget(-2);
        if (!l.isnil(-1)) {  // found, which is true
          const char* key = l.to_c_str(2);
          return luaL_error(l.L(), "Nonconst member function: %s", key);
        } else {
          l.pop(2);
        }
      }
    }

    // whether calling nonvolatile member function by a volatile object
    if (std::is_volatile<T>::value ||
        ((luaw_detail::is_std_shared_ptr<T>::value ||
          luaw_detail::is_std_unique_ptr<T>::value) &&
         std::is_volatile<
             typename luaw_detail::get_element_type<T>::type>::value)) {
      l.rawgeti(-1, luaw::member_info_fields::nonvolatile_member_function);
      if (!l.istable(-1)) {
        l.pop();
      } else {
        l.pushvalue(2);  // push the key
        l.rawget(-2);
        if (!l.isnil(-1)) {  // found, which is true
          const char* key = l.to_c_str(2);
          return luaL_error(l.L(), "Nonvolatile member function: %s", key);
        } else {
          l.pop(2);
        }
      }
    }

    // generic member getter
    l.rawgeti(-1, luaw::member_info_fields::generic_member_getter);
    if (l.isnil(-1)) {
      l.pop();
    } else {
      l.pushvalue(1);  // push the userdata
      l.pushvalue(2);  // push the key
      int retcode = l.pcall(2, 1, 0);
      if (retcode == LUA_OK) {
        return 1;
      } else {
        return lua_error(l.L());
      }
    }

    // not found handler
    l.pushnil();
    return 1;
  }

  static int __newindex(lua_State* L) {
    luaw_fake l(L);
    PEACALM_LUAW_ASSERT(l.gettop() == 3);
    void* ti =
        reinterpret_cast<void*>(const_cast<std::type_info*>(&typeid(T*)));
    l.pushlightuserdata(ti);
    l.gettable(LUA_REGISTRYINDEX);

    if (!l.istable(-1)) {
      l.pop();
      const char* key = l.to_c_str(2);
      return luaL_error(l.L(), "Not found setter: %s", key);
    }

    // member variable setter
    l.rawgeti(-1, luaw::member_info_fields::member_setter);
    if (!l.istable(-1)) {
      l.pop();
    } else {
      l.pushvalue(2);  // push the key
      l.rawget(-2);
      if (!l.isnil(-1)) {  // found
        l.pushvalue(1);    // push the userdata
        l.pushvalue(3);    // push the value
        int retcode = l.pcall(2, 0, 0);
        if (retcode == LUA_OK) {
          return 0;
        } else {
          return lua_error(l.L());  // failed
        }
      } else {
        l.pop(2);
      }
    }

    // const members
    l.rawgeti(-1, luaw::member_info_fields::const_member);
    if (!l.istable(-1)) {
      l.pop();
    } else {
      l.pushvalue(2);  // push the key
      l.rawget(-2);
      if (!l.isnil()) {  // found, more it is a bool value true
        const char* key = l.to_c_str(2);
        return luaL_error(l.L(), "Const member cannot be changed: %s", key);
      }
    }

    // generic member setter
    l.rawgeti(-1, luaw::member_info_fields::generic_member_setter);
    if (l.isnil(-1)) {
      l.pop();
    } else if (l.isboolean(-1)) {  // which is false
      return luaL_error(l.L(), "Cannot set member to const object");
    } else {
      l.pushvalue(1);  // push the userdata
      l.pushvalue(2);  // push the key
      l.pushvalue(3);  // push the value
      int retcode = l.pcall(3, 0, 0);
      if (retcode == LUA_OK) {
        return 0;
      } else {
        return lua_error(l.L());  // failed
      }
    }

    // not found handler
    const char* key = l.to_c_str(2);
    return luaL_error(l.L(), "Not found setter: %s", key);
  }
};

template <typename T, typename>
struct luaw::metatable_factory
    : public luaw_detail::metatable_factory_base<T,
                                                 luaw::metatable_factory<T>> {
  // No restrict to class only anymore, user can use it if he knows what he is
  // doing.
  // static_assert(std::is_class<T>::value,
  //               "Only class and it's pointer could have metatable");

  static void set_metamethods(luaw& l) {
    luaw::metatable_factory<T*>::set_metamethods(l);
    if (!std::is_trivially_destructible<T>::value) { l.setkv("__gc", __gc); }
  }

  static int __gc(lua_State* L) {
    luaw_fake l(L);
    PEACALM_LUAW_ASSERT(l.gettop() == 1);
    T* p = l.to<T*>(1);
    PEACALM_LUAW_ASSERT(p);
    p->~T();
    return 0;
  }
};

//////////////////// register ctor impl ////////////////////////////////////////

template <typename Return, typename... Args>
struct luaw::register_ctor_impl<Return (*)(Args...)> {
  static void register_ctor(luaw& l, const char* fname) {
    auto ctor = [](const Args&... args) -> Return { return Return(args...); };
    l.set<luaw::function_tag>(fname, ctor);
  }
};

//////////////////// mock_mem_fn impl //////////////////////////////////////////

namespace luaw_detail {

template <typename CallableObject, typename>
class mock_mem_fn_impl {
  static_assert(luaw_detail::decay_is_callable<CallableObject>::value,
                "The object to mock member function must be callable");
  // never happends here
};

// The first argument is pointer
template <typename CallableObject,
          typename Return,
          typename FirstArg,
          typename... Args>
class mock_mem_fn_impl<CallableObject, Return (*)(FirstArg*, Args...)> {
  static_assert(luaw_detail::decay_is_callable<CallableObject>::value,
                "The object to mock member function must be callable");

  static_assert(std::is_class<FirstArg>::value,
                "FirstArg should be pointer of Class whose members registered");

  using DecayFirstArg = std::decay_t<FirstArg>;

  CallableObject o;

public:
  mock_mem_fn_impl(CallableObject&& r) : o(std::forward<CallableObject>(r)) {}

  // reference

  template <typename... Ys>
  Return operator()(FirstArg& f, Ys&&... ys) const {
    return o(&f, std::forward<Ys>(ys)...);
  }

  // raw pointer

  template <typename... Ys>
  Return operator()(FirstArg* f, Ys&&... ys) const {
    return o(f, std::forward<Ys>(ys)...);
  }

  // std::shared_ptr

  template <typename... Ys>
  Return operator()(const std::shared_ptr<DecayFirstArg>& f, Ys&&... ys) const {
    PEACALM_LUAW_ASSERT(f.get());
    return o(f.get(), std::forward<Ys>(ys)...);
  }

  template <typename... Ys>
  Return operator()(const std::shared_ptr<const DecayFirstArg>& f,
                    Ys&&... ys) const {
    PEACALM_LUAW_ASSERT(f.get());
    return o(f.get(), std::forward<Ys>(ys)...);
  }

  template <typename... Ys>
  Return operator()(const std::shared_ptr<volatile DecayFirstArg>& f,
                    Ys&&... ys) const {
    PEACALM_LUAW_ASSERT(f.get());
    return o(f.get(), std::forward<Ys>(ys)...);
  }

  template <typename... Ys>
  Return operator()(const std::shared_ptr<const volatile DecayFirstArg>& f,
                    Ys&&... ys) const {
    PEACALM_LUAW_ASSERT(f.get());
    return o(f.get(), std::forward<Ys>(ys)...);
  }

  // std::unique_ptr

  template <typename D, typename... Ys>
  Return operator()(const std::unique_ptr<DecayFirstArg, D>& f,
                    Ys&&... ys) const {
    PEACALM_LUAW_ASSERT(f.get());
    return o(f.get(), std::forward<Ys>(ys)...);
  }

  template <typename D, typename... Ys>
  Return operator()(const std::unique_ptr<const DecayFirstArg, D>& f,
                    Ys&&... ys) const {
    PEACALM_LUAW_ASSERT(f.get());
    return o(f.get(), std::forward<Ys>(ys)...);
  }

  template <typename D, typename... Ys>
  Return operator()(const std::unique_ptr<volatile DecayFirstArg, D>& f,
                    Ys&&... ys) const {
    PEACALM_LUAW_ASSERT(f.get());
    return o(f.get(), std::forward<Ys>(ys)...);
  }

  template <typename D, typename... Ys>
  Return operator()(const std::unique_ptr<const volatile DecayFirstArg, D>& f,
                    Ys&&... ys) const {
    PEACALM_LUAW_ASSERT(f.get());
    return o(f.get(), std::forward<Ys>(ys)...);
  }
};

// The first argument is reference
template <typename CallableObject,
          typename Return,
          typename FirstArg,
          typename... Args>
class mock_mem_fn_impl<CallableObject, Return (*)(FirstArg&, Args...)> {
  static_assert(luaw_detail::decay_is_callable<CallableObject>::value,
                "The object to mock member function must be callable");

  // The FirstArg is the Class type whose members registered.
  static_assert(
      std::is_class<FirstArg>::value,
      "FirstArg should be reference of Class whose members registered");

  using DecayFirstArg = std::decay_t<FirstArg>;

  CallableObject o;

public:
  mock_mem_fn_impl(CallableObject&& r) : o(std::forward<CallableObject>(r)) {}

  // reference

  template <typename... Ys>
  Return operator()(FirstArg& f, Ys&&... ys) const {
    return o(f, std::forward<Ys>(ys)...);
  }

  // raw pointer

  template <typename... Ys>
  Return operator()(FirstArg* f, Ys&&... ys) const {
    return o(*f, std::forward<Ys>(ys)...);
  }

  // shared_ptr

  template <typename... Ys>
  Return operator()(const std::shared_ptr<DecayFirstArg>& f, Ys&&... ys) const {
    PEACALM_LUAW_ASSERT(f.get());
    return o(*f.get(), std::forward<Ys>(ys)...);
  }

  template <typename... Ys>
  Return operator()(const std::shared_ptr<const DecayFirstArg>& f,
                    Ys&&... ys) const {
    PEACALM_LUAW_ASSERT(f.get());
    return o(*f.get(), std::forward<Ys>(ys)...);
  }

  template <typename... Ys>
  Return operator()(const std::shared_ptr<volatile DecayFirstArg>& f,
                    Ys&&... ys) const {
    PEACALM_LUAW_ASSERT(f.get());
    return o(*f.get(), std::forward<Ys>(ys)...);
  }

  template <typename... Ys>
  Return operator()(const std::shared_ptr<const volatile DecayFirstArg>& f,
                    Ys&&... ys) const {
    PEACALM_LUAW_ASSERT(f.get());
    return o(*f.get(), std::forward<Ys>(ys)...);
  }

  // unique_ptr

  template <typename D, typename... Ys>
  Return operator()(const std::unique_ptr<DecayFirstArg, D>& f,
                    Ys&&... ys) const {
    PEACALM_LUAW_ASSERT(f.get());
    return o(*f.get(), std::forward<Ys>(ys)...);
  }

  template <typename D, typename... Ys>
  Return operator()(const std::unique_ptr<const DecayFirstArg, D>& f,
                    Ys&&... ys) const {
    PEACALM_LUAW_ASSERT(f.get());
    return o(*f.get(), std::forward<Ys>(ys)...);
  }

  template <typename D, typename... Ys>
  Return operator()(const std::unique_ptr<volatile DecayFirstArg, D>& f,
                    Ys&&... ys) const {
    PEACALM_LUAW_ASSERT(f.get());
    return o(*f.get(), std::forward<Ys>(ys)...);
  }

  template <typename D, typename... Ys>
  Return operator()(const std::unique_ptr<const volatile DecayFirstArg, D>& f,
                    Ys&&... ys) const {
    PEACALM_LUAW_ASSERT(f.get());
    return o(*f.get(), std::forward<Ys>(ys)...);
  }
};

}  // namespace luaw_detail

template <typename CallableObject>
struct luaw::mock_mem_fn
    : public luaw_detail::mock_mem_fn_impl<
          CallableObject,
          luaw_detail::detect_callable_cfunction_t<CallableObject>> {
private:
  using base_t = luaw_detail::mock_mem_fn_impl<
      CallableObject,
      luaw_detail::detect_callable_cfunction_t<CallableObject>>;

public:
  mock_mem_fn(CallableObject&& r) : base_t(std::forward<CallableObject>(r)) {}
};

//////////////////// registrar impl ////////////////////////////////////////////

template <typename T, typename>
struct luaw::registrar {
  // Let compile fail for unsupported member types,
  // such as ref- qualified member functions.
  // Supported member types should be specialized elsewhere.
  static_assert(!std::is_member_pointer<T>::value, "Unsupported member type");
  static_assert(std::is_same<T, std::decay_t<T>>::value, "T should be decayed");

  template <typename MemberFunction>
  static void register_member(luaw&          l,
                              const char*    fname,
                              MemberFunction mf) = delete;

  // generic members

  template <typename Getter>
  static void register_generic_member_getter(luaw& l, Getter&& getter) {
    static_assert(luaw_detail::decay_is_callable<Getter>::value,
                  "Getter should be callable");
    using CFPtr = luaw_detail::detect_callable_cfunction_t<Getter>;
    static_assert(__check_getter_type(CFPtr{}).value, "Invalid getter type");

    auto _g = l.make_guarder();
    l.touchtb((void*)(&typeid(T*)), LUA_REGISTRYINDEX)
        .setkv<luaw::function_tag>(
            luaw::member_info_fields::generic_member_getter, getter);
    l.touchtb((void*)(&typeid(const T*)), LUA_REGISTRYINDEX)
        .setkv<luaw::function_tag>(
            luaw::member_info_fields::generic_member_getter, getter);
    l.touchtb((void*)(&typeid(volatile T*)), LUA_REGISTRYINDEX)
        .setkv<luaw::function_tag>(
            luaw::member_info_fields::generic_member_getter, getter);
    l.touchtb((void*)(&typeid(const volatile T*)), LUA_REGISTRYINDEX)
        .setkv<luaw::function_tag>(
            luaw::member_info_fields::generic_member_getter, getter);
  }

  // Indicate no generic member getter to register.
  // Or unregister generic member getter.
  static void register_generic_member_getter(luaw& l, std::nullptr_t) {
    auto _g = l.make_guarder();
    l.touchtb((void*)(&typeid(T*)), LUA_REGISTRYINDEX)
        .setkv(luaw::member_info_fields::generic_member_getter, nullptr);
    l.touchtb((void*)(&typeid(const T*)), LUA_REGISTRYINDEX)
        .setkv(luaw::member_info_fields::generic_member_getter, nullptr);
    l.touchtb((void*)(&typeid(volatile T*)), LUA_REGISTRYINDEX)
        .setkv(luaw::member_info_fields::generic_member_getter, nullptr);
    l.touchtb((void*)(&typeid(const volatile T*)), LUA_REGISTRYINDEX)
        .setkv(luaw::member_info_fields::generic_member_getter, nullptr);
  }

  template <typename Setter>
  static void register_generic_member_setter(luaw& l, Setter&& setter) {
    static_assert(luaw_detail::decay_is_callable<Setter>::value,
                  "Setter should be callable");
    using CFPtr = luaw_detail::detect_callable_cfunction_t<Setter>;
    static_assert(__check_setter_type(CFPtr{}).value, "Invalid setter type");

    auto _g = l.make_guarder();
    l.touchtb((void*)(&typeid(T*)), LUA_REGISTRYINDEX)
        .setkv<luaw::function_tag>(
            luaw::member_info_fields::generic_member_setter, setter);
    l.touchtb((void*)(&typeid(volatile T*)), LUA_REGISTRYINDEX)
        .setkv<luaw::function_tag>(
            luaw::member_info_fields::generic_member_setter, setter);

    // const objects, set value false!
    l.touchtb((void*)(&typeid(const T*)), LUA_REGISTRYINDEX)
        .setkv(luaw::member_info_fields::generic_member_setter, false);
    l.touchtb((void*)(&typeid(const volatile T*)), LUA_REGISTRYINDEX)
        .setkv(luaw::member_info_fields::generic_member_setter, false);
  }

  // Indicate no generic member setter to register.
  // Or unregister generic member setter.
  static void register_generic_member_setter(luaw& l, std::nullptr_t) {
    auto _g = l.make_guarder();
    l.touchtb((void*)(&typeid(T*)), LUA_REGISTRYINDEX)
        .setkv(luaw::member_info_fields::generic_member_setter, nullptr);
    l.touchtb((void*)(&typeid(volatile T*)), LUA_REGISTRYINDEX)
        .setkv(luaw::member_info_fields::generic_member_setter, nullptr);

    l.touchtb((void*)(&typeid(const T*)), LUA_REGISTRYINDEX)
        .setkv(luaw::member_info_fields::generic_member_setter, nullptr);
    l.touchtb((void*)(&typeid(const volatile T*)), LUA_REGISTRYINDEX)
        .setkv(luaw::member_info_fields::generic_member_setter, nullptr);
  }

private:
  // do nothing but a static type check

  template <typename Class, typename Key, typename Value>
  static constexpr std::true_type __check_getter_type(Value (*)(Class*, Key)) {
    static_assert(std::is_const<Class>::value,
                  "First parameter of getter should be const 'const Class*'");
    static_assert(std::is_same<std::remove_cv_t<Class>, T>::value,
                  "First parameter of getter should be pointer to the template "
                  "parameter 'Class'");
    return std::true_type{};
  }

  static constexpr std::false_type __check_getter_type(...) {
    return std::false_type{};
  }

  template <typename Class, typename Key, typename Value>
  static constexpr std::true_type __check_setter_type(void (*)(Class*,
                                                               Key,
                                                               Value)) {
    static_assert(!std::is_const<Class>::value,
                  "First parameter of setter should not be const");
    static_assert(std::is_same<std::remove_cv_t<Class>, T>::value,
                  "First parameter of setter should be pointer to the template "
                  "parameter 'Class'");
    return std::true_type{};
  }

  static constexpr std::false_type __check_setter_type(...) {
    return std::false_type{};
  }
};

// register member variable
template <typename Class, typename Member>
struct luaw::registrar<
    Member Class::*,
    std::enable_if_t<std::is_member_object_pointer<Member Class::*>::value>> {
  // register a specific member
  template <typename F>
  static void register_member(luaw& l, const char* mname, F&& f) {
#define DEFINE_GETTER(ObjectType)                          \
  {                                                        \
    auto getter = [=](ObjectType o) -> Member {            \
      PEACALM_LUAW_ASSERT(o);                              \
      return f(*o);                                        \
    };                                                     \
    void* p = reinterpret_cast<void*>(                     \
        const_cast<std::type_info*>(&typeid(ObjectType))); \
    l.touchtb(p, LUA_REGISTRYINDEX)                        \
        .touchtb(luaw::member_info_fields::member_getter)  \
        .setkv<luaw::function_tag>(mname, getter);         \
    l.pop(2);                                              \
  }

    {
      DEFINE_GETTER(Class*);
      DEFINE_GETTER(const Class*);
      DEFINE_GETTER(volatile Class*);
      DEFINE_GETTER(const volatile Class*);
    }
    // Currently DO NOT support volatile for smart pointers!
    if (!luaw_detail::is_std_shared_ptr<Class>::value) {
      DEFINE_GETTER(std::shared_ptr<Class>*);
      DEFINE_GETTER(std::shared_ptr<const Class>*);
      DEFINE_GETTER(const std::shared_ptr<Class>*);
      DEFINE_GETTER(const std::shared_ptr<const Class>*);
    }
    if (!luaw_detail::is_std_unique_ptr<Class>::value) {
      DEFINE_GETTER(std::unique_ptr<Class>*);
      DEFINE_GETTER(std::unique_ptr<const Class>*);
      DEFINE_GETTER(const std::unique_ptr<Class>*);
      DEFINE_GETTER(const std::unique_ptr<const Class>*);
    }
#undef DEFINE_GETTER

    __register_setters(l, mname, std::forward<F>(f), std::is_const<Member>{});
  }

private:
  template <typename ObjectType>
  static void __register_const_member(luaw& l, const char* mname) {
    auto  _g = l.make_guarder();
    void* p  = reinterpret_cast<void*>(
        const_cast<std::type_info*>(&typeid(ObjectType)));
    l.touchtb(p, LUA_REGISTRYINDEX)
        .touchtb(luaw::member_info_fields::const_member)
        .setkv(mname, true);
  }

  template <typename F>
  static void __register_setters(luaw&       l,
                                 const char* mname,
                                 F&&         f,
                                 std::true_type) {
    {
      __register_const_member<Class*>(l, mname);
      __register_const_member<const Class*>(l, mname);
      __register_const_member<volatile Class*>(l, mname);
      __register_const_member<const volatile Class*>(l, mname);
    }
    if (!luaw_detail::is_std_shared_ptr<Class>::value) {
      __register_const_member<std::shared_ptr<Class>*>(l, mname);
      __register_const_member<std::shared_ptr<const Class>*>(l, mname);
      __register_const_member<const std::shared_ptr<Class>*>(l, mname);
      __register_const_member<const std::shared_ptr<const Class>*>(l, mname);
    }
    if (!luaw_detail::is_std_unique_ptr<Class>::value) {
      __register_const_member<std::unique_ptr<Class>*>(l, mname);
      __register_const_member<std::unique_ptr<const Class>*>(l, mname);
      __register_const_member<const std::unique_ptr<Class>*>(l, mname);
      __register_const_member<const std::unique_ptr<const Class>*>(l, mname);
    }
  }

  template <typename F>
  static void __register_setters(luaw&       l,
                                 const char* mname,
                                 F&&         f,
                                 std::false_type) {
#define DEFINE_SETTER(ObjectType)                          \
  {                                                        \
    auto setter = [=](ObjectType o, Member v) {            \
      PEACALM_LUAW_ASSERT(o);                              \
      f(*o) = std::move(v);                                \
    };                                                     \
    void* p = reinterpret_cast<void*>(                     \
        const_cast<std::type_info*>(&typeid(ObjectType))); \
    l.touchtb(p, LUA_REGISTRYINDEX)                        \
        .touchtb(luaw::member_info_fields::member_setter)  \
        .setkv<luaw::function_tag>(mname, setter);         \
    l.pop(2);                                              \
  }

    {
      DEFINE_SETTER(Class*);
      DEFINE_SETTER(volatile Class*);
      // the object is const, so member is const
      __register_const_member<const Class*>(l, mname);
      __register_const_member<const volatile Class*>(l, mname);
    }
    if (!luaw_detail::is_std_shared_ptr<Class>::value) {
      DEFINE_SETTER(std::shared_ptr<Class>*);
      DEFINE_SETTER(const std::shared_ptr<Class>*);
      __register_const_member<std::shared_ptr<const Class>*>(l, mname);
      __register_const_member<const std::shared_ptr<const Class>*>(l, mname);
    }
    if (!luaw_detail::is_std_unique_ptr<Class>::value) {
      DEFINE_SETTER(std::unique_ptr<Class>*);
      DEFINE_SETTER(const std::unique_ptr<Class>*);
      __register_const_member<std::unique_ptr<const Class>*>(l, mname);
      __register_const_member<const std::unique_ptr<const Class>*>(l, mname);
    }

#undef DEFINE_SETTER
  }
};

// register member functions
// @{

template <typename Class, typename Return, typename... Args>
struct luaw::registrar<Return (Class::*)(Args...)> {
  // implementations

  template <typename ObjectType, typename MemberFunction>
  static void register_member_function(luaw&            l,
                                       const char*      fname,
                                       MemberFunction&& mf) {
    auto f = [=](ObjectType o, Args... args) -> Return {
      PEACALM_LUAW_ASSERT(o);
      return mf(*o, std::move(args)...);
    };

    void* p = reinterpret_cast<void*>(
        const_cast<std::type_info*>(&typeid(ObjectType)));
    l.touchtb(p, LUA_REGISTRYINDEX)
        .touchtb(luaw::member_info_fields::member_function)
        .setkv<luaw::function_tag>(fname, f);
    l.pop(2);
  }

  template <typename ObjectType>
  static void register_nonconst_member_function(luaw& l, const char* fname) {
    void* p = reinterpret_cast<void*>(
        const_cast<std::type_info*>(&typeid(ObjectType)));
    l.touchtb(p, LUA_REGISTRYINDEX)
        .touchtb(luaw::member_info_fields::nonconst_member_function)
        .setkv(fname, true);
    l.pop(2);
  }

  template <typename ObjectType>
  static void register_nonvolatile_member_function(luaw& l, const char* fname) {
    void* p = reinterpret_cast<void*>(
        const_cast<std::type_info*>(&typeid(ObjectType)));
    l.touchtb(p, LUA_REGISTRYINDEX)
        .touchtb(luaw::member_info_fields::nonvolatile_member_function)
        .setkv(fname, true);
    l.pop(2);
  }

  // no cv- member functions
  template <typename MemberFunction>
  static void register_member(luaw& l, const char* fname, MemberFunction mf) {
    {
      register_member_function<Class*>(l, fname, mf);
      register_nonconst_member_function<const Class*>(l, fname);
      register_nonconst_member_function<const volatile Class*>(l, fname);
      register_nonvolatile_member_function<volatile Class*>(l, fname);
      register_nonvolatile_member_function<const volatile Class*>(l, fname);
    }
    if (!luaw_detail::is_std_shared_ptr<Class>::value) {
      register_member_function<std::shared_ptr<Class>*>(l, fname, mf);
      register_member_function<const std::shared_ptr<Class>*>(l, fname, mf);

      register_nonconst_member_function<std::shared_ptr<const Class>*>(l,
                                                                       fname);
      register_nonconst_member_function<const std::shared_ptr<const Class>*>(
          l, fname);
    }
    if (!luaw_detail::is_std_unique_ptr<Class>::value) {
      register_member_function<std::unique_ptr<Class>*>(l, fname, mf);
      register_member_function<const std::unique_ptr<Class>*>(l, fname, mf);

      register_nonconst_member_function<std::unique_ptr<const Class>*>(l,
                                                                       fname);
      register_nonconst_member_function<const std::unique_ptr<const Class>*>(
          l, fname);
    }
  }
};

template <typename Class, typename Return, typename... Args>
struct luaw::registrar<Return (Class::*)(Args...) const> {
  template <typename MemberFunction>
  static void register_member(luaw& l, const char* fname, MemberFunction mf) {
    using Basic = luaw::registrar<Return (Class::*)(Args...)>;
    {
      Basic::template register_member_function<Class*>(l, fname, mf);
      Basic::template register_member_function<const Class*>(l, fname, mf);
      Basic::template register_nonvolatile_member_function<volatile Class*>(
          l, fname);
      Basic::template register_nonvolatile_member_function<
          const volatile Class*>(l, fname);
    }
    if (!luaw_detail::is_std_shared_ptr<Class>::value) {
      Basic::template register_member_function<std::shared_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<const std::shared_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<std::shared_ptr<const Class>*>(
          l, fname, mf);
      Basic::template register_member_function<
          const std::shared_ptr<const Class>*>(l, fname, mf);
    }
    if (!luaw_detail::is_std_unique_ptr<Class>::value) {
      Basic::template register_member_function<std::unique_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<const std::unique_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<std::unique_ptr<const Class>*>(
          l, fname, mf);
      Basic::template register_member_function<
          const std::unique_ptr<const Class>*>(l, fname, mf);
    }
  }
};

template <typename Class, typename Return, typename... Args>
struct luaw::registrar<Return (Class::*)(Args...) volatile> {
  template <typename MemberFunction>
  static void register_member(luaw& l, const char* fname, MemberFunction mf) {
    using Basic = luaw::registrar<Return (Class::*)(Args...)>;
    {
      Basic::template register_member_function<Class*>(l, fname, mf);
      Basic::template register_member_function<volatile Class*>(l, fname, mf);
      Basic::template register_nonconst_member_function<const Class*>(l, fname);
      Basic::template register_nonconst_member_function<const volatile Class*>(
          l, fname);
    }
    if (!luaw_detail::is_std_shared_ptr<Class>::value) {
      Basic::template register_member_function<std::shared_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<const std::shared_ptr<Class>*>(
          l, fname, mf);

      Basic::template register_nonconst_member_function<
          std::shared_ptr<const Class>*>(l, fname);
      Basic::template register_nonconst_member_function<
          const std::shared_ptr<const Class>*>(l, fname);
    }
    if (!luaw_detail::is_std_unique_ptr<Class>::value) {
      Basic::template register_member_function<std::unique_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<const std::unique_ptr<Class>*>(
          l, fname, mf);

      Basic::template register_nonconst_member_function<
          std::unique_ptr<const Class>*>(l, fname);
      Basic::template register_nonconst_member_function<
          const std::unique_ptr<const Class>*>(l, fname);
    }
  }
};

template <typename Class, typename Return, typename... Args>
struct luaw::registrar<Return (Class::*)(Args...) const volatile> {
  template <typename MemberFunction>
  static void register_member(luaw& l, const char* fname, MemberFunction mf) {
    using Basic = luaw::registrar<Return (Class::*)(Args...)>;
    {
      Basic::template register_member_function<Class*>(l, fname, mf);
      Basic::template register_member_function<const Class*>(l, fname, mf);
      Basic::template register_member_function<volatile Class*>(l, fname, mf);
      Basic::template register_member_function<const volatile Class*>(
          l, fname, mf);
    }
    if (!luaw_detail::is_std_shared_ptr<Class>::value) {
      Basic::template register_member_function<std::shared_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<const std::shared_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<std::shared_ptr<const Class>*>(
          l, fname, mf);
      Basic::template register_member_function<
          const std::shared_ptr<const Class>*>(l, fname, mf);
    }
    if (!luaw_detail::is_std_unique_ptr<Class>::value) {
      Basic::template register_member_function<std::unique_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<const std::unique_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<std::unique_ptr<const Class>*>(
          l, fname, mf);
      Basic::template register_member_function<
          const std::unique_ptr<const Class>*>(l, fname, mf);
    }
  }
};

// @}

////////////////////////////////////////////////////////////////////////////////

namespace luaw_detail {
template <typename T>
struct __is_ptr : std::false_type {};
template <typename T>
struct __is_ptr<T*> : std::true_type {};
template <typename T>
struct __is_ptr<std::shared_ptr<T>> : std::true_type {};
template <typename T, typename D>
struct __is_ptr<std::unique_ptr<T, D>> : std::true_type {};
template <typename T>
struct is_ptr : __is_ptr<typename std::decay<T>::type> {};
}  // namespace luaw_detail

/**
 * @brief A Lua wrapper with custom variable provider.
 *
 * Derived from luaw, it can contain a user defined variable provider.
 * When a global variable used in some expression does not exist in Lua,
 * then it will seek the variable from the provider.
 *
 * The underlying provider type should implement a member function:
 *
 * * `bool provide(peacalm::luaw* l, const char* vname);`
 *
 * In this member function, it should push exactly one value whose name is
 * vname onto the stack of L then return true. Otherwise return false if vname
 * is illegal or vname doesn't have a correct value.
 *
 * @tparam VariableProviderPointer Should be a raw pointer type or
 * std::shared_ptr or std::unique_ptr.
 */
template <typename VariableProviderPointer>
class custom_luaw : public luaw {
  using base_t     = luaw;
  using provider_t = VariableProviderPointer;
  static_assert(luaw_detail::is_ptr<provider_t>::value,
                "VariableProviderPointer should be pointer type");
  using pointer_t      = custom_luaw*;
  provider_t provider_ = nullptr;

public:
  template <typename... Args>
  custom_luaw(Args&&... args) : base_t(std::forward<Args>(args)...) {
    set_globale_metateble();
  }

  // delete copy
  custom_luaw(const custom_luaw&)            = delete;
  custom_luaw& operator=(const custom_luaw&) = delete;

  // support move
  custom_luaw(custom_luaw&& l)
      : base_t(std::move(l)), provider_(std::move(l.provider_)) {
    set_globale_metateble();
  }
  custom_luaw& operator=(custom_luaw&& r) {
    base_t::operator=(std::move(r));
    provider_ = std::move(r.provider_);
    set_globale_metateble();
    return *this;
  }

  // provider getter and setter
  void              provider(const provider_t& p) { provider_ = p; }
  void              provider(provider_t&& p) { provider_ = std::move(p); }
  const provider_t& provider() const { return provider_; }
  provider_t&       provider() { return provider_; }

private:
  bool provide(lua_State* L, const char* var_name) {
    luaw_fake l(L);
    return provider() && provider()->provide(&l, var_name);
  }

  void set_globale_metateble() {
    pushglobaltable();
    if (!getmetatable(-1)) { newtable(); }
    pushcfunction(_G__index);
    setfield(-2, "__index");
    setmetatable(-2);
    pop();
    pushlightuserdata((void*)this);
    setfield(LUA_REGISTRYINDEX, "this");
  }

  static int _G__index(lua_State* L) {
    const char* name = lua_tostring(L, 2);
    lua_getfield(L, LUA_REGISTRYINDEX, "this");
    pointer_t p = (pointer_t)lua_touserdata(L, -1);
    if (!p) {
      return luaL_error(L, "Pointer 'this' not found");
    } else if (!p->provider()) {
      return luaL_error(L, "Need install provider");
    } else {
      int sz = lua_gettop(L);
      if (!p->provide(L, name)) {
        return luaL_error(L, "Provide failed: %s", name);
      }
      int diff = lua_gettop(L) - sz;
      if (diff != 1) {
        return luaL_error(L, "Should push exactly one value, given %d", diff);
      }
    }
    return 1;
  }
};

////////////////////////////////////////////////////////////////////////////////
/////////////////// The following are DEPRECATED! //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// CRTP: curious recurring template pattern
// Derived class needs to implement:
//     void provide(const std::vector<std::string>& vars)
template <typename Derived>
class luaw_crtp : public luaw {
  static const std::unordered_set<std::string> lua_key_words;
  using base_t = luaw;

public:
  template <typename... Args>
  luaw_crtp(Args&&... args) : base_t(std::forward<Args>(args)...) {}

  // Set global variables to Lua
  void prepare(const char* expr) {
    std::vector<std::string> vars = detect_variable_names(expr);
    static_cast<Derived*>(this)->provide(vars);
  }
  void prepare(const std::string& expr) { prepare(expr.c_str()); }

  /**
   * @brief Evaluate a Lua expression meanwhile can retrieve variables needed
   * from variable provider automatically, then get the result in C++ type.
   *
   * @param [in] expr Lua expression, which must have a return value.
   * @param [in] def The default value returned if failed.
   * @param [in] disable_log Whether print a log when exception occurs.
   * @param [out] failed Will be set whether the operation is failed if this
   * pointer is not nullptr.
   *
   * @{
   */

  // auto eval: prepare variables automatically
#define DEFINE_EVAL(typename, type, default)                                   \
  type auto_eval_##typename(const char* expr,                                  \
                            const type& def         = default,                 \
                            bool        disable_log = false,                   \
                            bool*       failed      = nullptr) {                          \
    prepare(expr);                                                             \
    return base_t::eval_##typename(expr, def, disable_log, failed);            \
  }                                                                            \
  type auto_eval_##typename(const std::string& expr,                           \
                            const type&        def         = default,          \
                            bool               disable_log = false,            \
                            bool*              failed      = nullptr) {                          \
    return this->auto_eval_##typename(expr.c_str(), def, disable_log, failed); \
  }

  DEFINE_EVAL(bool, bool, false)
  DEFINE_EVAL(int, int, 0)
  DEFINE_EVAL(uint, unsigned int, 0)
  DEFINE_EVAL(long, long long, 0)
  DEFINE_EVAL(ulong, unsigned long long, 0)
  DEFINE_EVAL(llong, long long, 0)
  DEFINE_EVAL(ullong, unsigned long long, 0)
  DEFINE_EVAL(float, float, 0)
  DEFINE_EVAL(double, double, 0)
  DEFINE_EVAL(ldouble, long double, 0)
  DEFINE_EVAL(string, std::string, "")
#undef DEFINE_EVAL

  const char* auto_eval_c_str(const char* expr,
                              const char* def         = "",
                              bool        disable_log = false,
                              bool*       failed      = nullptr) {
    prepare(expr);
    return base_t::eval_c_str(expr, def, disable_log, failed);
  }
  const char* auto_eval_c_str(const std::string& expr,
                              const char*        def         = "",
                              bool               disable_log = false,
                              bool*              failed      = nullptr) {
    return this->auto_eval_c_str(expr.c_str(), def, disable_log, failed);
  }

  /** @} */

  //////////////////////////////////////////////////////////////////////////////

  // Detect variable names in a lua script
  std::vector<std::string> detect_variable_names(
      const std::string& expr) const {
    return detect_variable_names(expr.c_str());
  }
  std::vector<std::string> detect_variable_names(const char* expr) const {
    if (!expr || *expr == 0) return std::vector<std::string>{};
    std::unordered_set<std::string> ret, ud;  // ud: user defined
    std::string                     name;
    bool                            found = false;
    for (const char* s = expr; *s;) {
      if (found) {
        if (std::isalnum(*s) || *s == '_') {
          name += *s++;
        } else if (*s == '(') {
          name.clear();
          found = false;
          ++s;
        } else {
          if (!lua_key_words.count(name) && !ud.count(name)) {
            const char* t = s;
            while (std::isspace(*t)) ++t;
            if (*t == '.') {
              if (t[1] == '.') {
                ret.insert(name);
              } else {
                // package or table
              }
            } else if (*t == '=' && t[1] != '=') {
              // user defined var
              ud.insert(name);
            } else {
              ret.insert(name);
            }
          }
          name.clear();
          found = false;
          if (*s == '.') {
            if (s[1] == '.') {
              s += 2;
            } else {
              while (isalnum(*s) || *s == '_' || *s == '.') ++s;
            }
          } else if (*s == '-' && s[1] == '-') {
            // code comment
          } else {
            ++s;
          }
        }
      } else {  // not found variable name
        if (isalpha(*s) || *s == '_') {
          found = true;
          name += *s++;
        } else if (*s == '-' && s[1] == '-') {  // code comments
          bool single_line_comment = false;
          if (s[2] == '[') {
            if (s[3] == '[') {
              s += 4;
              while (*s && !(*s == ']' && s[1] == ']')) ++s;
              if (*s) s += 2;
            } else if (s[3] == '=') {
              int         cnt = 1;  // count of consecutive "="
              const char* t   = s + 4;
              while (*t && *t == '=') {
                ++cnt;
                ++t;
              }
              if (*t == '[') {
                s = t + 1;
                while (*s) {
                  if (*s != ']') {
                    ++s;
                    continue;
                  }
                  if (s[1] == '=') {
                    int         cnt2 = 1;
                    const char* t2   = s + 2;
                    while (*t2 && *t2 == '=') ++cnt2, ++t2;
                    if (cnt2 == cnt && *t2 == ']') {
                      s = t2 + 1;
                      break;
                    }
                    s = t2;
                    continue;
                  }
                  ++s;
                }
              } else {
                single_line_comment = true;
              }
            } else {
              single_line_comment = true;
            }
          } else {
            single_line_comment = true;
          }
          if (single_line_comment) {
            s += 2;
            while (*s != '\n') ++s;
            ++s;
          }
        } else if (*s == '[' && s[1] == '[') {  // multi line string
          s += 2;
          while (*s && !(*s == ']' && s[1] == ']')) ++s;
          if (*s) s += 2;
        } else if (*s == '\'' || *s == '\"') {  // one line string
          char target = *s;
          while (true) {
            ++s;
            if (*s == 0) break;
            if (*s == target) {
              int cnt = 0;  // count of consecutive "\"
              while (s[-cnt - 1] == '\\') ++cnt;
              if (~cnt & 1) {
                ++s;
                break;
              }
            }
          }
        } else {
          ++s;
        }
      }
    }
    if (!name.empty() && !lua_key_words.count(name) && !ud.count(name)) {
      ret.insert(name);
    }
    return std::vector<std::string>(ret.begin(), ret.end());
  }
};

template <typename Derived>
const std::unordered_set<std::string> luaw_crtp<Derived>::lua_key_words{
    "nil",      "true",  "false",    "and",   "or",     "not",
    "if",       "then",  "elseif",   "else",  "end",    "for",
    "do",       "while", "repeat",   "until", "return", "break",
    "continue", "goto",  "function", "in",    "local"};

// Usage examples of luaw_crtp
// VariableProviderType should implement member function:
// `void provide(const std::vector<std::string> &vars, peacalm::luaw* l);`

// Usage template 1
// Inherited raw variable provider type
template <typename VariableProviderType>
class luaw_is_provider
    : public VariableProviderType,
      public luaw_crtp<luaw_is_provider<VariableProviderType>> {
  using base_t     = luaw_crtp<luaw_is_provider<VariableProviderType>>;
  using provider_t = VariableProviderType;

public:
  luaw_is_provider() {}

  template <typename... Args>
  luaw_is_provider(lua_State* L, Args&&... args)
      : base_t(L), provider_t(std::forward<Args>(args)...) {}

  const provider_t& provider() const {
    return static_cast<const provider_t&>(*this);
  }
  provider_t& provider() { return static_cast<provider_t&>(*this); }

  void provide(const std::vector<std::string>& vars) {
    provider().provide(vars, this);
  }
};

// Usage template 2
// Has a member provider_, which could be raw variable provider type T,
// or T*, or std::shared_ptr<T> or std::unique_ptr<T>.
// Should install provider before use.
template <typename VariableProviderType>
class luaw_has_provider
    : public luaw_crtp<luaw_has_provider<VariableProviderType>> {
  using base_t     = luaw_crtp<luaw_has_provider<VariableProviderType>>;
  using provider_t = VariableProviderType;

  provider_t provider_;

public:
  template <typename... Args>
  luaw_has_provider(Args&&... args) : base_t(std::forward<Args>(args)...) {}

  void              provider(const provider_t& p) { provider_ = p; }
  void              provider(provider_t&& p) { provider_ = std::move(p); }
  const provider_t& provider() const { return provider_; }
  provider_t&       provider() { return provider_; }

  void provide(const std::vector<std::string>& vars) {
    __provide(vars, luaw_detail::is_ptr<provider_t>{});
  }

private:
  void __provide(const std::vector<std::string>& vars, std::true_type) {
    provider()->provide(vars, this);
  }
  void __provide(const std::vector<std::string>& vars, std::false_type) {
    provider().provide(vars, this);
  }
};

}  // namespace peacalm

#endif  // PEACALM_LUAW_H_
