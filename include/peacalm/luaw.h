// Copyright (c) 2023-2024 Li Shuangquan. All Rights Reserved.
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
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
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

// Macro switch: Whether support accessing members by volatile objects.
// Do not support by default.
#ifndef PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT
#ifdef PEACALM_LUAW_NEED_VOLATILE
#define PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT true
#else
#define PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT false
#endif
#endif

static_assert(
    std::is_integral<decltype(PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT)>::value,
    "PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT should be bool or int");

#define PEACALM_LUAW_SUPPORT_CPP17 (__cplusplus >= 201703)

#if PEACALM_LUAW_SUPPORT_CPP17
#define PEACAML_LUAW_IF_CONSTEXPR constexpr
#else
#define PEACAML_LUAW_IF_CONSTEXPR
#endif

namespace peacalm {

namespace luaexf {  // Useful extended functions for Lua

/// Short writing for if-elseif-else statement.
/// The number of arguments should be odd and at least 3.
/// Usage: IF(expr1, result_if_expr1_is_true,
///           expr2, result_if_expr2_is_true,
///           ...,
///           result_if_all_exprs_are_false)
/// Example: return IF(a > b, 'good', 'bad')
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

/// Convert multiple arguments or a list to a set, where key's mapped value is
/// boolean true.
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

/// Convert multiple arguments or a list to a dict, where key's mapped value is
/// the key's appearance count. Return nil if key not exists.
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

/// Like COUNTER but return 0 if key not exists.
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

namespace luaw_detail {

template <typename Class, typename F>
struct c_function_to_const_member_function {
  using type = F;
};

template <typename Class, typename Return, typename... Args>
struct c_function_to_const_member_function<Class, Return(Args...)> {
  using type = Return (Class::*)(Args...) const;
};

template <typename Class, typename Return, typename... Args>
struct c_function_to_const_member_function<Class, Return(Args..., ...)> {
  using type = Return (Class::*)(Args..., ...) const;
};

template <typename Class, typename F>
using c_function_to_const_member_function_t =
    typename c_function_to_const_member_function<Class, F>::type;

}  // namespace luaw_detail

// The luaw family.
class luaw;
class fakeluaw;
class subluaw;

/// Basic Lua wrapper class.
class luaw {
  using self_t = luaw;

  // The path where registered member operations stored:
  // LUA_REGISTRYINDEX -> (void*)(&typeid(T)) -> this enum field
  enum member_info_fields {
    member_function = 1,
    member_getter,
    member_setter,
    dynamic_member_getter,
    dynamic_member_setter,
    const_member,
    nonconst_member_function,
    nonvolatile_member_function
  };

  // std::tuple as one Lua table, like std::pair
  template <typename T, typename = void>
  struct pusher;
  template <typename T, typename = void>
  struct convertor;

  // std::tuple as multi value in stack
  template <typename T>
  struct pusher_for_return;
  template <typename T>
  struct convertor_for_return;

  // Implementation for registering members of classes
  template <typename T>
  struct register_ctor_impl;
  template <typename T, typename = void>
  struct registrar;
  // Used for registrar sepcialication for registering member ptr/ref.
  struct registrar_tag_for_member_ptr;

  // Mock std::mem_fn by a function or callable object whose first argument
  // must be a raw pointer of a class.
  // Then can call this class by reference, raw pointer or smart pointers
  // (std::shared_ptr and std::unique_ptr) of the class.
  // Also can accept a real member pointer, then it is just like std::mem_fn.
  template <typename T, typename = void>
  class mock_mem_fn;

  // Mock non-member to be a class's member.
  // Behaves like std::mem_fn, accepts a first argument representing the class's
  // instance, but this one would drop it, it's just a placeholder, non-member
  // variables/functions don't need it after all.
  // Can accept variable pointer or function pointer.
  template <typename T, typename = void>
  class static_mem_fn;

  lua_State* L_;

public:
  using lua_cfunction_t = lua_CFunction;  // int (*)(lua_State *L)
  using lua_number_t    = lua_Number;     // double as default
  using lua_integer_t   = lua_Integer;    // long long as default

  /// To generate shared/exclusive metatable of type T.
  template <typename T, typename = void>
  struct metatable_factory;

  /// A callable wrapper for Lua functions. Like std::function and can convert
  /// to std::function, but contains more status information. Used within method
  /// get.
  template <typename T>
  class function;

  /// Used as hint type for set/push/setkv, indicate the value is a class
  /// object.
  struct class_tag {};

  /// Used as hint type for set/push/setkv, indicate the value is a function or
  /// a callable object used as a function.
  struct function_tag {};

  /// Used as a value for set/push/setkv, indicate that should set/push a new
  /// empty table to Lua.
  struct newtable_tag {};

  /// Used as a key for seek/touchtb/setkv/lset, indicate that we're
  /// getting/setting a value's metatable.
  /// "tname" is used for touchtb to create a new metatable with this given name
  /// if the value doesn't have a metatable.
  struct metatable_tag {
    const char* tname;
    metatable_tag(const char* name = nullptr) : tname(name) {}
  };

  /// Indicate that convert a Lua value to nothing, the Lua value is useless.
  /// Any Lua value can convert to this.
  /// Maybe used as a C++ function formal parameter.
  struct placeholder_tag {};

  /// Represent a Lua value in stack by index.
  class luavalueidx {
    lua_State* L_;
    int        idx_;

  public:
    luavalueidx(lua_State* L = nullptr, int idx = 0) : L_(L), idx_(idx) {}

    lua_State* L() const { return L_; }

    int idx() const { return idx_; }

    bool valid() const {
      return L_ && std::abs(idx_) >= 1 && std::abs(idx_) <= lua_gettop(L_);
    }

    lua_State* main_thread() const { return luaw::get_main_thread_of(L_); }
  };

  /// Make a luavalueidx to represent the value at given index.
  luavalueidx make_luavalueidx(int idx) const { return luavalueidx(L_, idx); }

  /// A reference of some Lua value in LUA_REGISTRYINDEX.
  class luavalueref {
    lua_State*                 L_;
    std::shared_ptr<const int> ref_sptr_;

  public:
    /// Make a reference of the value at index "idx" in the stack "L".
    luavalueref(lua_State* L = nullptr, int idx = -1) : L_(L) {
      if (L && std::abs(idx) >= 1 && std::abs(idx) <= lua_gettop(L)) {
        lua_pushvalue(L, idx);  // make a copy
        // pops the value on top and returns its ref_id.
        const int ref_id = luaL_ref(L, LUA_REGISTRYINDEX);
        ref_sptr_.reset(new int(ref_id), [L](const int* p) {
          luaL_unref(L, LUA_REGISTRYINDEX, *p);  // release the ref_id
          delete p;
        });
      }
    }

    lua_State* L() const { return L_; }

    int ref_id() const { return ref_sptr_ ? *ref_sptr_ : LUA_NOREF; }

    bool valid() const { return L_ && ref_sptr_ && *ref_sptr_ != LUA_NOREF; }

    bool as_nil() const { return !valid() || (ref_id() == LUA_REFNIL); }

    lua_State* main_thread() const { return luaw::get_main_thread_of(L_); }

    void unref() { ref_sptr_.reset(); }

    /// Set the referenced value to a global variable with given name.
    /// Equivalent to luaw::set(name, this luavalueref).
    void setglobal(const char* name) const {
      PEACALM_LUAW_ASSERT(name);
      lua_rawgeti(L_, LUA_REGISTRYINDEX, ref_id());
      lua_setglobal(L_, name);
    }

    /// Push the value referenced on top of this stack.
    void pushvalue() const {
      PEACALM_LUAW_ASSERT(L_);
      lua_rawgeti(L_, LUA_REGISTRYINDEX, ref_id());
    }

    void pop(int n = 1) { lua_pop(L_, n); }
    int  gettop() const { return lua_gettop(L_); }
    void settop(int idx) { lua_settop(L_, idx); }
    void cleartop() { settop(0); }
  };

  /// Make a reference for the value at given index.
  luavalueref make_luavalueref(int idx = -1) const {
    return luavalueref(L_, idx);
  }

  /// Stack balance guarder.
  /// Automatically set stack to a specific size when destruct.
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

  /// Initialization options for luaw.
  class opt {
    enum libopt : char { ignore = 0, load = 1, preload = 2 };

  public:
    opt() {}

    /// Ignore all standard libs.
    opt& ignore_libs() {
      libopt_ = ignore;
      return *this;
    }
    /// Load all standard libs.
    opt& load_libs() {
      libopt_ = load;
      return *this;
    }
    /// Preload all standard libs.
    opt& preload_libs() {
      libopt_ = preload;
      return *this;
    }

    /// Register extended functions.
    opt& register_exfunctions(bool r) {
      exfunc_ = r;
      return *this;
    }

    /// Use already existed lua_State.
    opt& use_state(lua_State* L) {
      PEACALM_LUAW_ASSERT(L);
      L_ = L;
      return *this;
    }

    /// Load user specified libs.
    opt& custom_load(const std::vector<luaL_Reg>& l) {
      libs_load_ = l;
      return *this;
    }
    opt& custom_load(std::vector<luaL_Reg>&& l) {
      libs_load_ = std::move(l);
      return *this;
    }

    /// Preload user specified libs.
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

  // Default constructor
  luaw(const opt& o = opt{}) { init(o); }

  luaw(lua_State* L) : L_(L) {}

  // Not copyable
  luaw(const luaw&) = delete;

  // Movable
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
    // Initialize lua_State
    if (o.L_) {
      L_ = o.L_;
    } else {
      L_ = luaL_newstate();
    }

    // Initialize standard libs
    if (o.libopt_ == opt::libopt::load) {
      // Load all libs, which is costly
      luaL_openlibs(L_);
    } else if (o.libopt_ == opt::libopt::preload) {
      // Preload all libs, which is light
      preload_libs();
    }

    // Register extended functions
    if (o.exfunc_) { register_exfunctions(); }

    // Load user specified libs
    if (!o.libs_load_.empty()) {
      for (const luaL_Reg& l : o.libs_load_) {
        luaL_requiref(L_, l.name, l.func, 1);
        pop();
      }
    }

    // Preload user specified libs
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
  void       clearL() { L_ = nullptr; }

  /// Generate a subluaw by a new thread and a ref_id to it.
  subluaw make_subluaw();

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

  // clang-format off
  bool is_type_none(int idx = -1)          const { return lua_type(L_, idx) == LUA_TNONE; }
  bool is_type_nil(int idx = -1)           const { return lua_type(L_, idx) == LUA_TNIL; }
  bool is_type_boolean(int idx = -1)       const { return lua_type(L_, idx) == LUA_TBOOLEAN; }
  bool is_type_lightuserdata(int idx = -1) const { return lua_type(L_, idx) == LUA_TLIGHTUSERDATA; }
  bool is_type_number(int idx = -1)        const { return lua_type(L_, idx) == LUA_TNUMBER; }
  bool is_type_string(int idx = -1)        const { return lua_type(L_, idx) == LUA_TSTRING; }
  bool is_type_table(int idx = -1)         const { return lua_type(L_, idx) == LUA_TTABLE; }
  bool is_type_function(int idx = -1)      const { return lua_type(L_, idx) == LUA_TFUNCTION; }
  bool is_type_userdata(int idx = -1)      const { return lua_type(L_, idx) == LUA_TUSERDATA; }
  bool is_type_thread(int idx = -1)        const { return lua_type(L_, idx) == LUA_TTHREAD; }
  // clang-format on

  int         type(int idx = -1) const { return lua_type(L_, idx); }
  const char* type_name(int idx = -1) const {
    return lua_typename(L_, type(idx));
  }

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

  /// Push the metatable in regristry with name "tname" onto stack, if not
  /// exists, create one. Return whether created a new metatable.
  bool gtouchmetatb(const char* tname) { return luaL_newmetatable(L_, tname); }

  void       newtable() { lua_newtable(L_); }
  void*      newuserdata(size_t size) { return lua_newuserdata(L_, size); }
  lua_State* newthread() { return lua_newthread(L_); }

  /// Get main thread of a given thread.
  static lua_State* get_main_thread_of(lua_State* L) {
    if (!L) return nullptr;
    lua_geti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
    lua_State* ret = lua_tothread(L, -1);
    lua_pop(L, 1);
    return ret;
  }

  /// Get main thread.
  lua_State* main_thread() const { return get_main_thread_of(L_); }

  /// Push onto the stack the global value with given name.
  /// Return the type of that value.
  int getglobal(const char* name) {
    PEACALM_LUAW_ASSERT(name);
    return lua_getglobal(L_, name);
  }

  /// Pop a value from the stack and set it as the new value of global "name".
  void setglobal(const char* name) {
    PEACALM_LUAW_ASSERT(name);
    lua_setglobal(L_, name);
  }

  int pcall(int narg, int nret, int f) { return lua_pcall(L_, narg, nret, f); }

  /// Whether the value at idx is indexable.
  bool indexable(int idx = -1) const {
    if (istable(idx)) return true;
    if (lua_getmetatable(L_, idx) == 0) return false;
    auto _g = make_guarder(gettop() - 1);
    lua_getfield(L_, -1, "__index");
    // if __index exists, then regard as indexable
    bool ret = !isnoneornil(-1);
    // maybe more checks?
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
    // maybe more checks?
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
    // maybe more checks?
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
  // 6. Integer's precision won't be lost if its value is representable by 64bit
  //    signed integer type, i.e. between [-2^63, 2^63 -1], which is
  //    [-9223372036854775808, 9223372036854775807]
  //
  // Examples:
  // * number 2.5 -> string "2.5" (By Lua)
  // * number 3 -> string "3.0" (By Lua)
  // * integer 3 -> string "3" (By Lua)
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

  // Unsafe version.
  // NOTICE: Lua will implicitly convert number to string
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

  // Safe version.
  // Make a copy of the value before converts it to string when it is a number.
  std::string to_string(int                idx         = -1,
                        const std::string& def         = "",
                        bool               disable_log = false,
                        bool*              failed      = nullptr,
                        bool*              exists      = nullptr) {
    if (is_type_number(idx)) {
      pushvalue(idx);  // make a copy
      std::string ret = to_c_str(-1, def.c_str(), disable_log, failed, exists);
      pop();
      return ret;
    }
    return std::string{to_c_str(idx, def.c_str(), disable_log, failed, exists)};
  }

  /** @} */

  /**
   * @brief Convert a value in Lua stack to complex C++ type.
   *
   * Can get a C++ data type.
   * T could be any type composited by bool, integer types, float number types,
   * C string, std::string, std::pair, std::tuple, std::vector, std::set,
   * std::unordered_set, std::map, std::unordered_map, std::deque, std::list,
   * std::forward_list.
   * Also, T could be cv-qualified, but cannot be a reference.
   * Note that using C string "const char*" as key type of set or map is
   * forbidden, and anytime using "const char*" is not recommended, should
   * better us std::string instead.
   *
   * Can get a user defined class type.
   *
   * Can get a callable function like type.
   * T could be std::function or luaw::function.
   *
   * Can get a pointer type.
   *
   * @tparam T The result type user expected.
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
    static_assert(
        std::is_same<Hint,
                     std::remove_cv_t<std::remove_reference_t<Hint>>>::value,
        "Hint should not be const/volatile/reference");
    return pusher<std::decay_t<Hint>>::push(*this, std::forward<T>(value));
  }

  /// Copy and push a value already in the stack.
  void pushvalue(int idx = -1) { lua_pushvalue(L_, idx); }

  /// Copy a value in stack to a global value with given name.
  void copy_to_global(const char* name, int idx = -1) {
    pushvalue(idx);
    setglobal(name);
  }

  /// Push the global environment onto the stack.
  void pushglobaltable() { lua_pushglobaltable(L_); }

  /// Push a new table onto the stack.
  void pushnewtable() { lua_newtable(L_); }

  /// Push the thread represented by L onto the stack.
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
   * T could be data type, pointer type, C function, lambda, std::function and
   * user defined class types.
   *
   * set(name, nullptr) means set nil to "name".
   * set(name, luaw::newtable_tag) means set a new empty table to "name".
   *
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

  /// Long set. The last argument is value, the rest arguments are indexes and
  /// sub-indexes, where could contain luaw::metatable_tag.
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

  ///////////////////////// set raw pointer by wrapper /////////////////////////

  /// Pointer wrapper type
  template <typename T>
  using ptrw = const std::shared_ptr<T>;

  /// Make a wrapper for a raw pointer.
  /// (The raw pointer can't be pointer of smart ptr)
  template <typename T>
  static ptrw<T> make_ptrw(T* p);

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
  void set_ptr_by_wrapper(const char* name, T* p) {
    set(name, make_ptrw<T>(p));
  }
  template <typename T>
  void set_ptr_by_wrapper(const std::string& name, T* p) {
    set_ptr_by_wrapper<T>(name.c_str(), p);
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
   * @sa method "to"
   *
   * @tparam T The result type user expected.
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
    auto _g = make_guarder();
    getglobal(name);
    return to<T>(-1, disable_log, failed, exists);
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

  /// Options used as first parameter for "lget".
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
  //////////////////////// call Lua function ///////////////////////////////////

  /// Call a global Lua function specified by name using C++ parameters.
  /// Should at least provide return type.
  template <typename Return, typename... Args>
  Return callf(const char* fname, const Args&... args) {
    PEACALM_LUAW_ASSERT(fname);
    auto f = get<luaw::function<Return(Args...)>>(fname);
    return f(args...);
  }
  template <typename Return, typename... Args>
  Return callf(const std::string& fname, const Args&... args) {
    auto f = get<luaw::function<Return(Args...)>>(fname);
    return f(args...);
  }

  /// Call a Lua function specified by path using C++ parameters.
  template <typename Return, typename... Args>
  Return callf(const std::initializer_list<const char*>& path,
               const Args&... args) {
    auto f = get<luaw::function<Return(Args...)>>(path);
    return f(args...);
  }
  template <typename Return, typename... Args>
  Return callf(const std::initializer_list<std::string>& path,
               const Args&... args) {
    auto f = get<luaw::function<Return(Args...)>>(path);
    return f(args...);
  }
  template <typename Return, typename... Args>
  Return callf(const std::vector<const char*>& path, const Args&... args) {
    auto f = get<luaw::function<Return(Args...)>>(path);
    return f(args...);
  }
  template <typename Return, typename... Args>
  Return callf(const std::vector<std::string>& path, const Args&... args) {
    auto f = get<luaw::function<Return(Args...)>>(path);
    return f(args...);
  }

  //////////////////////// register ctor/memeber for class /////////////////////

  /**
   * @brief Register a global function to Lua who can create object.
   *
   * It creates a userdata in Lua, whose C++ type is specified by Return type
   * of Ctor. e.g. register_ctor<Object(int)>("NewObject"). Then can run
   * "o = NewObject(1)" in Lua.
   *
   * @tparam Ctor Should be a function type of "Return(Args...)".
   * @param fname The global function name registered.
   */
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
  register_member(const char* name, MemberPointer mp) {
    PEACALM_LUAW_ASSERT(name);
    PEACALM_LUAW_ASSERT(mp);
    registrar<std::decay_t<MemberPointer>>::register_member(
        *this, name, std::mem_fn(mp));
  }
  template <typename MemberPointer>
  std::enable_if_t<std::is_member_pointer<MemberPointer>::value>
  register_member(const std::string& name, MemberPointer mp) {
    register_member<MemberPointer>(name.c_str(), mp);
  }

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
   * @tparam Member The static member's type.
   * @param name The static member's name.
   * @param m The static member's pointer.
   * @return void
   */
  template <typename Class, typename Member>
  std::enable_if_t<std::is_class<Class>::value> register_static_member(
      const char* name, Member* m) {
    static_assert(std::is_same<Class, std::decay_t<Class>>::value,
                  "Class type should be decayed");
    PEACALM_LUAW_ASSERT(name);
    PEACALM_LUAW_ASSERT(m);

    // add const member property for function
    using MemberPointer = std::conditional_t<
        std::is_member_function_pointer<Member Class::*>::value,
        luaw_detail::c_function_to_const_member_function_t<Class, Member>,
        Member Class::*>;

    register_static_member<MemberPointer>(name, m);
  }
  template <typename Class, typename Member>
  std::enable_if_t<std::is_class<Class>::value> register_static_member(
      const std::string& name, Member* m) {
    register_static_member<Class, Member>(name.c_str(), m);
  }

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
   * @tparam Member The static member's type.
   * @param name The static member's name.
   * @param m The static member's pointer.
   * @return void
   */
  template <typename MemberPointer, typename Member>
  std::enable_if_t<std::is_member_pointer<MemberPointer>::value>
  register_static_member(const char* name, Member* m) {
    PEACALM_LUAW_ASSERT(name);
    PEACALM_LUAW_ASSERT(m);
    static_assert(
        std::is_same<MemberPointer, std::decay_t<MemberPointer>>::value,
        "MemberPointer should be decayed");
    registrar<std::decay_t<MemberPointer>>::register_member(
        *this, name, static_mem_fn<Member*>(m));
  }
  template <typename MemberPointer, typename Member>
  std::enable_if_t<std::is_member_pointer<MemberPointer>::value>
  register_static_member(const std::string& name, Member* m) {
    register_static_member<MemberPointer, Member>(name.c_str(), m);
  }

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
  void register_dynamic_member(Getter&& getter, Setter&& setter) {
    register_dynamic_member_getter(std::forward<Getter>(getter));
    register_dynamic_member_setter(std::forward<Setter>(setter));
  }
  template <typename Getter>
  void register_dynamic_member_getter(Getter&& getter);
  template <typename Setter>
  void register_dynamic_member_setter(Setter&& setter);

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
  void register_member_ptr(const char* name, Member Class::*mp) {
    static_assert(std::is_same<Class, std::decay_t<Class>>::value,
                  "Class must be decayed");
    PEACALM_LUAW_ASSERT(name);
    PEACALM_LUAW_ASSERT(mp);
    registrar<Member Class::*, luaw::registrar_tag_for_member_ptr>::
        register_member_ptr(*this, name, [=](auto& o) { return &(o.*mp); });
  }
  template <typename Class, typename Member>
  void register_member_ptr(const std::string& name, Member Class::*mp) {
    register_member_ptr<Class, Member>(name.c_str(), mp);
  }

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
  void register_member_cptr(const char* name, Member Class::*mp) {
    register_member_ptr<Class, const Member>(name, mp);
  }
  template <typename Class, typename Member>
  void register_member_cptr(const std::string& name, Member Class::*mp) {
    register_member_cptr<Class, Member>(name.c_str(), mp);
  }

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
  void register_member_ref(const char* name, Member Class::*mp) {
    static_assert(std::is_same<Class, std::decay_t<Class>>::value,
                  "Class must be decayed");
    PEACALM_LUAW_ASSERT(name);
    PEACALM_LUAW_ASSERT(mp);
    registrar<Member Class::*, luaw::registrar_tag_for_member_ptr>::
        register_member_ref(*this, name, std::mem_fn(mp));
  }
  template <typename Class, typename Member>
  void register_member_ref(const std::string& name, Member Class::*mp) {
    register_member_ref<Class, Member>(name.c_str(), mp);
  }

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
  void register_member_cref(const char* name, Member Class::*mp) {
    register_member_ref<Class, const Member>(name, mp);
  }
  template <typename Class, typename Member>
  void register_member_cref(const std::string& name, Member Class::*mp) {
    register_member_cref<Class, Member>(name.c_str(), mp);
  }

  /**
   * @brief Register a static member's pointer into Lua.
   *
   * @sa "register_static_member"
   * @sa "register_member_ptr"
   *
   * @tparam Class The class whom the static member will belong to. Must be a
   * decayed class type.
   * @tparam Member Type of the static member.
   * @param name Name of the member's pointer used in Lua.
   * @param mp The static member's pointer value.
   */
  template <typename Class, typename Member>
  void register_static_member_ptr(const char* name, Member* mp) {
    static_assert(std::is_class<Class>::value,
                  "Static members must belong to a class type");
    static_assert(std::is_same<Class, std::decay_t<Class>>::value,
                  "Class must be decayed");
    PEACALM_LUAW_ASSERT(name);
    PEACALM_LUAW_ASSERT(mp);
    registrar<Member Class::*, luaw::registrar_tag_for_member_ptr>::
        register_member_ptr(*this, name, [=](auto&) { return mp; });
  }
  template <typename Class, typename Member>
  void register_static_member_ptr(const std::string& name, Member* mp) {
    register_static_member_ptr<Class, Member>(name.c_str(), mp);
  }

  /**
   * @brief Register a static member's low-level const pointer into Lua.
   *
   * @sa "register_static_member_ptr"
   */
  template <typename Class, typename Member>
  void register_static_member_cptr(const char* name, Member* mp) {
    register_static_member_ptr<Class, const Member>(name, mp);
  }
  template <typename Class, typename Member>
  void register_static_member_cptr(const std::string& name, Member* mp) {
    register_static_member_cptr<Class, Member>(name.c_str(), mp);
  }

  /**
   * @brief Register a static member's reference into Lua.
   *
   * @sa "register_static_member"
   * @sa "register_member_ref"
   */
  template <typename Class, typename Member>
  void register_static_member_ref(const char* name, Member* mp) {
    static_assert(std::is_class<Class>::value,
                  "Static members must belong to a class type");
    static_assert(std::is_same<Class, std::decay_t<Class>>::value,
                  "Class must be decayed");
    PEACALM_LUAW_ASSERT(name);
    PEACALM_LUAW_ASSERT(mp);
    registrar<Member Class::*, luaw::registrar_tag_for_member_ptr>::
        register_member_ref(*this, name, static_mem_fn<Member*>(mp));
  }
  template <typename Class, typename Member>
  void register_static_member_ref(const std::string& name, Member* mp) {
    register_static_member_ref<Class, Member>(name.c_str(), mp);
  }

  /**
   * @brief Register a static member's low-level const reference into Lua.
   *
   * @sa "register_static_member_ref"
   */
  template <typename Class, typename Member>
  void register_static_member_cref(const char* name, Member* mp) {
    register_static_member_ref<Class, const Member>(name, mp);
  }
  template <typename Class, typename Member>
  void register_static_member_cref(const std::string& name, Member* mp) {
    register_static_member_cref<Class, Member>(name.c_str(), mp);
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
   * @sa method "to".
   * But more, T could be void or std::tuple<> to represent the expression has
   * no return, or we'll discard all it's returns.
   * Besides, T could be std::tuple to accept multiple returns of the
   * expression. If T is a nested std::tuple, only the outmost level std::tuple
   * is mapped to multiple values of return, any inner std::tuple is mapped to a
   * Lua table.
   *
   * @tparam T The result type user expected.
   * @param [in] expr Lua expression, which can have no return, one return or
   * multiple returns.
   * @param [in] disable_log Whether print a log when exception occurs.
   * @param [out] failed Will be set whether the operation is failed if this
   * pointer is not nullptr. If T is a container type, it regards the operation
   * as failed if any element failed.
   * @return The expression's result in type T. If T is not void or
   * std::tuple<>, the expression must provide return values.
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
    return convertor_for_return<std::decay_t<T>>::to(
        *this, sz + 1, disable_log, failed);
  }
  template <typename T>
  T eval(const std::string& expr,
         bool               disable_log = false,
         bool*              failed      = nullptr) {
    return eval<T>(expr.c_str(), disable_log, failed);
  }

  ///////////////////////// metatable for lightuserdata ////////////////////////

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
  void set_lightuserdata_metatable() {
    static_assert(std::is_pointer<T>::value,
                  "Should provide a pointer type for lightuserdata");
    auto _g = make_guarder();
    pushlightuserdata(static_cast<void*>(0));
    metatable_factory<T>::push_shared_metatable(*this);
    setmetatable(-2);
  }

  /// Remove lightuserdata's metatable, i.e. set nil as metatable.
  void clear_lightuserdata_metatable() {
    auto _g = make_guarder();
    pushlightuserdata(static_cast<void*>(0));
    pushnil();
    setmetatable(-2);
  }

  /// Tell whether lightuserdata has metatable (not nil).
  bool lightuserdata_has_metatable() {
    auto _g = make_guarder();
    pushlightuserdata(static_cast<void*>(0));
    bool has = getmetatable(-1);
    return has;
  }

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
                                               bool* exists = nullptr) {
    auto _g = make_guarder();
    pushlightuserdata(static_cast<void*>(0));
    bool has = getmetatable(-1);
    if (has_metatable) *has_metatable = has;
    if (!has) return def;
    getfield(-1, "__name");
    return to_string(-1, def, disable_log, failed, exists);
  }

  /// Get metatable name for value at given index. (not only for light userdata)
  std::string get_metatable_name(int                idx           = -1,
                                 const std::string& def           = "",
                                 bool*              has_metatable = nullptr,
                                 bool               disable_log   = false,
                                 bool*              failed        = nullptr,
                                 bool*              exists        = nullptr) {
    auto _g  = make_guarder();
    bool has = getmetatable(idx);
    if (has_metatable) *has_metatable = has;
    if (!has) return def;
    getfield(-1, "__name");
    return to_string(-1, def, disable_log, failed, exists);
  }

  /// Get a global variable's metatable name.
  std::string get_metatable_name(const char*        name,
                                 const std::string& def           = "",
                                 bool*              has_metatable = nullptr,
                                 bool               disable_log   = false,
                                 bool*              failed        = nullptr,
                                 bool*              exists        = nullptr) {
    auto _g = make_guarder();
    getglobal(name);
    return get_metatable_name(
        -1, def, has_metatable, disable_log, failed, exists);
  }
  std::string get_metatable_name(const std::string& name,
                                 const std::string& def           = "",
                                 bool*              has_metatable = nullptr,
                                 bool               disable_log   = false,
                                 bool*              failed        = nullptr,
                                 bool*              exists        = nullptr) {
    return get_metatable_name(
        name.c_str(), def, has_metatable, disable_log, failed, exists);
  }

  /// Get a variable's metatable name by a given path.
  std::string get_metatable_name(const std::initializer_list<const char*>& path,
                                 const std::string& def           = "",
                                 bool*              has_metatable = nullptr,
                                 bool               disable_log   = false,
                                 bool*              failed        = nullptr,
                                 bool*              exists        = nullptr) {
    auto _g = make_guarder();
    gseek_env();
    for (const auto& s : path) seek(s);
    return get_metatable_name(
        -1, def, has_metatable, disable_log, failed, exists);
  }
  std::string get_metatable_name(const std::vector<const char*>& path,
                                 const std::string&              def = "",
                                 bool* has_metatable                 = nullptr,
                                 bool  disable_log                   = false,
                                 bool* failed                        = nullptr,
                                 bool* exists = nullptr) {
    auto _g = make_guarder();
    gseek_env();
    for (const auto& s : path) seek(s);
    return get_metatable_name(
        -1, def, has_metatable, disable_log, failed, exists);
  }
  std::string get_metatable_name(const std::initializer_list<std::string>& path,
                                 const std::string& def           = "",
                                 bool*              has_metatable = nullptr,
                                 bool               disable_log   = false,
                                 bool*              failed        = nullptr,
                                 bool*              exists        = nullptr) {
    auto _g = make_guarder();
    gseek_env();
    for (const auto& s : path) seek(s);
    return get_metatable_name(
        -1, def, has_metatable, disable_log, failed, exists);
  }
  std::string get_metatable_name(const std::vector<std::string>& path,
                                 const std::string&              def = "",
                                 bool* has_metatable                 = nullptr,
                                 bool  disable_log                   = false,
                                 bool* failed                        = nullptr,
                                 bool* exists = nullptr) {
    auto _g = make_guarder();
    gseek_env();
    for (const auto& s : path) seek(s);
    return get_metatable_name(
        -1, def, has_metatable, disable_log, failed, exists);
  }
  ///////////////////////// error log //////////////////////////////////////////

  static void log_error(const char* s) {
    std::cerr << "Lua: " << s << std::endl;
  }

  const char* get_error_info_in_stack(int idx = -1) const {
    return lua_tostring(L_, idx);
  }

  /// Output the error info at given index of stack to stderr.
  bool log_error_in_stack(int idx = -1) const {
    const char* s = get_error_info_in_stack(idx);
    if (s) {
      std::cerr << "Lua: " << s << std::endl;
      return true;
    } else {
      std::cerr << "No valid error info in stack at index: " << idx
                << std::endl;
      return false;
    }
  }

  /// Output the error info on top of stack to stderr then pop it.
  bool log_error_out() {
    if (log_error_in_stack(-1)) {
      pop();
      return true;
    }
    return false;
  }

  /// Output error: Can't convert value at given index to target type.
  void log_type_convert_error(int idx, const char* to) {
    std::cerr << "Lua: Can't convert to " << to << " by ";
    if (isstring(idx) || isnumber(idx) || isboolean(idx) || isinteger(idx)) {
      std::cerr << type_name(idx) << ": ";
    }
    std::cerr << luaL_tolstring(L_, idx, NULL) << std::endl;
    pop();
  }

  /// Output every value in stack in order.
  void print_stack(const char* name = nullptr) const {
    std::cout << "Stack";
    if (name) std::cout << " " << name;
    if (gettop() <= 0) {
      std::cout << " empty." << std::endl;
      return;
    }
    std::cout << ":\n";
    for (int i = 1, n = gettop(); i <= n; ++i) {
      std::cout << "[" << std::setw(2) << i << "] " << std::setw(8)
                << type_name(i) << "(" << type(i)
                << "): " << luaL_tolstring(L_, i, NULL) << std::endl;
      lua_pop(L_, 1);
    }
  }
};

/// Wrapper for a given lua_State. Only to use methods of luaw,
/// won't close the given lua_State when it destructs.
class fakeluaw : public luaw {
  using base_t = luaw;

public:
  fakeluaw(lua_State* L) : base_t(L) {}

  ~fakeluaw() { base_t::clearL(); }
};

/// Wrapper for a sub thread. So it has an independent execution stack.
/// Generated by luaw::make_subluaw().
class subluaw : public luaw {
  using base_t = luaw;
  const int ref_id_;  // ref id for the sub thread (subL)

public:
  subluaw(lua_State* subL, int ref_id) : base_t(subL), ref_id_(ref_id) {}
  subluaw(subluaw&& r) : base_t(r.release()), ref_id_(r.ref_id_) {}
  subluaw(const subluaw&) = delete;

  void init()  = delete;
  void close() = delete;
  void reset() = delete;

  ~subluaw() {
    if (L()) {
      cleartop();
      // Must unref the sub thread before "clearL".
      luaL_unref(L(), LUA_REGISTRYINDEX, ref_id_);
      // Sub thread shouldn't be closed, so "clearL" must be called at the end.
      base_t::clearL();
    }
  }

  /// Get ref id for this sub thread.
  const int ref_id() const { return ref_id_; }

  /// Push the referenced sub thread onto top of given stack.
  void pushthread(lua_State* L) const {
    PEACALM_LUAW_ASSERT(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref_id_);
  }
};

inline subluaw luaw::make_subluaw() {
  lua_State* subL   = newthread();
  int        ref_id = luaL_ref(L_, LUA_REGISTRYINDEX);
  return subluaw(subL, ref_id);
}

// Support output luaw::luavalueref by std::basic_ostream
template <class Char, class Traits>
std::basic_ostream<Char, Traits>& operator<<(
    std::basic_ostream<Char, Traits>& o, const luaw::luavalueidx& r) {
  fakeluaw l(r.L());
  auto     _g = l.make_guarder();
  o << "{";
  if (l.is_type_string(r.idx()) || l.is_type_number(r.idx()) ||
      l.is_type_boolean(r.idx())) {
    o << l.type_name(r.idx()) << ": ";
  }
  o << luaL_tolstring(l.L(), r.idx(), NULL) << ", idx: " << r.idx() << "}";
  return o;
}

// Support output luaw::luavalueref by std::basic_ostream
template <class Char, class Traits>
std::basic_ostream<Char, Traits>& operator<<(
    std::basic_ostream<Char, Traits>& o, const luaw::luavalueref& r) {
  fakeluaw l(r.L());
  auto     _g = l.make_guarder();
  r.pushvalue();
  o << "{";
  if (l.is_type_string() || l.is_type_number() || l.is_type_boolean()) {
    o << l.type_name() << ": ";
  }
  o << luaL_tolstring(l.L(), -1, NULL) << ", ref_id: " << r.ref_id() << "}";
  return o;
}

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

// is array version std::unique_ptr

template <typename T>
struct is_std_unique_ptr_of_array : std::false_type {};

template <typename T, typename D>
struct is_std_unique_ptr_of_array<std::unique_ptr<T[], D>> : std::true_type {};

// is std::default_delete

template <typename T>
struct is_std_default_delete : std::false_type {};

template <typename T>
struct is_std_default_delete<std::default_delete<T>> : std::true_type {};

// get element_type

template <typename T, typename = void>
struct get_element_type {
  using type = void;
};

template <typename T>
struct get_element_type<T, void_t<typename T::element_type>> {
  using type = typename T::element_type;
};

// Get a C function pointer type by a member function pointer type

template <typename T>
struct get_cfptr_by_memfptr {
  using type = void;
};

template <typename Object, typename Return, typename... Args>
struct get_cfptr_by_memfptr<Return (Object::*)(Args...)> {
  using type = Return (*)(Args...);
};

template <typename Object, typename Return, typename... Args>
struct get_cfptr_by_memfptr<Return (Object::*)(Args...) const> {
  using type = Return (*)(Args...);
};

template <typename Object, typename Return, typename... Args>
struct get_cfptr_by_memfptr<Return (Object::*)(Args...) volatile> {
  using type = Return (*)(Args...);
};

template <typename Object, typename Return, typename... Args>
struct get_cfptr_by_memfptr<Return (Object::*)(Args...) const volatile> {
  using type = Return (*)(Args...);
};

template <typename Object, typename Return, typename... Args>
struct get_cfptr_by_memfptr<Return (Object::*)(Args..., ...)> {
  using type = Return (*)(Args..., ...);
};

template <typename Object, typename Return, typename... Args>
struct get_cfptr_by_memfptr<Return (Object::*)(Args..., ...) const> {
  using type = Return (*)(Args..., ...);
};

template <typename Object, typename Return, typename... Args>
struct get_cfptr_by_memfptr<Return (Object::*)(Args..., ...) volatile> {
  using type = Return (*)(Args..., ...);
};

template <typename Object, typename Return, typename... Args>
struct get_cfptr_by_memfptr<Return (Object::*)(Args..., ...) const volatile> {
  using type = Return (*)(Args..., ...);
};

#if PEACALM_LUAW_SUPPORT_CPP17
// Keep noexcept if C++17 supported

template <typename Object, typename Return, typename... Args>
struct get_cfptr_by_memfptr<Return (Object::*)(Args...) noexcept> {
  using type = Return (*)(Args...) noexcept;
};

template <typename Object, typename Return, typename... Args>
struct get_cfptr_by_memfptr<Return (Object::*)(Args...) const noexcept> {
  using type = Return (*)(Args...) noexcept;
};

template <typename Object, typename Return, typename... Args>
struct get_cfptr_by_memfptr<Return (Object::*)(Args...) volatile noexcept> {
  using type = Return (*)(Args...) noexcept;
};

template <typename Object, typename Return, typename... Args>
struct get_cfptr_by_memfptr<Return (Object::*)(Args...)
                                const volatile noexcept> {
  using type = Return (*)(Args...) noexcept;
};

template <typename Object, typename Return, typename... Args>
struct get_cfptr_by_memfptr<Return (Object::*)(Args..., ...) noexcept> {
  using type = Return (*)(Args..., ...) noexcept;
};

template <typename Object, typename Return, typename... Args>
struct get_cfptr_by_memfptr<Return (Object::*)(Args..., ...) const noexcept> {
  using type = Return (*)(Args..., ...) noexcept;
};

template <typename Object, typename Return, typename... Args>
struct get_cfptr_by_memfptr<Return (Object::*)(Args...,
                                               ...) volatile noexcept> {
  using type = Return (*)(Args..., ...) noexcept;
};

template <typename Object, typename Return, typename... Args>
struct get_cfptr_by_memfptr<Return (Object::*)(Args..., ...)
                                const volatile noexcept> {
  using type = Return (*)(Args..., ...) noexcept;
};

#endif

template <typename T>
using get_cfptr_by_memfptr_t = typename get_cfptr_by_memfptr<T>::type;

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
using detect_c_callee_t = get_cfptr_by_memfptr_t<detect_callee_t<T>>;

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

#if PEACALM_LUAW_SUPPORT_CPP17

template <typename Return, typename... Args>
struct is_cfunction<Return(Args...) noexcept> : std::true_type {};

template <typename Return, typename... Args>
struct is_cfunction<Return(Args..., ...) noexcept> : std::true_type {};

#endif

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

// remove_ptr_or_smart_ptr

template <typename T>
using remove_ptr_or_smart_ptr_t =
    std::conditional_t<is_std_shared_ptr<std::decay_t<T>>::value ||
                           is_std_unique_ptr<std::decay_t<T>>::value,
                       typename get_element_type<T>::type,
                       typename std::remove_pointer<T>::type>;

}  // namespace luaw_detail

//////////////////// pusher impl ///////////////////////////////////////////////

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

// primary pusher. guess whether it may be a lambda, push as function if true,
// otherwise push as an user defined custom object.
template <typename T, typename>
struct luaw::pusher {
  static_assert(!std::is_pointer<T>::value,
                "Never happen (Pointers should be specialized elsewhere)");
  static_assert(std::is_same<T, std::decay_t<T>>::value, "T should be decayed");
  static_assert(std::is_class<T>::value, "T should be a class type");

  static const size_t size = 1;

  template <typename Y>
  static int push(luaw& l, Y&& v) {
    static_assert(std::is_convertible<std::decay_t<Y>, T>::value,
                  "Y should convertible to T");

    // Guess whether it may be a lambda object, if it is, then push as a
    // function, otherwise push as custom class.
    return __push(l, std::forward<Y>(v), luaw_detail::decay_maybe_lambda<Y>{});
  }

private:
  // Push as a function.
  template <typename Y>
  static int __push(luaw& l, Y&& v, std::true_type) {
    return luaw::pusher<luaw::function_tag>::push(l, std::forward<Y>(v));
  }

  // Push as a full userdata.
  template <typename Y>
  static int __push(luaw& l, Y&& v, std::false_type) {
    using SolidY = std::remove_reference_t<Y>;
    using T0     = T;
    using T1     = std::
        conditional_t<std::is_const<SolidY>::value, std::add_const_t<T0>, T0>;
    using T2 = std::conditional_t<std::is_volatile<SolidY>::value,
                                  std::add_volatile_t<T1>,
                                  T1>;
    static_assert(std::is_same<T, std::decay_t<Y>>::value
                      ? std::is_same<T2, SolidY>::value
                      : true,
                  "Never happen");

    void* p = l.newuserdata(sizeof(T2));
    new (p) T2(std::forward<Y>(v));  // construct T by Y
    luaw::metatable_factory<T2>::push_shared_metatable(l);
    l.setmetatable(-2);
    return 1;
  }
};

// pointer to class
template <typename T>
struct luaw::pusher<
    T*,
    std::enable_if_t<!std::is_function<T>::value &&
                     !std::is_same<T*, const char*>::value &&
                     !luaw_detail::is_std_unique_ptr<std::decay_t<T>>::value &&
                     std::is_class<std::decay_t<T>>::value>> {
  static const size_t size = 1;

  static int push(luaw& l, T* v) {
    l.pushlightuserdata(
        reinterpret_cast<void*>(const_cast<std::remove_cv_t<T>*>(v)));

    luaw::metatable_factory<T*>::push_shared_metatable(l);
    l.setmetatable(-2);

    return 1;
  }
};

// non-class pointer, as lightuserdata
template <typename T>
struct luaw::pusher<
    T*,
    std::enable_if_t<!std::is_function<T>::value &&
                     !std::is_same<T*, const char*>::value &&
                     !luaw_detail::is_std_unique_ptr<std::decay_t<T>>::value &&
                     !std::is_class<std::decay_t<T>>::value>> {
  static const size_t size = 1;

  static int push(luaw& l, T* v) {
    l.pushlightuserdata(
        reinterpret_cast<void*>(const_cast<std::remove_cv_t<T>*>(v)));
    return 1;
  }
};

// pointer to std::unique_ptr<T, D>, share metatable with pointer to
// std::unique_ptr<T>
template <typename T>
struct luaw::pusher<
    T*,
    std::enable_if_t<luaw_detail::is_std_unique_ptr<std::decay_t<T>>::value>> {
  static const size_t size = 1;

  static int push(luaw& l, T* v) {
    using E = std::conditional_t<
        luaw_detail::is_std_unique_ptr_of_array<std::decay_t<T>>::value,
        typename T::element_type[],
        typename T::element_type>;
    using T0 = std::unique_ptr<E>;  // use default_deleter
    using T1 =
        std::conditional_t<std::is_const<T>::value, std::add_const_t<T0>, T0>;
    using T2 = std::
        conditional_t<std::is_volatile<T>::value, std::add_volatile_t<T1>, T1>;

    static_assert(
        luaw_detail::is_std_default_delete<typename T::deleter_type>::value
            ? std::is_same<T, T2>::value
            : true,
        "Never happen");

    l.pushlightuserdata(
        reinterpret_cast<void*>(const_cast<std::remove_cv_t<T>*>(v)));

    luaw::metatable_factory<T2*>::push_shared_metatable(l);
    l.setmetatable(-2);

    return 1;
  }
};

// std::unique_ptr<T, D>, not share metatable with std::unique_ptr<T>
template <typename T>
struct luaw::pusher<
    T,
    std::enable_if_t<luaw_detail::is_std_unique_ptr<std::decay_t<T>>::value>> {
  static_assert(std::is_same<T, std::decay_t<T>>::value, "T should be decayed");

  static const size_t size = 1;

  template <typename Y>
  static int push(luaw& l, Y&& v) {
    static_assert(luaw_detail::is_std_unique_ptr<std::decay_t<Y>>::value,
                  "Decayed Y should be std::unique_ptr");

    using SolidY = std::remove_reference_t<Y>;
    using E      = std::conditional_t<
        luaw_detail::is_std_unique_ptr_of_array<std::decay_t<T>>::value,
        typename T::element_type[],
        typename T::element_type>;
    using T0 = std::unique_ptr<E>;  // use default_deleter
    using T1 = std::
        conditional_t<std::is_const<SolidY>::value, std::add_const_t<T0>, T0>;
    using T2 = std::conditional_t<std::is_volatile<SolidY>::value,
                                  std::add_volatile_t<T1>,
                                  T1>;

    static_assert(
        luaw_detail::is_std_default_delete<typename T::deleter_type>::value
            ? std::is_same<T2, SolidY>::value
            : true,
        "Never happen");

    void* p = l.newuserdata(sizeof(SolidY));
    new (p) SolidY(std::forward<Y>(v));

    bool first_create = luaw::metatable_factory<SolidY>::gtouchmetatb(l);
    if (first_create) {
      luaw::metatable_factory<T2*>::set_metamethods(l);
      luaw::metatable_factory<SolidY>::set_gc_to_metatable(l);
    }
    l.setmetatable(-2);

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
  // SolidY is pointer type
  template <typename SolidY, typename Y>
  static void __push(luaw& l, Y&& v, std::true_type) {
    l.pushlightuserdata(reinterpret_cast<void*>(
        const_cast<std::remove_cv_t<std::remove_pointer_t<SolidY>>*>(v)));
    luaw::metatable_factory<SolidY>::push_shared_metatable(l);
    l.setmetatable(-2);
  }

  // SolidY is not pointer type
  template <typename SolidY, typename Y>
  static void __push(luaw& l, Y&& v, std::false_type) {
    void* p = l.newuserdata(sizeof(SolidY));
    new (p) SolidY(std::forward<Y>(v));
    luaw::metatable_factory<SolidY>::push_shared_metatable(l);
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
      l.clearL();
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
      l.clearL();
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
      l.clearL();
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

  // For type Return is void.
  template <typename Callee>
  static int callback(luaw& l, Callee&& c, int start_idx, std::true_type) {
    do_call(l, std::forward<Callee>(c), start_idx, 1, wrap<Args>{}...);
    return 0;
  }

  // For type Return is not void. Return the number of result.
  template <typename Callee>
  static int callback(luaw& l, Callee&& c, int start_idx, std::false_type) {
    return luaw::pusher_for_return<std::decay_t<Return>>::push(
        l, do_call(l, std::forward<Callee>(c), start_idx, 1, wrap<Args>{}...));
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
      // Never runs here.
      PEACALM_LUAW_ASSERT(false);
      // Do not return Return() to enable functions with reference results.
      // return Return();
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

// Variadic function, not support!
template <typename Return, typename... Args>
struct luaw::pusher<Return (*)(Args..., ...)> {
  // Let compile fail
  template <typename F>
  static int push(luaw& l, F&& f) = delete;
};

#if PEACALM_LUAW_SUPPORT_CPP17

// Function with noexcept-specification after C++17
template <typename Return, typename... Args>
struct luaw::pusher<Return (*)(Args...) noexcept> {
  using Basic = luaw::pusher<Return (*)(Args...)>;

  template <typename F>
  static int push(luaw& l, F&& f) {
    return Basic::push(l, std::forward<F>(f));
  }

  // Explicitly call C function pointer version of push
  static int push(luaw& l, Return (*f)(Args...) noexcept) {
    return Basic::push(l, static_cast<Return (*)(Args...)>(f));
  }
};

// Variadic function, not support
template <typename Return, typename... Args>
struct luaw::pusher<Return (*)(Args..., ...) noexcept> {
  // Let compile fail
  template <typename F>
  static int push(luaw& l, F&& f) = delete;
};

#endif

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

// std::tuple, as a table list
template <typename... Ts>
struct luaw::pusher<std::tuple<Ts...>> {
  static const size_t size = 1;

  static int push(luaw& l, const std::tuple<Ts...>& v) {
    l.newtable();
    const size_t N = std::tuple_size<std::tuple<Ts...>>::value;
    __push<0, N>(l, v, std::integral_constant<bool, 0 < N>{});
    return 1;
  }

private:
  template <size_t I, size_t N, typename T>
  static void __push(luaw& l, const T& t, std::true_type) {
    l.push(std::get<I>(t));
    l.rawseti(-2, I + 1);
    __push<I + 1, N>(l, t, std::integral_constant<bool, I + 1 < N>{});
  }
  template <size_t I, size_t N, typename T>
  static void __push(luaw& l, const T& t, std::false_type) {}
};

template <>
struct luaw::pusher<luaw::luavalueidx> {
  static const size_t size = 1;

  static int push(luaw& l, const luaw::luavalueidx& r) {
    if (!r.valid()) {
      l.pushnil();
    } else if (l.L() == r.L()) {
      l.pushvalue(r.idx());
    } else if (l.main_thread() == r.main_thread()) {
      lua_pushvalue(r.L(), r.idx());
      lua_xmove(r.L(), l.L(), 1);
    } else {
      // Can't push value from other lua_State.
      l.pushnil();
    }
    return 1;
  }
};

template <>
struct luaw::pusher<luaw::luavalueref> {
  static const size_t size = 1;

  static int push(luaw& l, const luaw::luavalueref& r) {
    if (r.as_nil()) {
      l.pushnil();
    } else if (l.L() == r.L() || l.main_thread() == r.main_thread()) {
      lua_rawgeti(l.L(), LUA_REGISTRYINDEX, r.ref_id());
    } else {
      // Can't push value from other lua_State.
      l.pushnil();
    }
    return 1;
  }
};

//////////////////// pusher_for_return impl ////////////////////////////////////

template <typename T>
struct luaw::pusher_for_return : public luaw::pusher<T> {
  static_assert(std::is_same<T, std::decay_t<T>>::value, "T should be decayed");
};

// std::tuple
// Push elements separately in order. One element takes one place in stack.
template <typename... Ts>
struct luaw::pusher_for_return<std::tuple<Ts...>> {
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
    int x = l.push(std::get<I>(t));
    int y = __push<I + 1, N>(l, t, std::integral_constant<bool, I + 1 < N>{});
    return x + y;
  }

  template <size_t I, size_t N, typename T>
  static int __push(luaw& l, const T& t, std::false_type) {
    return 0;
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

namespace luaw_detail {

// Has any reference type in a std::tuple
template <typename T>
struct tuple_has_ref : std::false_type {};

template <typename T, typename... Ts>
struct tuple_has_ref<std::tuple<T, Ts...>>
    : public std::integral_constant<
          bool,
          std::is_reference<T>::value ||
              tuple_has_ref<std::tuple<Ts...>>::value> {};

template <>
struct tuple_has_ref<std::tuple<>> : std::false_type {};

// Has any reference type in Ts
template <typename... Ts>
struct has_ref : public tuple_has_ref<std::tuple<Ts...>> {};

}  // namespace luaw_detail

template <typename Return, typename... Args>
class luaw::function<Return(Args...)> {
  static_assert(
      !luaw_detail::has_ref<Args...>::value,
      "Do not support reference type as luaw::function's argument. "
      ""
      "If using reference as argument, it will make a copy of the referenced "
      "argument into Lua, won't implicitly take its address, meaning that it "
      "will not share the same object in Lua with that in C++, and this "
      "behaves differently with that in C++. This may confuse users, so "
      "explicitly forbid it. "
      ""
      "Directly using the underlying type if you want to make a copy of the "
      "argument into Lua. "
      "Or if you want to share the same argument objects in Lua with C++, "
      "so you can modify them in Lua, you can use raw pointer type if there is "
      "only one kind of raw pointer type in all arguments, or use smart "
      "pointer type or peacalm::luaw::ptrw type, and these are safer and more "
      "reassuring.");
  static_assert(!std::is_reference<Return>::value &&
                    !luaw_detail::tuple_has_ref<Return>::value,
                "Do not support reference type as luaw::function's result. "
                "Cannot make a C++ reference to a value in Lua");

  // component
  lua_State*                 L_ = nullptr;
  std::shared_ptr<const int> ref_sptr_;

  // parameters put in
  bool disable_log_ = false;

  // states put out
  mutable bool function_failed_ = false, function_exists_ = false,
               result_failed_ = false, result_exists_ = false;
  mutable int real_result_size_ = 0;

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
    function_exists_ = !lua_isnoneornil(L_, idx);
    if (exists) *exists = !lua_isnoneornil(L_, idx);
    if (failed) {
      if (lua_isnoneornil(L_, idx)) {
        // noneornil is not callable, so we regard this (not-exists) as failed
        // for function, which is not same as getting variabls.
        *failed = true;
      } else {
        // check more whether it's callable
        fakeluaw l(L);
        *failed = !l.callable(idx);
      }
    }

    lua_pushvalue(L_, idx);
    const int ref_id = luaL_ref(L, LUA_REGISTRYINDEX);
    ref_sptr_.reset(new int(ref_id), [L](const int* p) {
      luaL_unref(L, LUA_REGISTRYINDEX, *p);
      delete p;
    });
  }

  /// Unref the referenced Lua function value.
  void unref() { ref_sptr_.reset(); }

  /// Get the ref id for the referenced Lua function value.
  const int ref_id() const { return ref_sptr_ ? *ref_sptr_ : LUA_NOREF; }

  /// Get internal lua_State.
  lua_State* L() const { return L_; }

  /// Set log on-off.
  void disable_log(bool v) { disable_log_ = v; }

  /// Return a C string text message to indicate current state.
  const char* state_msg() const {
    if (!failed()) return "No fail";
    if (function_failed()) return "Function failed";
    if (!function_exists()) return "Function not exist";
    if (result_failed()) return "Result failed";
    if (!result_enough()) {
      if (!result_exists()) return "Result not exist";
      return "Result not enough";
    }
    return "Unknown";
  }

  // states after function call
  // @{

  /// Whether the whole process failed.
  /// Any step fails the whole process fails, including running the function in
  /// Lua and converting results to C++, and whether get enough results.
  bool failed() const {
    return function_failed() || !function_exists() || result_failed() ||
           !result_enough();
  }

  /// Whether the function failed while running in Lua (not including function
  /// doesn't exist)
  bool function_failed() const { return function_failed_; }

  /// Whether the function exists in Lua
  bool function_exists() const { return function_exists_; }

  /// Whether converting the Lua function's results to C++ failed
  bool result_failed() const { return result_failed_; }

  /// Whether the Lua function returns results (whether results exist)
  bool result_exists() const { return result_exists_; }

  /// Whether Lua function returns enough results.
  /// Could return more than expect, but couldn't less.
  bool result_enough() const {
    return real_result_size() >= expected_result_size();
  }

  /// Result number the Lua function returned
  int real_result_size() const { return real_result_size_; }

  // @}

  /// Result number we expect the Lua function should return.
  /// Could return more, couldn't less.
  constexpr int expected_result_size() const {
    return luaw::pusher_for_return<std::decay_t<Return>>::size;
  }

  Return operator()(const Args&... args) const {
    // reset all states first
    function_failed_  = false;
    function_exists_  = false;
    result_failed_    = false;
    result_exists_    = false;
    real_result_size_ = 0;

    if (!L_ || !ref_sptr_) {
      function_failed_ = false;
      function_exists_ = false;
      if (!disable_log_) {
        if (!L_) {
          luaw::log_error("luaw::function has no lua_State");
        } else if (!ref_sptr_) {
          luaw::log_error("luaw::function refers to nothing");
        } else {
          PEACALM_LUAW_ASSERT(false);
        }
      }
      return Return();
    }

    fakeluaw l(L_);
    auto     _g = l.make_guarder();
    int      sz = l.gettop();
    l.rawgeti(LUA_REGISTRYINDEX, *ref_sptr_);
    if (l.isnoneornil()) {
      function_failed_ = false;
      function_exists_ = false;
      l.pop();
      if (!disable_log_) { luaw::log_error("calling an inexistent function"); }
      return Return();
    } else {
      function_exists_ = true;
    }

    int narg      = push_args(l, args...);
    int pcall_ret = l.pcall(narg, LUA_MULTRET, 0);
    PEACALM_LUAW_ASSERT(l.gettop() >= sz);

    if (pcall_ret == LUA_OK) {
      function_failed_  = false;
      real_result_size_ = l.gettop() - sz;
    } else {
      function_failed_ = true;
      if (!disable_log_) { l.log_error_in_stack(); }
      l.pop();
      return Return();
    }

    return luaw::convertor_for_return<std::decay_t<Return>>::to(
        l, sz + 1, disable_log_, &result_failed_, &result_exists_);
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

namespace luaw_detail {

template <typename T>
T __to_list(luaw&       l,
            int         idx         = -1,
            bool        disable_log = false,
            bool*       failed      = nullptr,
            bool*       exists      = nullptr,
            const char* tname       = "list") {
  using value_type = typename T::value_type;
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
  int sz = luaL_len(l.L(), idx);
  for (int i = 1; i <= sz; ++i) {
    l.geti(idx, i);
    bool       subfailed, subexists;
    value_type subret =
        l.to<value_type>(-1, disable_log, &subfailed, &subexists);
    // Only add elements exist and conversion succeeded
    if (!subfailed && subexists) ret.push_back(std::move(subret));
    if (subfailed && failed) *failed = true;
    l.pop();
  }
  return ret;
}

// Convert all keys of a Lua table into a C++ set
template <typename T>
T __to_set(luaw&       l,
           int         idx         = -1,
           bool        disable_log = false,
           bool*       failed      = nullptr,
           bool*       exists      = nullptr,
           const char* tname       = "set") {
  static_assert(!std::is_same<typename T::key_type, const char*>::value,
                "const char* as key type of set is forbidden");

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
    bool kfailed, kexists;
    auto key = l.to<typename T::key_type>(-2, disable_log, &kfailed, &kexists);
    if (!kfailed && kexists) { ret.insert(std::move(key)); }
    if (kfailed && failed) *failed = true;
    l.pop();
  }
  return ret;
}

template <typename T>
T __to_map(luaw&       l,
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
    return luaw_detail::__to_list<result_t>(
        l, idx, disable_log, failed, exists, "vector");
  }
};

// to std::deque
template <typename T, typename Allocator>
struct luaw::convertor<std::deque<T, Allocator>> {
  using result_t = std::deque<T, Allocator>;
  static result_t to(luaw& l,
                     int   idx         = -1,
                     bool  disable_log = false,
                     bool* failed      = nullptr,
                     bool* exists      = nullptr) {
    return luaw_detail::__to_list<result_t>(
        l, idx, disable_log, failed, exists, "deque");
  }
};

// to std::list
template <typename T, typename Allocator>
struct luaw::convertor<std::list<T, Allocator>> {
  using result_t = std::list<T, Allocator>;
  static result_t to(luaw& l,
                     int   idx         = -1,
                     bool  disable_log = false,
                     bool* failed      = nullptr,
                     bool* exists      = nullptr) {
    return luaw_detail::__to_list<result_t>(
        l, idx, disable_log, failed, exists, "list");
  }
};

// to std::forward_list
template <typename T, typename Allocator>
struct luaw::convertor<std::forward_list<T, Allocator>> {
  using result_t = std::forward_list<T, Allocator>;
  static result_t to(luaw& l,
                     int   idx         = -1,
                     bool  disable_log = false,
                     bool* failed      = nullptr,
                     bool* exists      = nullptr) {
    auto t = luaw_detail::__to_list<std::vector<T>>(
        l, idx, disable_log, failed, exists, "vector");
    return result_t(t.begin(), t.end());
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
    return luaw_detail::__to_set<result_t>(
        l, idx, disable_log, failed, exists, "set");
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
    return luaw_detail::__to_set<result_t>(
        l, idx, disable_log, failed, exists, "unordered_set");
  }
};

// to std::map
template <typename Key, typename Compare, typename Allocator>
struct luaw::convertor<std::map<Key, Compare, Allocator>> {
  using result_t = std::map<Key, Compare, Allocator>;
  static result_t to(luaw& l,
                     int   idx         = -1,
                     bool  disable_log = false,
                     bool* failed      = nullptr,
                     bool* exists      = nullptr) {
    return luaw_detail::__to_map<result_t>(
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
    return luaw_detail::__to_map<result_t>(
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
    result_t ret;
    if (l.isnoneornil(idx)) {
      if (exists) *exists = false;
      if (failed) *failed = false;
      return ret;
    }
    if (exists) *exists = true;
    if (!l.istable(idx)) {
      if (failed) *failed = true;
      return ret;
    }

    constexpr size_t N = std::tuple_size<result_t>::value;
    __to<0, N>(ret,
               std::integral_constant<bool, 0 < N>{},
               l,
               l.abs_index(idx),
               disable_log,
               failed);
    return ret;
  }

private:
  template <size_t I, size_t N, typename T>
  static void __to(T& ret,
                   std::true_type,
                   luaw& l,
                   int   idx,
                   bool  disable_log = false,
                   bool* failed      = nullptr) {
    l.seek(I + 1, idx);

    bool thisfailed;
    std::get<I>(ret) =
        l.to<std::tuple_element_t<I, T>>(-1, disable_log, &thisfailed);

    l.pop();

    bool restfailed;
    __to<I + 1, N>(ret,
                   std::integral_constant<bool, I + 1 < N>{},
                   l,
                   idx,
                   disable_log,
                   &restfailed);

    if (failed) *failed = thisfailed || restfailed;
  }

  template <size_t I, size_t N, typename T>
  static void __to(T& ret,
                   std::false_type,
                   luaw& l,
                   int   idx         = -1,
                   bool  disable_log = false,
                   bool* failed      = nullptr) {
    if (failed) *failed = false;
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
    return luaw::luavalueref(l.L(), idx);
  }
};

//////////////////// convertor_for_return impl /////////////////////////////////

template <typename T>
struct luaw::convertor_for_return : public luaw::convertor<T> {};

// to std::tuple, each element in tuple is converted from one index in stack.
template <typename... Ts>
struct luaw::convertor_for_return<std::tuple<Ts...>> {
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

//////////////////// metatable_factory impl ////////////////////////////////////

namespace luaw_detail {

template <typename T, typename Derived>
struct metatable_factory_base {
  // Touch (push onto stack) metatable by type T, create a new metatable if it
  // doesn't exist. Return whether created a new one.
  static bool gtouchmetatb(luaw& l) {
    return gtouchmetatb(l, std::is_pointer<T>{});
  }

  // For: T is pointer type
  static bool gtouchmetatb(luaw& l, std::true_type) {
    // Make metatable for lightuserdata have a different name with userdata.
    auto name = std::string("&") + std::string(typeid(T).name());
    return l.gtouchmetatb(name.c_str());
  }

  // For: T is not pointer type
  static bool gtouchmetatb(luaw& l, std::false_type) {
    // Since no-cv T and cv-qualified T have same name using typeid,
    // so make an unique type name by their pointer.
    return l.gtouchmetatb(typeid(T*).name());
  }

  // Objects with same type (cv- is concerned) share a common metatable
  static void push_shared_metatable(luaw& l) {
    bool first_create = gtouchmetatb(l, std::is_pointer<T>{});
    // Only build metatable once to improve performance
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
    set_index_to_metatable(l, -1);
    set_newindex_to_metatable(l, -1);
  }

  static void set_index_to_metatable(luaw& l, int idx = -1) {
    l.setkv("__index", __index, idx);
  }

  static void set_newindex_to_metatable(luaw& l, int idx = -1) {
    l.setkv("__newindex", __newindex, idx);
  }

  static int __index(lua_State* L) {
    fakeluaw l(L);
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
    if PEACAML_LUAW_IF_CONSTEXPR (
        (luaw_detail::is_std_shared_ptr<std::decay_t<T>>::value ||
         luaw_detail::is_std_unique_ptr<std::decay_t<T>>::value)
            ? std::is_const<
                  typename luaw_detail::get_element_type<T>::type>::value
            : std::is_const<T>::value) {
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
    if PEACAML_LUAW_IF_CONSTEXPR (
        (luaw_detail::is_std_shared_ptr<std::decay_t<T>>::value ||
         luaw_detail::is_std_unique_ptr<std::decay_t<T>>::value)
            ? std::is_volatile<
                  typename luaw_detail::get_element_type<T>::type>::value
            : std::is_volatile<T>::value) {
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

    // dynamic member getter
    l.rawgeti(-1, luaw::member_info_fields::dynamic_member_getter);
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
    fakeluaw l(L);
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
        return luaL_error(l.L(), "Const member cannot be modified: %s", key);
      } else {
        l.pop(2);
      }
    }

    // dynamic member setter
    l.rawgeti(-1, luaw::member_info_fields::dynamic_member_setter);
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
    if (!std::is_trivially_destructible<T>::value) {
      set_gc_to_metatable(l, -1);
    }
  }

  static void set_gc_to_metatable(luaw& l, int idx = -1) {
    l.setkv("__gc", __gc, idx);
  }

  static int __gc(lua_State* L) {
    fakeluaw l(L);
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

// retrieve_underlying_ptr

template <typename T>
struct __retrieve_underlying_ptr {
  template <typename U>
  auto operator()(U&& t) const {
    return &t;
  }
};
template <typename T>
struct __retrieve_underlying_ptr<T*> {
  auto operator()(T* t) const { return t; }
};
template <typename T>
struct __retrieve_underlying_ptr<std::shared_ptr<T>> {
  auto operator()(const std::shared_ptr<T>& t) const { return t.get(); }
};
template <typename T, typename D>
struct __retrieve_underlying_ptr<std::unique_ptr<T, D>> {
  auto operator()(const std::unique_ptr<T, D>& t) const { return t.get(); }
};

template <typename T>
auto retrieve_underlying_ptr(T&& t) {
  return __retrieve_underlying_ptr<std::decay_t<T>>{}(std::forward<T>(t));
}

}  // namespace luaw_detail

// The first argument of CallableObject must be raw pointer, and could be
// `auto*` (in lambda).
template <typename CallableObject>
class luaw::mock_mem_fn<
    CallableObject,
    std::enable_if_t<!std::is_member_pointer<CallableObject>::value>> {
  CallableObject o;

public:
  mock_mem_fn(CallableObject&& r) : o(std::forward<CallableObject>(r)) {}

  template <typename FirstArg, typename... Args>
  decltype(auto) operator()(FirstArg&& f, Args&&... args) const {
    return o(luaw_detail::retrieve_underlying_ptr(std::forward<FirstArg>(f)),
             std::forward<Args>(args)...);
  }
};

template <typename MemberPointer>
class luaw::mock_mem_fn<
    MemberPointer,
    std::enable_if_t<std::is_member_pointer<MemberPointer>::value>> {
  decltype(std::mem_fn(static_cast<MemberPointer>(nullptr))) o;

public:
  mock_mem_fn(MemberPointer p) : o(std::mem_fn(p)) {}

  template <typename FirstArg, typename... Args>
  decltype(auto) operator()(FirstArg&& f, Args&&... args) const {
    return o(luaw_detail::retrieve_underlying_ptr(std::forward<FirstArg>(f)),
             std::forward<Args>(args)...);
  };
};

//////////////////// static_mem_fn impl ////////////////////////////////////////

// specialize for C function pointer
template <typename T>
class luaw::static_mem_fn<T*, std::enable_if_t<std::is_function<T>::value>> {
  T* f;

public:
  static_mem_fn(T* t) : f(t) {
    // Can't be null ptr, since we're required to call the function ptr
    PEACALM_LUAW_ASSERT(f);
  }

  template <typename Class, typename... Args>
  decltype(auto) operator()(Class&&, Args&&... args) const {
    return f(std::forward<Args>(args)...);
  }
};

// specialize for C variable pointer
template <typename T>
class luaw::static_mem_fn<T*, std::enable_if_t<!std::is_function<T>::value>> {
  T* m;

public:
  static_mem_fn(T* t) : m(t) {
    // Can't be null ptr, since we're required to dereference the ptr
    PEACALM_LUAW_ASSERT(m);
  }

  template <typename Class>
  T& operator()(Class&&) const {
    return *m;
  }
};

//////////////////// registrar impl ////////////////////////////////////////////

template <typename Getter>
void luaw::register_dynamic_member_getter(Getter&& getter) {
  luaw::registrar<luaw_detail::detect_callable_cfunction_t<Getter>>::
      register_dynamic_member_getter(
          *this, luaw::mock_mem_fn<Getter>(std::forward<Getter>(getter)));
}

template <typename Setter>
void luaw::register_dynamic_member_setter(Setter&& setter) {
  luaw::registrar<luaw_detail::detect_callable_cfunction_t<Setter>>::
      register_dynamic_member_setter(
          *this, luaw::mock_mem_fn<Setter>(std::forward<Setter>(setter)));
}

// Support nothing in this basic registrar.
// Supported features should be specialized elsewhere.
template <typename T, typename>
struct luaw::registrar {
  // Explicitly delete the member functions to let compile fail for unsupported
  // member types, such as ref- qualified member functions, or C style variadic
  // functions.

  template <typename MemberFunction>
  static void register_member(luaw&          l,
                              const char*    fname,
                              MemberFunction mf) = delete;

  template <typename Getter>
  static void register_dynamic_member_getter(luaw& l, Getter&& getter) = delete;

  template <typename Setter>
  static void register_dynamic_member_setter(luaw& l, Setter&& setter) = delete;

  template <typename F>
  static void register_member_ptr(luaw& l, const char* name, F&& f) = delete;

  template <typename F>
  static void register_member_ref(luaw& l, const char* name, F&& f) = delete;
};

/* -------------------------------------------------------------------------- */
// Registrar specialization for dynamic member getter.
template <typename Member, typename Class, typename Key>
struct luaw::registrar<Member (*)(Class*, Key)> {
  static_assert(std::is_class<Class>::value && std::is_const<Class>::value,
                "First argument should be pointer of const class");
  using DecayClass = std::remove_cv_t<Class>;
  static_assert(std::is_same<DecayClass, std::decay_t<Class>>::value,
                "Never happen");

#define REGISTER_GETTER(ObjectType)                          \
  l.touchtb((void*)(&typeid(ObjectType)), LUA_REGISTRYINDEX) \
      .setkv<Member (*)(Class*, Key)>(                       \
          luaw::member_info_fields::dynamic_member_getter, getter);

#define REGISTER_SMART_GETTER(ObjectType)                                      \
  l.touchtb((void*)(&typeid(ObjectType)), LUA_REGISTRYINDEX)                   \
      .setkv<Member (*)(ObjectType, Key)>(                                     \
          luaw::member_info_fields::dynamic_member_getter,                     \
          [=, &l](ObjectType o, Key k) -> Member {                             \
            PEACALM_LUAW_ASSERT(o);                                            \
            auto p = luaw_detail::retrieve_underlying_ptr(*o);                 \
            if (!p) {                                                          \
              luaL_error(l.L(), "Getting dynamic member by empty smart ptr."); \
              /* Never runs here */                                            \
              PEACALM_LUAW_ASSERT(false);                                      \
            }                                                                  \
            return getter(*p, k);                                              \
          });

  template <typename Getter>
  static void register_dynamic_member_getter(luaw& l, Getter&& getter) {
    auto _g = l.make_guarder();

    REGISTER_GETTER(DecayClass*);
    REGISTER_GETTER(const DecayClass*);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

    if PEACAML_LUAW_IF_CONSTEXPR (std::is_volatile<Class>::value) {
      REGISTER_GETTER(volatile DecayClass*);
      REGISTER_GETTER(const volatile DecayClass*);
    }

#endif

    // Do not support nested smart pointers, which is meaningless!
    // DO NOT support top-level volatile for smart pointers!
    if PEACAML_LUAW_IF_CONSTEXPR (!luaw_detail::is_std_shared_ptr<
                                      DecayClass>::value &&
                                  !luaw_detail::is_std_unique_ptr<
                                      DecayClass>::value) {
      REGISTER_SMART_GETTER(std::shared_ptr<DecayClass>*);
      REGISTER_SMART_GETTER(const std::shared_ptr<DecayClass>*);
      REGISTER_SMART_GETTER(std::shared_ptr<const DecayClass>*);
      REGISTER_SMART_GETTER(const std::shared_ptr<const DecayClass>*);

      REGISTER_SMART_GETTER(std::unique_ptr<DecayClass>*);
      REGISTER_SMART_GETTER(const std::unique_ptr<DecayClass>*);
      REGISTER_SMART_GETTER(std::unique_ptr<const DecayClass>*);
      REGISTER_SMART_GETTER(const std::unique_ptr<const DecayClass>*);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

      // for low-level volatile:
      __register_dynamic_member_getter_for_volatile(
          l, std::forward<Getter>(getter), std::is_volatile<Class>{});

#endif
    }
  }

  template <typename Getter>
  static void __register_dynamic_member_getter_for_volatile(luaw&    l,
                                                            Getter&& getter,
                                                            std::true_type) {
    auto _g = l.make_guarder();

    REGISTER_SMART_GETTER(std::shared_ptr<volatile DecayClass>*);
    REGISTER_SMART_GETTER(const std::shared_ptr<volatile DecayClass>*);
    REGISTER_SMART_GETTER(std::shared_ptr<const volatile DecayClass>*);
    REGISTER_SMART_GETTER(const std::shared_ptr<const volatile DecayClass>*);

    REGISTER_SMART_GETTER(std::unique_ptr<volatile DecayClass>*);
    REGISTER_SMART_GETTER(const std::unique_ptr<volatile DecayClass>*);
    REGISTER_SMART_GETTER(std::unique_ptr<const volatile DecayClass>*);
    REGISTER_SMART_GETTER(const std::unique_ptr<const volatile DecayClass>*);
  }

  template <typename Getter>
  static void __register_dynamic_member_getter_for_volatile(luaw&    l,
                                                            Getter&& getter,
                                                            std::false_type) {
    // Do nothing.
    // Since the getter user provided cannot be used for volatile objects.
    // We donot make unnecessary troubles.
  }

#undef REGISTER_SMART_GETTER
#undef REGISTER_GETTER
};

/* -------------------------------------------------------------------------- */
// Registrar specialization for dynamic member setter.
template <typename Class, typename Key, typename Member>
struct luaw::registrar<void (*)(Class*, Key, Member)> {
  static_assert(std::is_class<Class>::value && !std::is_const<Class>::value,
                "First argument should be pointer of non-const class");
  using DecayClass = std::remove_cv_t<Class>;
  static_assert(std::is_same<DecayClass, std::decay_t<Class>>::value,
                "Never happen");

#define REGISTER_SETTER(ObjectType)                          \
  l.touchtb((void*)(&typeid(ObjectType)), LUA_REGISTRYINDEX) \
      .setkv<void (*)(Class*, Key, Member)>(                 \
          luaw::member_info_fields::dynamic_member_setter, setter);

#define REGISTER_SMART_SETTER(ObjectType)                                      \
  l.touchtb((void*)(&typeid(ObjectType)), LUA_REGISTRYINDEX)                   \
      .setkv<void (*)(ObjectType, Key, Member)>(                               \
          luaw::member_info_fields::dynamic_member_setter,                     \
          [=, &l](ObjectType o, Key k, Member v) {                             \
            PEACALM_LUAW_ASSERT(o);                                            \
            auto p = luaw_detail::retrieve_underlying_ptr(*o);                 \
            if (!p) {                                                          \
              luaL_error(l.L(), "Setting dynamic member by empty smart ptr."); \
              /* Never runs here */                                            \
              PEACALM_LUAW_ASSERT(false);                                      \
            }                                                                  \
            setter(*p, k, v);                                                  \
          });

#define REGISTER_SETTER_OF_CONST(ObjectType)                 \
  l.touchtb((void*)(&typeid(ObjectType)), LUA_REGISTRYINDEX) \
      .setkv(luaw::member_info_fields::dynamic_member_setter, false);

  template <typename Setter>
  static void register_dynamic_member_setter(luaw& l, Setter&& setter) {
    auto _g = l.make_guarder();

    REGISTER_SETTER(DecayClass*);
    REGISTER_SETTER_OF_CONST(const DecayClass*);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

    if PEACAML_LUAW_IF_CONSTEXPR (std::is_volatile<Class>::value) {
      REGISTER_SETTER(volatile DecayClass*);
      REGISTER_SETTER_OF_CONST(const volatile DecayClass*);
    }

#endif

    // Do not support nested smart pointers, which is meaningless!
    // DO NOT support top-level volatile for smart pointers!
    if PEACAML_LUAW_IF_CONSTEXPR (!luaw_detail::is_std_shared_ptr<
                                      Class>::value &&
                                  !luaw_detail::is_std_unique_ptr<
                                      Class>::value) {
      REGISTER_SMART_SETTER(std::shared_ptr<DecayClass>*);
      REGISTER_SMART_SETTER(const std::shared_ptr<DecayClass>*);

      REGISTER_SETTER_OF_CONST(std::shared_ptr<const DecayClass>*);
      REGISTER_SETTER_OF_CONST(const std::shared_ptr<const DecayClass>*);

      REGISTER_SMART_SETTER(std::unique_ptr<DecayClass>*);
      REGISTER_SMART_SETTER(const std::unique_ptr<DecayClass>*);

      REGISTER_SETTER_OF_CONST(std::unique_ptr<const DecayClass>*);
      REGISTER_SETTER_OF_CONST(const std::unique_ptr<const DecayClass>*);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

      // for low-level volatile:
      __register_dynamic_member_setter_for_volatile(
          l, std::forward<Setter>(setter), std::is_volatile<Class>{});

#endif
    }
  }

  template <typename Setter>
  static void __register_dynamic_member_setter_for_volatile(luaw&    l,
                                                            Setter&& setter,
                                                            std::true_type) {
    auto _g = l.make_guarder();

    REGISTER_SMART_SETTER(std::shared_ptr<volatile DecayClass>*);
    REGISTER_SMART_SETTER(const std::shared_ptr<volatile DecayClass>*);

    REGISTER_SETTER_OF_CONST(std::shared_ptr<const volatile DecayClass>*);
    REGISTER_SETTER_OF_CONST(const std::shared_ptr<const volatile DecayClass>*);

    REGISTER_SMART_SETTER(std::unique_ptr<volatile DecayClass>*);
    REGISTER_SMART_SETTER(const std::unique_ptr<volatile DecayClass>*);

    REGISTER_SETTER_OF_CONST(std::unique_ptr<const volatile DecayClass>*);
    REGISTER_SETTER_OF_CONST(const std::unique_ptr<const volatile DecayClass>*);
  }

  template <typename Setter>
  static void __register_dynamic_member_setter_for_volatile(luaw&    l,
                                                            Setter&& setter,
                                                            std::false_type) {
    // Do nothing.
    // Since the setter user provided cannot be used for volatile objects.
    // We donot make unnecessary troubles.
  }

#undef REGISTER_SETTER_OF_CONST
#undef REGISTER_SMART_SETTER
#undef REGISTER_SETTER
};

/* -------------------------------------------------------------------------- */
// Make wrapper for raw pointer.

namespace luaw_detail {

// A mock allocator to support volatile
template <typename T>
struct allocator {
  using value_type = T;
  using size_type  = std::size_t;

  allocator() = default;

  template <typename U>
  allocator(const allocator<U>& r) {}

  value_type* allocate(size_type n) {
    if (n == 0) { return nullptr; }
    if (n > max_size()) { throw std::bad_array_new_length(); }
    void* const p = ::operator new(n * sizeof(value_type));
    if (!p) { throw std::bad_alloc(); }
    return static_cast<value_type*>(p);
  }

  void deallocate(const value_type* p, size_type n) {
    return ::operator delete((void*)(p));
  }

  size_type max_size() const { return size_type(~0) / sizeof(T); }
};

// To make a shared_ptr with a given pointer,
// but never deletes the given pointer,
// and uses an allocator who supports volatile.
template <typename T>
std::shared_ptr<T> mock_shared(T* p) {
  return std::shared_ptr<T>(
      p, [](...) {}, allocator<T>{});
}

}  // namespace luaw_detail

// Make a wrapper for a raw pointer.
// Thus the wrapped pointer will be a full userdata in Lua, not light userdata.
template <typename T>
luaw::ptrw<T> luaw::make_ptrw(T* p) {
  static_assert(!luaw_detail::is_std_shared_ptr<std::decay_t<T>>::value &&
                    !luaw_detail::is_std_unique_ptr<std::decay_t<T>>::value,
                "No need to make wrapper for smart pointers");
  return luaw_detail::mock_shared<T>(p);
}

/* -------------------------------------------------------------------------- */
// Registrar specialization for register_member_ptr and register_member_ref.
template <typename Class, typename Member>
struct luaw::registrar<Member Class::*, luaw::registrar_tag_for_member_ptr> {
  static_assert(std::is_member_object_pointer<Member Class::*>::value,
                "Only member variables can be registered ptr/ref");

  // Actually this static assertion won't work.
  // C++ regards "Member cv-Class::*" as a same type as "Member Class::*".
  // And the compiler will automatically remove cv- property for Class.
  static_assert(std::is_same<Class, std::decay_t<Class>>::value,
                "Never happen (Class must be decayed)");

  static_assert(!std::is_pointer<Member>::value,
                "No need to register pointer or reference for pointer members");
  static_assert(
      !luaw_detail::is_std_shared_ptr<std::decay_t<Member>>::value &&
          !luaw_detail::is_std_unique_ptr<std::decay_t<Member>>::value,
      "No need to register pointer or reference for smart pointer members");

  template <typename F>
  static void register_member_ptr(luaw& l, const char* name, F&& f) {
    // Member pointer should have same low-level cv- property as class pointer.

    using Registrar = luaw::registrar<Member Class::*>;

    Registrar::template do_register_one_getter<Class*, Member* const>(
        l, name, f);
    Registrar::template do_register_one_getter<const Class*,
                                               const Member* const>(l, name, f);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT
    Registrar::template do_register_one_getter<volatile Class*,
                                               volatile Member* const>(
        l, name, f);
    Registrar::template do_register_one_getter<const volatile Class*,
                                               const volatile Member* const>(
        l, name, f);
#endif

    Registrar::template do_register_one_getter<std::shared_ptr<Class>*,
                                               Member* const>(l, name, f);
    Registrar::template do_register_one_getter<std::shared_ptr<const Class>*,
                                               const Member* const>(l, name, f);

    Registrar::template do_register_one_getter<const std::shared_ptr<Class>*,
                                               Member* const>(l, name, f);
    Registrar::template do_register_one_getter<
        const std::shared_ptr<const Class>*,
        const Member* const>(l, name, f);

    Registrar::template do_register_one_getter<std::unique_ptr<Class>*,
                                               Member* const>(l, name, f);
    Registrar::template do_register_one_getter<std::unique_ptr<const Class>*,
                                               const Member* const>(l, name, f);

    Registrar::template do_register_one_getter<const std::unique_ptr<Class>*,
                                               Member* const>(l, name, f);
    Registrar::template do_register_one_getter<
        const std::unique_ptr<const Class>*,
        const Member* const>(l, name, f);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT
    Registrar::template do_register_one_getter<std::shared_ptr<volatile Class>*,
                                               volatile Member* const>(
        l, name, f);
    Registrar::template do_register_one_getter<
        std::shared_ptr<const volatile Class>*,
        const volatile Member* const>(l, name, f);

    Registrar::template do_register_one_getter<
        const std::shared_ptr<volatile Class>*,
        volatile Member* const>(l, name, f);
    Registrar::template do_register_one_getter<
        const std::shared_ptr<const volatile Class>*,
        const volatile Member* const>(l, name, f);

    Registrar::template do_register_one_getter<std::unique_ptr<volatile Class>*,
                                               volatile Member* const>(
        l, name, f);
    Registrar::template do_register_one_getter<
        std::unique_ptr<const volatile Class>*,
        const volatile Member* const>(l, name, f);

    Registrar::template do_register_one_getter<
        const std::unique_ptr<volatile Class>*,
        volatile Member* const>(l, name, f);
    Registrar::template do_register_one_getter<
        const std::unique_ptr<const volatile Class>*,
        const volatile Member* const>(l, name, f);
#endif
  }

  template <typename F>
  static void register_member_ref(luaw& l, const char* name, F&& f) {
    PEACALM_LUAW_ASSERT(name);

    // Member reference should have same low-level cv- property as class
    // pointer.

    using Registrar = luaw::registrar<Member Class::*>;

    Registrar::template do_register_one_getter<Class*,
                                               const std::shared_ptr<Member>>(
        l, name, [=](auto& p) {
          return luaw_detail::mock_shared<Member>(&f(p));
        });
    Registrar::template do_register_one_getter<
        const Class*,
        const std::shared_ptr<const Member>>(l, name, [=](auto& p) {
      return luaw_detail::mock_shared<const Member>(&f(p));
    });

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT
    Registrar::template do_register_one_getter<
        volatile Class*,
        const std::shared_ptr<volatile Member>>(l, name, [=](auto& p) {
      return luaw_detail::mock_shared<volatile Member>(&f(p));
    });
    Registrar::template do_register_one_getter<
        const volatile Class*,
        const std::shared_ptr<const volatile Member>>(l, name, [=](auto& p) {
      return luaw_detail::mock_shared<const volatile Member>(&f(p));
    });
#endif

    Registrar::template do_register_one_getter<std::shared_ptr<Class>*,
                                               const std::shared_ptr<Member>>(
        l, name, [=](auto& p) {
          return luaw_detail::mock_shared<Member>(&f(p));
        });
    Registrar::template do_register_one_getter<
        std::shared_ptr<const Class>*,
        const std::shared_ptr<const Member>>(l, name, [=](auto& p) {
      return luaw_detail::mock_shared<const Member>(&f(p));
    });

    Registrar::template do_register_one_getter<const std::shared_ptr<Class>*,
                                               const std::shared_ptr<Member>>(
        l, name, [=](auto& p) {
          return luaw_detail::mock_shared<Member>(&f(p));
        });
    Registrar::template do_register_one_getter<
        const std::shared_ptr<const Class>*,
        const std::shared_ptr<const Member>>(l, name, [=](auto& p) {
      return luaw_detail::mock_shared<const Member>(&f(p));
    });

    Registrar::template do_register_one_getter<std::unique_ptr<Class>*,
                                               const std::shared_ptr<Member>>(
        l, name, [=](auto& p) {
          return luaw_detail::mock_shared<Member>(&f(p));
        });
    Registrar::template do_register_one_getter<
        std::unique_ptr<const Class>*,
        const std::shared_ptr<const Member>>(l, name, [=](auto& p) {
      return luaw_detail::mock_shared<const Member>(&f(p));
    });

    Registrar::template do_register_one_getter<const std::unique_ptr<Class>*,
                                               const std::shared_ptr<Member>>(
        l, name, [=](auto& p) {
          return luaw_detail::mock_shared<Member>(&f(p));
        });
    Registrar::template do_register_one_getter<
        const std::unique_ptr<const Class>*,
        const std::shared_ptr<const Member>>(l, name, [=](auto& p) {
      return luaw_detail::mock_shared<const Member>(&f(p));
    });

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT
    Registrar::template do_register_one_getter<
        std::shared_ptr<volatile Class>*,
        const std::shared_ptr<volatile Member>>(l, name, [=](auto& p) {
      return luaw_detail::mock_shared<volatile Member>(&f(p));
    });
    Registrar::template do_register_one_getter<
        std::shared_ptr<const volatile Class>*,
        const std::shared_ptr<const volatile Member>>(l, name, [=](auto& p) {
      return luaw_detail::mock_shared<const volatile Member>(&f(p));
    });

    Registrar::template do_register_one_getter<
        const std::shared_ptr<volatile Class>*,
        const std::shared_ptr<volatile Member>>(l, name, [=](auto& p) {
      return luaw_detail::mock_shared<volatile Member>(&f(p));
    });
    Registrar::template do_register_one_getter<
        const std::shared_ptr<const volatile Class>*,
        const std::shared_ptr<const volatile Member>>(l, name, [=](auto& p) {
      return luaw_detail::mock_shared<const volatile Member>(&f(p));
    });

    Registrar::template do_register_one_getter<
        std::unique_ptr<volatile Class>*,
        const std::shared_ptr<volatile Member>>(l, name, [=](auto& p) {
      return luaw_detail::mock_shared<volatile Member>(&f(p));
    });
    Registrar::template do_register_one_getter<
        std::unique_ptr<const volatile Class>*,
        const std::shared_ptr<const volatile Member>>(l, name, [=](auto& p) {
      return luaw_detail::mock_shared<const volatile Member>(&f(p));
    });

    Registrar::template do_register_one_getter<
        const std::unique_ptr<volatile Class>*,
        const std::shared_ptr<volatile Member>>(l, name, [=](auto& p) {
      return luaw_detail::mock_shared<volatile Member>(&f(p));
    });
    Registrar::template do_register_one_getter<
        const std::unique_ptr<const volatile Class>*,
        const std::shared_ptr<const volatile Member>>(l, name, [=](auto& p) {
      return luaw_detail::mock_shared<const volatile Member>(&f(p));
    });
#endif
  }
};

/* -------------------------------------------------------------------------- */
// Registrar specialization for member variables.
template <typename Class, typename Member>
struct luaw::registrar<
    Member Class::*,
    std::enable_if_t<std::is_member_object_pointer<Member Class::*>::value>> {
  // register a specific member
  template <typename F>
  static void register_member(luaw& l, const char* mname, F&& f) {
    register_one_getter<Class*>(l, mname, std::forward<F>(f));
    register_one_getter<const Class*>(l, mname, std::forward<F>(f));

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

    register_one_getter<volatile Class*>(l, mname, std::forward<F>(f));
    register_one_getter<const volatile Class*>(l, mname, std::forward<F>(f));

#endif

    // Do not support nested smart pointers, which is meaningless!
    // DO NOT support top-level volatile for smart pointers!
    if PEACAML_LUAW_IF_CONSTEXPR (!luaw_detail::is_std_shared_ptr<
                                      Class>::value &&
                                  !luaw_detail::is_std_unique_ptr<
                                      Class>::value) {
      register_one_getter<std::shared_ptr<Class>*>(
          l, mname, std::forward<F>(f));
      register_one_getter<std::shared_ptr<const Class>*>(
          l, mname, std::forward<F>(f));

      register_one_getter<const std::shared_ptr<Class>*>(
          l, mname, std::forward<F>(f));
      register_one_getter<const std::shared_ptr<const Class>*>(
          l, mname, std::forward<F>(f));

      register_one_getter<std::unique_ptr<Class>*>(
          l, mname, std::forward<F>(f));
      register_one_getter<std::unique_ptr<const Class>*>(
          l, mname, std::forward<F>(f));

      register_one_getter<const std::unique_ptr<Class>*>(
          l, mname, std::forward<F>(f));
      register_one_getter<const std::unique_ptr<const Class>*>(
          l, mname, std::forward<F>(f));

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT
      // for low-level volatile

      register_one_getter<std::shared_ptr<volatile Class>*>(
          l, mname, std::forward<F>(f));
      register_one_getter<std::shared_ptr<const volatile Class>*>(
          l, mname, std::forward<F>(f));

      register_one_getter<const std::shared_ptr<volatile Class>*>(
          l, mname, std::forward<F>(f));
      register_one_getter<const std::shared_ptr<const volatile Class>*>(
          l, mname, std::forward<F>(f));

      register_one_getter<std::unique_ptr<volatile Class>*>(
          l, mname, std::forward<F>(f));
      register_one_getter<std::unique_ptr<const volatile Class>*>(
          l, mname, std::forward<F>(f));

      register_one_getter<const std::unique_ptr<volatile Class>*>(
          l, mname, std::forward<F>(f));
      register_one_getter<const std::unique_ptr<const volatile Class>*>(
          l, mname, std::forward<F>(f));

#endif
    }

    __register_setters(l, mname, std::forward<F>(f), std::is_const<Member>{});
  }

  // Register a member getter for `ObjectPointer`, the member's type is same cv-
  // qualifed with object on `Member`.
  template <typename ObjectPointer, typename F>
  static void register_one_getter(luaw& l, const char* mname, F&& f) {
    static_assert(std::is_pointer<ObjectPointer>::value,
                  "ObjectPointer should be a pointer type");

    // To add same cv- property on Member as Object
    using O = luaw_detail::remove_ptr_or_smart_ptr_t<
        std::remove_pointer_t<ObjectPointer>>;
    using M0 = Member;
    using M1 = std::conditional_t<std::is_volatile<O>::value, volatile M0, M0>;
    using M2 = std::conditional_t<std::is_const<O>::value, const M1, M1>;

    do_register_one_getter<ObjectPointer, M2, F>(l, mname, std::forward<F>(f));
  }

  // Register a member getter for `ObjectPointer`, the member's type is
  // `CVPossibleMember`.
  template <typename ObjectPointer, typename CVPossibleMember, typename F>
  static void do_register_one_getter(luaw& l, const char* mname, F&& f) {
    static_assert(std::is_pointer<ObjectPointer>::value,
                  "ObjectPointer should be a pointer type");
    // The getter will return a copy of the member with type `CVPossibleMember`
    // which is a type maybe possibly cv- qualified on `Member`. And it should
    // have same cv- property with `ObjectPointer`'s low-level cv- property.
    //
    // The type `Member` maybe the member's real type, or could be sepcified by
    // `Hint` and it could be different with the member's real type, but it must
    // be convertible from the real type to this target member type.
    // Thus, it enables us to register a member to another type in Lua, e.g.
    // register a non-const member as a const member in Lua, or register a
    // integer member as a boolean member in Lua, etc.
    auto getter = [f, &l](ObjectPointer o) -> CVPossibleMember {
      PEACALM_LUAW_ASSERT(o);
      auto p = luaw_detail::retrieve_underlying_ptr(*o);
      if (!p) {
        luaL_error(l.L(), "Getting member by empty smart ptr.");
        // Never runs here
        PEACALM_LUAW_ASSERT(false);
      }
      return f(*p);
    };
    void* p = reinterpret_cast<void*>(
        const_cast<std::type_info*>(&typeid(ObjectPointer)));
    l.touchtb(p, LUA_REGISTRYINDEX)
        .touchtb(luaw::member_info_fields::member_getter)
        .setkv<luaw::function_tag>(mname, getter);
    l.pop(2);
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

  // Member is const
  template <typename F>
  static void __register_setters(luaw&       l,
                                 const char* mname,
                                 F&&         f,
                                 std::true_type) {
    __register_const_member<Class*>(l, mname);
    __register_const_member<const Class*>(l, mname);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

    __register_const_member<volatile Class*>(l, mname);
    __register_const_member<const volatile Class*>(l, mname);

#endif

    // Do not support nested smart pointers, which is meaningless!
    // DO NOT support top-level volatile for smart pointers!
    if PEACAML_LUAW_IF_CONSTEXPR (!luaw_detail::is_std_shared_ptr<
                                      Class>::value &&
                                  !luaw_detail::is_std_unique_ptr<
                                      Class>::value) {
      __register_const_member<std::shared_ptr<Class>*>(l, mname);
      __register_const_member<std::shared_ptr<const Class>*>(l, mname);
      __register_const_member<const std::shared_ptr<Class>*>(l, mname);
      __register_const_member<const std::shared_ptr<const Class>*>(l, mname);

      __register_const_member<std::unique_ptr<Class>*>(l, mname);
      __register_const_member<std::unique_ptr<const Class>*>(l, mname);
      __register_const_member<const std::unique_ptr<Class>*>(l, mname);
      __register_const_member<const std::unique_ptr<const Class>*>(l, mname);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT
      // for low-level volatile:

      __register_const_member<std::shared_ptr<volatile Class>*>(l, mname);
      __register_const_member<std::shared_ptr<const volatile Class>*>(l, mname);
      __register_const_member<const std::shared_ptr<volatile Class>*>(l, mname);
      __register_const_member<const std::shared_ptr<const volatile Class>*>(
          l, mname);

      __register_const_member<std::unique_ptr<volatile Class>*>(l, mname);
      __register_const_member<std::unique_ptr<const volatile Class>*>(l, mname);
      __register_const_member<const std::unique_ptr<volatile Class>*>(l, mname);
      __register_const_member<const std::unique_ptr<const volatile Class>*>(
          l, mname);

#endif
    }
  }

  // Member is not const
  template <typename F>
  static void __register_setters(luaw&       l,
                                 const char* mname,
                                 F&&         f,
                                 std::false_type) {
#define DEFINE_SETTER(ObjectType)                                \
  {                                                              \
    auto setter = [=, &l](ObjectType o, Member v) {              \
      PEACALM_LUAW_ASSERT(o);                                    \
      auto p = luaw_detail::retrieve_underlying_ptr(*o);         \
      if (!p) {                                                  \
        luaL_error(l.L(), "Setting member by empty smart ptr."); \
        /* Never runs here */                                    \
        PEACALM_LUAW_ASSERT(false);                              \
      }                                                          \
      f(*p) = std::move(v);                                      \
    };                                                           \
    void* p = reinterpret_cast<void*>(                           \
        const_cast<std::type_info*>(&typeid(ObjectType)));       \
    l.touchtb(p, LUA_REGISTRYINDEX)                              \
        .touchtb(luaw::member_info_fields::member_setter)        \
        .setkv<luaw::function_tag>(mname, setter);               \
    l.pop(2);                                                    \
  }

    DEFINE_SETTER(Class*);
    // the object is const, so member is const
    __register_const_member<const Class*>(l, mname);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

    DEFINE_SETTER(volatile Class*);
    __register_const_member<const volatile Class*>(l, mname);

#endif

    // Do not support nested smart pointers, which is meaningless!
    // DO NOT support top-level volatile for smart pointers!
    if PEACAML_LUAW_IF_CONSTEXPR (!luaw_detail::is_std_shared_ptr<
                                      Class>::value &&
                                  !luaw_detail::is_std_unique_ptr<
                                      Class>::value) {
      DEFINE_SETTER(std::shared_ptr<Class>*);
      DEFINE_SETTER(const std::shared_ptr<Class>*);
      __register_const_member<std::shared_ptr<const Class>*>(l, mname);
      __register_const_member<const std::shared_ptr<const Class>*>(l, mname);

      DEFINE_SETTER(std::unique_ptr<Class>*);
      DEFINE_SETTER(const std::unique_ptr<Class>*);
      __register_const_member<std::unique_ptr<const Class>*>(l, mname);
      __register_const_member<const std::unique_ptr<const Class>*>(l, mname);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT
      // for low-level volatile

      DEFINE_SETTER(std::shared_ptr<volatile Class>*);
      DEFINE_SETTER(const std::shared_ptr<volatile Class>*);
      __register_const_member<std::shared_ptr<const volatile Class>*>(l, mname);
      __register_const_member<const std::shared_ptr<const volatile Class>*>(
          l, mname);

      DEFINE_SETTER(std::unique_ptr<volatile Class>*);
      DEFINE_SETTER(const std::unique_ptr<volatile Class>*);
      __register_const_member<std::unique_ptr<const volatile Class>*>(l, mname);
      __register_const_member<const std::unique_ptr<const volatile Class>*>(
          l, mname);

#endif
    }

#undef DEFINE_SETTER
  }
};

/* -------------------------------------------------------------------------- */
// Registrar specialization for member functions.
// @{

template <typename Class, typename Return, typename... Args>
struct luaw::registrar<Return (Class::*)(Args...)> {
  // implementations

  template <typename ObjectType, typename MemberFunction>
  static void register_member_function(luaw&            l,
                                       const char*      fname,
                                       MemberFunction&& mf) {
    auto f = [mf, &l](ObjectType o, Args... args) -> Return {
      if (!o) {
        luaL_error(l.L(), "Calling member function by null pointer.");
        // Never runs here.
        PEACALM_LUAW_ASSERT(false);
        // Do not return Return() to enable functions with reference results.
        // return Return();
      }
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
    register_member_function<Class*>(l, fname, mf);
    register_nonconst_member_function<const Class*>(l, fname);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

    register_nonvolatile_member_function<volatile Class*>(l, fname);
    register_nonconst_member_function<const volatile Class*>(l, fname);
    register_nonvolatile_member_function<const volatile Class*>(l, fname);

#endif

    // Do not support nested smart pointers, which is meaningless!
    // DO NOT support top-level volatile for smart pointers!
    if PEACAML_LUAW_IF_CONSTEXPR (!luaw_detail::is_std_shared_ptr<
                                      Class>::value &&
                                  !luaw_detail::is_std_unique_ptr<
                                      Class>::value) {
      register_member_function<std::shared_ptr<Class>*>(l, fname, mf);
      register_member_function<const std::shared_ptr<Class>*>(l, fname, mf);

      register_nonconst_member_function<std::shared_ptr<const Class>*>(l,
                                                                       fname);
      register_nonconst_member_function<const std::shared_ptr<const Class>*>(
          l, fname);

      register_member_function<std::unique_ptr<Class>*>(l, fname, mf);
      register_member_function<const std::unique_ptr<Class>*>(l, fname, mf);

      register_nonconst_member_function<std::unique_ptr<const Class>*>(l,
                                                                       fname);
      register_nonconst_member_function<const std::unique_ptr<const Class>*>(
          l, fname);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT
      // for low-level volatile

      register_nonvolatile_member_function<std::shared_ptr<volatile Class>*>(
          l, fname);
      register_nonvolatile_member_function<
          const std::shared_ptr<volatile Class>*>(l, fname);

      register_nonconst_member_function<std::shared_ptr<const volatile Class>*>(
          l, fname);
      register_nonconst_member_function<
          const std::shared_ptr<const volatile Class>*>(l, fname);

      register_nonvolatile_member_function<
          std::shared_ptr<const volatile Class>*>(l, fname);
      register_nonvolatile_member_function<
          const std::shared_ptr<const volatile Class>*>(l, fname);

      register_nonvolatile_member_function<std::unique_ptr<volatile Class>*>(
          l, fname);
      register_nonvolatile_member_function<
          const std::unique_ptr<volatile Class>*>(l, fname);

      register_nonconst_member_function<std::unique_ptr<const volatile Class>*>(
          l, fname);
      register_nonconst_member_function<
          const std::unique_ptr<const volatile Class>*>(l, fname);

      register_nonvolatile_member_function<
          std::unique_ptr<const volatile Class>*>(l, fname);
      register_nonvolatile_member_function<
          const std::unique_ptr<const volatile Class>*>(l, fname);

#endif
    }
  }
};

template <typename Class, typename Return, typename... Args>
struct luaw::registrar<Return (Class::*)(Args...) const> {
  template <typename MemberFunction>
  static void register_member(luaw& l, const char* fname, MemberFunction mf) {
    using Basic = luaw::registrar<Return (Class::*)(Args...)>;

    Basic::template register_member_function<Class*>(l, fname, mf);
    Basic::template register_member_function<const Class*>(l, fname, mf);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

    Basic::template register_nonvolatile_member_function<volatile Class*>(
        l, fname);
    Basic::template register_nonvolatile_member_function<const volatile Class*>(
        l, fname);

#endif

    // Do not support nested smart pointers, which is meaningless!
    // DO NOT support top-level volatile for smart pointers!
    if PEACAML_LUAW_IF_CONSTEXPR (!luaw_detail::is_std_shared_ptr<
                                      Class>::value &&
                                  !luaw_detail::is_std_unique_ptr<
                                      Class>::value) {
      Basic::template register_member_function<std::shared_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<const std::shared_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<std::shared_ptr<const Class>*>(
          l, fname, mf);
      Basic::template register_member_function<
          const std::shared_ptr<const Class>*>(l, fname, mf);

      Basic::template register_member_function<std::unique_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<const std::unique_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<std::unique_ptr<const Class>*>(
          l, fname, mf);
      Basic::template register_member_function<
          const std::unique_ptr<const Class>*>(l, fname, mf);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT
      // for low-level volatile

      Basic::template register_nonvolatile_member_function<
          std::shared_ptr<volatile Class>*>(l, fname);
      Basic::template register_nonvolatile_member_function<
          const std::shared_ptr<volatile Class>*>(l, fname);
      Basic::template register_nonvolatile_member_function<
          std::shared_ptr<const volatile Class>*>(l, fname);
      Basic::template register_nonvolatile_member_function<
          const std::shared_ptr<const volatile Class>*>(l, fname);

      Basic::template register_nonvolatile_member_function<
          std::unique_ptr<volatile Class>*>(l, fname);
      Basic::template register_nonvolatile_member_function<
          const std::unique_ptr<volatile Class>*>(l, fname);
      Basic::template register_nonvolatile_member_function<
          std::unique_ptr<const volatile Class>*>(l, fname);
      Basic::template register_nonvolatile_member_function<
          const std::unique_ptr<const volatile Class>*>(l, fname);

#endif
    }
  }
};

template <typename Class, typename Return, typename... Args>
struct luaw::registrar<Return (Class::*)(Args...) volatile> {
  template <typename MemberFunction>
  static void register_member(luaw& l, const char* fname, MemberFunction mf) {
    using Basic = luaw::registrar<Return (Class::*)(Args...)>;

    Basic::template register_member_function<Class*>(l, fname, mf);
    Basic::template register_nonconst_member_function<const Class*>(l, fname);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

    Basic::template register_member_function<volatile Class*>(l, fname, mf);
    Basic::template register_nonconst_member_function<const volatile Class*>(
        l, fname);

#endif

    // Do not support nested smart pointers, which is meaningless!
    // DO NOT support top-level volatile for smart pointers!
    if PEACAML_LUAW_IF_CONSTEXPR (!luaw_detail::is_std_shared_ptr<
                                      Class>::value &&
                                  !luaw_detail::is_std_unique_ptr<
                                      Class>::value) {
      Basic::template register_member_function<std::shared_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<const std::shared_ptr<Class>*>(
          l, fname, mf);

      Basic::template register_nonconst_member_function<
          std::shared_ptr<const Class>*>(l, fname);
      Basic::template register_nonconst_member_function<
          const std::shared_ptr<const Class>*>(l, fname);

      Basic::template register_member_function<std::unique_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<const std::unique_ptr<Class>*>(
          l, fname, mf);

      Basic::template register_nonconst_member_function<
          std::unique_ptr<const Class>*>(l, fname);
      Basic::template register_nonconst_member_function<
          const std::unique_ptr<const Class>*>(l, fname);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT
      // for low-level volatile

      Basic::template register_member_function<
          std::shared_ptr<volatile Class>*>(l, fname, mf);
      Basic::template register_member_function<
          const std::shared_ptr<volatile Class>*>(l, fname, mf);

      Basic::template register_nonconst_member_function<
          std::shared_ptr<const volatile Class>*>(l, fname);
      Basic::template register_nonconst_member_function<
          const std::shared_ptr<const volatile Class>*>(l, fname);

      Basic::template register_member_function<
          std::unique_ptr<volatile Class>*>(l, fname, mf);
      Basic::template register_member_function<
          const std::unique_ptr<volatile Class>*>(l, fname, mf);

      Basic::template register_nonconst_member_function<
          std::unique_ptr<const volatile Class>*>(l, fname);
      Basic::template register_nonconst_member_function<
          const std::unique_ptr<const volatile Class>*>(l, fname);

#endif
    }
  }
};

template <typename Class, typename Return, typename... Args>
struct luaw::registrar<Return (Class::*)(Args...) const volatile> {
  template <typename MemberFunction>
  static void register_member(luaw& l, const char* fname, MemberFunction mf) {
    using Basic = luaw::registrar<Return (Class::*)(Args...)>;

    Basic::template register_member_function<Class*>(l, fname, mf);
    Basic::template register_member_function<const Class*>(l, fname, mf);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

    Basic::template register_member_function<volatile Class*>(l, fname, mf);
    Basic::template register_member_function<const volatile Class*>(
        l, fname, mf);

#endif

    // Do not support nested smart pointers, which is meaningless!
    // DO NOT support top-level volatile for smart pointers!
    if PEACAML_LUAW_IF_CONSTEXPR (!luaw_detail::is_std_shared_ptr<
                                      Class>::value &&
                                  !luaw_detail::is_std_unique_ptr<
                                      Class>::value) {
      Basic::template register_member_function<std::shared_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<const std::shared_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<std::shared_ptr<const Class>*>(
          l, fname, mf);
      Basic::template register_member_function<
          const std::shared_ptr<const Class>*>(l, fname, mf);

      Basic::template register_member_function<std::unique_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<const std::unique_ptr<Class>*>(
          l, fname, mf);
      Basic::template register_member_function<std::unique_ptr<const Class>*>(
          l, fname, mf);
      Basic::template register_member_function<
          const std::unique_ptr<const Class>*>(l, fname, mf);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT
      // for low-level volatile

      Basic::template register_member_function<
          std::shared_ptr<volatile Class>*>(l, fname, mf);
      Basic::template register_member_function<
          const std::shared_ptr<volatile Class>*>(l, fname, mf);
      Basic::template register_member_function<
          std::shared_ptr<const volatile Class>*>(l, fname, mf);
      Basic::template register_member_function<
          const std::shared_ptr<const volatile Class>*>(l, fname, mf);

      Basic::template register_member_function<
          std::unique_ptr<volatile Class>*>(l, fname, mf);
      Basic::template register_member_function<
          const std::unique_ptr<volatile Class>*>(l, fname, mf);
      Basic::template register_member_function<
          std::unique_ptr<const volatile Class>*>(l, fname, mf);
      Basic::template register_member_function<
          const std::unique_ptr<const volatile Class>*>(l, fname, mf);

#endif
    }
  }
};

#if PEACALM_LUAW_SUPPORT_CPP17
// for member functions with noexcept-specification

template <typename Class, typename Return, typename... Args>
struct luaw::registrar<Return (Class::*)(Args...) noexcept> {
  template <typename MemberFunction>
  static void register_member(luaw& l, const char* fname, MemberFunction mf) {
    using Basic = luaw::registrar<Return (Class::*)(Args...)>;
    Basic::template register_member(l, fname, mf);
  }
};

template <typename Class, typename Return, typename... Args>
struct luaw::registrar<Return (Class::*)(Args...) const noexcept> {
  template <typename MemberFunction>
  static void register_member(luaw& l, const char* fname, MemberFunction mf) {
    using Basic = luaw::registrar<Return (Class::*)(Args...) const>;
    Basic::template register_member(l, fname, mf);
  }
};

template <typename Class, typename Return, typename... Args>
struct luaw::registrar<Return (Class::*)(Args...) volatile noexcept> {
  template <typename MemberFunction>
  static void register_member(luaw& l, const char* fname, MemberFunction mf) {
    using Basic = luaw::registrar<Return (Class::*)(Args...) volatile>;
    Basic::template register_member(l, fname, mf);
  }
};

template <typename Class, typename Return, typename... Args>
struct luaw::registrar<Return (Class::*)(Args...) const volatile noexcept> {
  template <typename MemberFunction>
  static void register_member(luaw& l, const char* fname, MemberFunction mf) {
    using Basic = luaw::registrar<Return (Class::*)(Args...) const volatile>;
    Basic::template register_member(l, fname, mf);
  }
};

#endif

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
    fakeluaw l(L);
    return provider() && provider()->provide(l, var_name);
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
    provider().provide(vars, *this);
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
    provider()->provide(vars, *this);
  }
  void __provide(const std::vector<std::string>& vars, std::false_type) {
    provider().provide(vars, *this);
  }
};

}  // namespace peacalm

#endif  // PEACALM_LUAW_H_
