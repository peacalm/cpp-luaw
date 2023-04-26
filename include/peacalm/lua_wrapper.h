
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

#ifndef LUA_WRAPPER_H_
#define LUA_WRAPPER_H_

#include <cassert>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
// This comment to avoid clang-format mix includes before sort
#include <lua.hpp>

static_assert(LUA_VERSION_NUM >= 504, "Lua version at least 5.4");

namespace peacalm {

namespace luafunc {
// Useful Lua functions extended for Lua

// Short writting for if-elseif-else statement.
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
    lua_pushstring(L, s);
    lua_error(L);
    return 0;
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
  lua_getglobal(L, "COUNTER0_mt");
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);
    luaL_newmetatable(L, "COUNTER0_mt");
    lua_pushcfunction(L, COUNTER0__index);
    lua_setfield(L, -2, "__index");
    lua_pushvalue(L, -1);
    lua_setglobal(L, "COUNTER0_mt");
  }
  lua_setmetatable(L, -2);
  return 1;
}

}  // namespace luafunc

class lua_wrapper {
  using self_t = lua_wrapper;
  lua_State* L_;

public:
  // Initialization options for lua_wrapper
  class opt {
    enum libinit : char { ignore = 0, load = 1, preload = 2 };

  public:
    opt() {}
    // Ignore all standard libs
    opt& ignore_libs() {
      linit_ = ignore;
      return *this;
    }
    // Load all standard libs
    opt& load_libs() {
      linit_ = load;
      return *this;
    }
    // Preload all standard libs
    opt& preload_libs() {
      linit_ = preload;
      return *this;
    }

    // Register extended functions
    opt& register_exfunc(bool r) {
      exfunc_ = r;
      return *this;
    }

    opt& use_state(lua_State* L) {
      L_ = L;
      return *this;
    }

    // Load libs specified by `l`
    // `l` must point to a luaL_Reg array which ends with {NULL, NULL}
    opt& custom_load(const luaL_Reg* l) {
      lload_ = l;
      return *this;
    }

    // Preload libs specified by `l`
    // `l` must point to a luaL_Reg array which ends with {NULL, NULL}
    opt& custom_preload(const luaL_Reg* l) {
      lpreload_ = l;
      return *this;
    }

  private:
    libinit         linit_    = load;
    bool            exfunc_   = true;
    lua_State*      L_        = nullptr;
    const luaL_Reg* lload_    = nullptr;
    const luaL_Reg* lpreload_ = nullptr;
    friend class lua_wrapper;
  };

  lua_wrapper(const opt& o = opt{}) { init(o); }
  lua_wrapper(lua_State* L) { init(opt{}.use_state(L)); }
  ~lua_wrapper() { close(); }

  void init(const opt& o = opt{}) {
    if (o.L_) {
      L_ = o.L_;
    } else {
      L_ = luaL_newstate();
    }

    if (o.linit_ == opt::libinit::load) {
      luaL_openlibs(L_);
    } else if (o.linit_ == opt::libinit::preload) {
      preload_libs();
    }

    if (o.exfunc_) { register_functions(); }

    if (o.lload_) {
      for (const luaL_Reg* p = o.lload_; p->func; ++p) {
        luaL_requiref(L_, p->name, p->func, 1);
        pop();
      }
    }

    if (o.lpreload_) {
      lua_getglobal(L_, LUA_LOADLIBNAME);
      lua_getfield(L_, -1, "preload");
      for (const luaL_Reg* p = o.lpreload_; p->func; ++p) {
        lua_pushcfunction(L_, p->func);
        lua_setfield(L_, -2, p->name);
      }
      pop(2);
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
    luaL_requiref(L_, LUA_GNAME, luaopen_base, 1);
    luaL_requiref(L_, LUA_LOADLIBNAME, luaopen_package, 1);
    lua_getfield(L_, -1, "preload");
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
      lua_pushcfunction(L_, p->func);
      lua_setfield(L_, -2, p->name);
    }
    pop(3);
  }

  void register_functions() {
    lua_register(L_, "IF", luafunc::IF);
    lua_register(L_, "SET", luafunc::SET);
    lua_register(L_, "COUNTER", luafunc::COUNTER);
    lua_register(L_, "COUNTER0", luafunc::COUNTER0);
  }

  lua_State* L() const { return L_; }
  void       L(lua_State* L) { L_ = L; }

  /// Convert given index to absolute index of stack
  int abs_index(int idx) {
    return idx < 0 && -idx <= gettop() ? gettop() + idx + 1 : idx;
  }

  void pop(int n = 1) { lua_pop(L_, n); }
  int  gettop() const { return lua_gettop(L_); }
  void settop(int idx) { lua_settop(L_, idx); }

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
  bool isstring(int idx = -1)        { return lua_isstring(L_, idx); }
  bool isnumber(int idx = -1)        { return lua_isnumber(L_, idx); }
  bool isinteger(int idx = -1)       { return lua_isinteger(L_, idx); }
  bool isboolean(int idx = -1)       { return lua_isboolean(L_, idx); }
  bool isnil(int idx = -1)           { return lua_isnil(L_, idx); }
  bool isnone(int idx = -1)          { return lua_isnone(L_, idx); }
  bool isnoneornil(int idx = -1)     { return lua_isnoneornil(L_, idx); }
  bool istable(int idx = -1)         { return lua_istable(L_, idx); }
  bool iscfunction(int idx = -1)     { return lua_iscfunction(L_, idx); }
  bool isfunction(int idx = -1)      { return lua_isfunction(L_, idx); }
  bool isuserdata(int idx = -1)      { return lua_isuserdata(L_, idx); }
  bool islightuserdata(int idx = -1) { return lua_islightuserdata(L_, idx); }
  bool isthread(int idx = -1)        { return lua_isthread(L_, idx); }
  // clang-format on

  int getglobal(const char* name) { return lua_getglobal(L_, name); }

  int pcall(int n, int r, int f) { return lua_pcall(L_, n, r, f); }

  int         type(int i) const { return lua_type(L_, i); }
  const char* type_name(int i) const { return lua_typename(L_, type(i)); }

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

  DEFINE_TYPE_CONVERSION(int, int, 0)
  DEFINE_TYPE_CONVERSION(uint, unsigned int, 0)
  DEFINE_TYPE_CONVERSION(long, long, 0)
  DEFINE_TYPE_CONVERSION(ulong, unsigned long, 0)
  DEFINE_TYPE_CONVERSION(llong, long long, 0)
  DEFINE_TYPE_CONVERSION(ullong, unsigned long long, 0)
  DEFINE_TYPE_CONVERSION(bool, bool, false)
  DEFINE_TYPE_CONVERSION(double, double, 0)
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
   * bool, integer types, double, std::string, std::vector, std::set,
   * std::unordered_set, std::map and std::unordered_map.
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
   *
   * @{
   */

  // To simple types except of to c_str which is unsafe
  template <typename T>
  std::enable_if_t<
      std::is_same<T, bool>::value || std::is_same<T, int>::value ||
          std::is_same<T, unsigned int>::value ||
          std::is_same<T, long>::value ||
          std::is_same<T, unsigned long>::value ||
          std::is_same<T, long long>::value ||
          std::is_same<T, unsigned long long>::value ||
          std::is_same<T, double>::value || std::is_same<T, std::string>::value,
      T>
  to(int   idx         = -1,
     bool  disable_log = false,
     bool* failed      = nullptr,
     bool* exists      = nullptr);

  // To std::vector
  // NOTICE: Discard nil in list! e.g. {1,2,nil,4} -> vector<int>{1,2,4}
  template <typename T>
  std::enable_if_t<std::is_same<T,
                                std::vector<typename T::value_type,
                                            typename T::allocator_type>>::value,
                   T>
  to(int   idx         = -1,
     bool  disable_log = false,
     bool* failed      = nullptr,
     bool* exists      = nullptr) {
    if (exists) *exists = !isnoneornil(idx);
    if (isnoneornil(idx)) {
      if (failed) *failed = false;
      return T{};
    }
    if (!istable(idx)) {
      if (failed) *failed = true;
      if (!disable_log) log_type_convert_error(idx, "vector");
      return T{};
    }
    T ret;
    if (failed) *failed = false;
    int sz = luaL_len(L_, idx);
    ret.reserve(sz);
    for (int i = 1; i <= sz; ++i) {
      lua_geti(L_, idx, i);
      bool subfailed, subexists;
      auto subret =
          to<typename T::value_type>(-1, disable_log, &subfailed, &subexists);
      // Only add elements exist and conversion succeeded
      if (!subfailed && subexists) ret.push_back(std::move(subret));
      if (subfailed && failed) *failed = true;
      pop();
    }
    return ret;
  }

  // To std::set
  template <typename T>
  std::enable_if_t<std::is_same<T,
                                std::set<typename T::key_type,
                                         typename T::key_compare,
                                         typename T::allocator_type>>::value,
                   T>
  to(int   idx         = -1,
     bool  disable_log = false,
     bool* failed      = nullptr,
     bool* exists      = nullptr) {
    auto v = to<std::vector<typename T::key_type, typename T::allocator_type>>(
        idx, disable_log, failed, exists);
    return T(v.begin(), v.end());
  }

  // To std::unordered_set
  template <typename T>
  std::enable_if_t<
      std::is_same<T,
                   std::unordered_set<typename T::key_type,
                                      typename T::hasher,
                                      typename T::key_equal,
                                      typename T::allocator_type>>::value,
      T>
  to(int   idx         = -1,
     bool  disable_log = false,
     bool* failed      = nullptr,
     bool* exists      = nullptr) {
    auto v = to<std::vector<typename T::key_type, typename T::allocator_type>>(
        idx, disable_log, failed, exists);
    return T(v.begin(), v.end());
  }

  // To std::map
  template <typename T>
  std::enable_if_t<std::is_same<T,
                                std::map<typename T::key_type,
                                         typename T::mapped_type,
                                         typename T::key_compare,
                                         typename T::allocator_type>>::value,
                   T>
  to(int   idx         = -1,
     bool  disable_log = false,
     bool* failed      = nullptr,
     bool* exists      = nullptr) {
    return tom<T>(idx, disable_log, failed, exists, "map");
  }

  // To std::unordered_map
  template <typename T>
  std::enable_if_t<
      std::is_same<T,
                   std::unordered_map<typename T::key_type,
                                      typename T::mapped_type,
                                      typename T::hasher,
                                      typename T::key_equal,
                                      typename T::allocator_type>>::value,
      T>
  to(int   idx         = -1,
     bool  disable_log = false,
     bool* failed      = nullptr,
     bool* exists      = nullptr) {
    return tom<T>(idx, disable_log, failed, exists, "unordered_map");
  }

  // Implementation of to map or to unordered_map
  template <typename T>
  T tom(int         idx         = -1,
        bool        disable_log = false,
        bool*       failed      = nullptr,
        bool*       exists      = nullptr,
        const char* tname       = "map") {
    if (exists) *exists = !isnoneornil(idx);
    if (isnoneornil(idx)) {
      if (failed) *failed = false;
      return T{};
    }
    if (!istable(idx)) {
      if (failed) *failed = true;
      if (!disable_log) log_type_convert_error(idx, tname);
      return T{};
    }
    T ret;
    if (failed) *failed = false;
    int absidx = abs_index(idx);
    lua_pushnil(L_);
    while (lua_next(L_, absidx) != 0) {
      bool kfailed, kexists, vfailed, vexists;
      auto key = to<typename T::key_type>(-2, disable_log, &kfailed, &kexists);
      if (!kfailed && kexists) {
        auto val =
            to<typename T::mapped_type>(-1, disable_log, &vfailed, &vexists);
        if (!vfailed && vexists) ret.insert({std::move(key), std::move(val)});
      }
      if ((kfailed || vfailed) && failed) *failed = true;
      pop();
    }
    return ret;
  }

  /** @} */

  ///////////////////////// seek fields ////////////////////////////////////////

  /// Get a global value by name and push it onto the stack, or push a nil if
  /// the name does not exist.
  self_t& gseek(const char* name) {
    getglobal(name);
    return *this;
  }
  self_t& gseek(const std::string& name) { return gseek(name.c_str()); }

  /// Push t[name] onto the stack where t is the value at the given index `idx`,
  /// or push a nil if the operation fails.
  self_t& seek(const char* name, int idx = -1) {
    if (gettop() > 0 && istable(idx)) {
      lua_getfield(L_, idx, name);
    } else {
      lua_pushnil(L_);
    }
    return *this;
  }
  self_t& seek(const std::string& name) { return seek(name.c_str()); }

  /// Push t[n] onto the stack where t is the value at the given index `idx`, or
  /// push a nil if the operation fails. Note that index of list in Lua starts
  /// from 1.
  self_t& seek(int n, int idx = -1) {
    if (gettop() > 0 && istable(idx)) {
      lua_geti(L_, idx, n);
    } else {
      lua_pushnil(L_);
    }
    return *this;
  }

  ///////////////////////// set global variables ///////////////////////////////

  void set_integer(const char* name, long long value) {
    lua_pushinteger(L_, value);
    lua_setglobal(L_, name);
  }
  void set_integer(const std::string& name, long long value) {
    set_integer(name.c_str(), value);
  }

  void set_number(const char* name, double value) {
    lua_pushnumber(L_, value);
    lua_setglobal(L_, name);
  }
  void set_number(const std::string& name, double value) {
    set_number(name.c_str(), value);
  }

  void set_boolean(const char* name, bool value) {
    lua_pushboolean(L_, static_cast<int>(value));
    lua_setglobal(L_, name);
  }
  void set_boolean(const std::string& name, bool value) {
    set_boolean(name.c_str(), value);
  }

  void set_nil(const char* name) {
    lua_pushnil(L_);
    lua_setglobal(L_, name);
  }
  void set_nil(const std::string& name) { set_nil(name.c_str()); }

  void set_string(const char* name, const char* value) {
    lua_pushstring(L_, value);
    lua_setglobal(L_, name);
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
    lua_getglobal(L_, name);                                               \
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

  DEFINE_GLOBAL_GET(int, int, 0)
  DEFINE_GLOBAL_GET(uint, unsigned int, 0)
  DEFINE_GLOBAL_GET(long, long, 0)
  DEFINE_GLOBAL_GET(ulong, unsigned long, 0)
  DEFINE_GLOBAL_GET(llong, long long, 0)
  DEFINE_GLOBAL_GET(ullong, unsigned long long, 0)
  DEFINE_GLOBAL_GET(bool, bool, false)
  DEFINE_GLOBAL_GET(double, double, 0)
  DEFINE_GLOBAL_GET(string, std::string, "")
#undef DEFINE_GLOBAL_GET

  // NO POP! Cause the c_str body is in stack
  // Leave the duty of popping stack to caller
  // You'd better use get_string unless you know the difference.
  const char* get_c_str(const char* name,
                        const char* def         = "",
                        bool        disable_log = false,
                        bool*       failed      = nullptr,
                        bool*       exists      = nullptr) {
    lua_getglobal(L_, name);
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
   * bool, integer types, double, std::string, std::vector, std::set,
   * std::unordered_set, std::map and std::unordered_map. Note that here const
   * char* is not supported, which is unsafe.
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

  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(int, int, 0)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(uint, unsigned int, 0)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(long, long, 0)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(ulong, unsigned long, 0)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(llong, long long, 0)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(ullong, unsigned long long, 0)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(bool, bool, false)
  DEFINE_RECURSIVE_GET_SIMPLE_TYPE(double, double, 0)
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
      return T{};
    }
    int  sz = gettop();
    auto it = b;
    gseek(tocstr(*it++));
    if (it == e) {
      auto ret = to<T>(-1, disable_log, failed, exists);
      settop(sz);
      return ret;
    }
    while (it != e) {
      if (isnoneornil(-1)) {
        if (failed) *failed = false;
        if (exists) *exists = false;
        settop(sz);
        return T{};
      }
      if (!istable(-1)) {
        if (failed) *failed = true;
        if (exists) *exists = true;
        if (!disable_log) log_type_convert_error(-1, "table");
        settop(sz);
        return T{};
      }
      seek(tocstr(*it++));
    }
    auto ret = to<T>(-1, disable_log, failed, exists);
    settop(sz);
    return ret;
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

  const char* tocstr(const char* s) { return s; }
  const char* tocstr(const std::string& s) { return s.c_str(); }

public:
  //////////////////////// evaluate expression /////////////////////////////////

  /**
   * @brief Evaluate a Lua expression and get the result in simple C++ type.
   *
   * @param [in] expr Lua expression, which must have a return value.
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
    int sz = gettop();                                              \
    if (dostring(expr) != LUA_OK) {                                 \
      if (failed) *failed = true;                                   \
      if (!disable_log) log_error_in_stack();                       \
      settop(sz);                                                   \
      return def;                                                   \
    }                                                               \
    assert(gettop() >= sz);                                         \
    if (gettop() <= sz) {                                           \
      if (failed) *failed = true;                                   \
      if (!disable_log) log_error("No return");                     \
      return def;                                                   \
    }                                                               \
    type ret = to_##typename(-1, def, disable_log, failed);         \
    settop(sz);                                                     \
    return ret;                                                     \
  }                                                                 \
  type eval_##typename(const std::string& expr,                     \
                       const type&        def         = default,    \
                       bool               disable_log = false,      \
                       bool*              failed      = nullptr) {                    \
    return eval_##typename(expr.c_str(), def, disable_log, failed); \
  }

  DEFINE_EVAL(int, int, 0)
  DEFINE_EVAL(uint, unsigned int, 0)
  DEFINE_EVAL(long, long, 0)
  DEFINE_EVAL(ulong, unsigned long, 0)
  DEFINE_EVAL(llong, long long, 0)
  DEFINE_EVAL(ullong, unsigned long long, 0)
  DEFINE_EVAL(bool, bool, false)
  DEFINE_EVAL(double, double, 0)
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
    assert(gettop() >= sz);
    if (gettop() <= sz) {
      if (failed) *failed = true;
      if (!disable_log) log_error("No return");
      return def;
    }
    return to_c_str(-1, def, disable_log, failed);
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
   * bool, integer types, double, std::string, std::vector, std::set,
   * std::unordered_set, std::map and std::unordered_map.
   * @param [in] expr Lua expression, which must have a return value.
   * @param [in] disable_log Whether print a log when exception occurs.
   * @param [out] failed Will be set whether the operation is failed if this
   * pointer is not nullptr. If T is a container type, it regards the operation
   * as failed if any element failed.
   * @return The expression's result in type T.
   */
  template <typename T>
  T eval(const char* expr, bool disable_log = false, bool* failed = nullptr) {
    int sz = gettop();
    if (dostring(expr) != LUA_OK) {
      if (failed) *failed = true;
      if (!disable_log) log_error_in_stack();
      settop(sz);
      return T{};
    }
    assert(gettop() >= sz);
    if (gettop() <= sz) {
      if (failed) *failed = true;
      if (!disable_log) log_error("No return");
      return T{};
    }
    auto ret = to<T>(-1, disable_log, failed);
    settop(sz);
    return ret;
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

#define DEFINE_TO_SPECIALIZATIOIN(typename, type, default)           \
  template <>                                                        \
  inline type lua_wrapper::to<type>(                                 \
      int idx, bool disable_log, bool* failed, bool* exists) {       \
    return to_##typename(idx, default, disable_log, failed, exists); \
  }

DEFINE_TO_SPECIALIZATIOIN(bool, bool, false)
DEFINE_TO_SPECIALIZATIOIN(int, int, 0)
DEFINE_TO_SPECIALIZATIOIN(uint, unsigned int, 0)
DEFINE_TO_SPECIALIZATIOIN(long, long, 0)
DEFINE_TO_SPECIALIZATIOIN(ulong, unsigned long, 0)
DEFINE_TO_SPECIALIZATIOIN(llong, long long, 0)
DEFINE_TO_SPECIALIZATIOIN(ullong, unsigned long long, 0)
DEFINE_TO_SPECIALIZATIOIN(double, double, false)
#undef DEFINE_TO_SPECIALIZATIOIN

// A safe implementation of tostring
// Avoid implicitly modifying number to string in stack, which may cause panic
// while doing lua_next
template <>
inline std::string lua_wrapper::to<std::string>(int   idx,
                                                bool  disable_log,
                                                bool* failed,
                                                bool* exists) {
  if (exists) *exists = !isnoneornil(idx);
  if (isstring(idx)) {
    lua_pushvalue(L_, idx);  // make a copy, so, it's safe
    std::string ret = lua_tostring(L_, -1);
    pop();
    if (failed) *failed = false;
    return ret;
  }
  if (isnoneornil(idx)) {
    if (failed) *failed = false;
    return "";
  }
  if (failed) *failed = true;
  if (!disable_log) log_type_convert_error(idx, "string");
  return "";
}

////////////////////////////////////////////////////////////////////////////////

namespace lua_wrapper_internal {
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
}  // namespace lua_wrapper_internal

// VariableProviderPointerType should be a raw pointer type or std::shared_ptr
// or std::unique_ptr. The underlying provider type should implement a member
// function:
//     bool provide(lua_State* L, const char* vname);
// and in this function, should push a value whose name is vname on the stack of
// L then return true. Otherwise return false if vname is illegal or vname
// doesn't have a correct value.
template <typename VariableProviderPointerType>
class custom_lua_wrapper : public lua_wrapper {
  using base_t     = lua_wrapper;
  using provider_t = VariableProviderPointerType;
  static_assert(lua_wrapper_internal::is_ptr<provider_t>::value,
                "VariableProviderPointerType should be pointer type");
  using pointer_t = custom_lua_wrapper*;
  provider_t provider_;

public:
  template <typename... Args>
  custom_lua_wrapper(Args&&... args) : base_t(std::forward<Args>(args)...) {
    _G_setmetateble();
  }

  void              provider(const provider_t& p) { provider_ = p; }
  void              provider(provider_t&& p) { provider_ = std::move(p); }
  const provider_t& provider() const { return provider_; }
  provider_t&       provider() { return provider_; }

  bool provide(lua_State* L, const char* var_name) {
    return provider()->provide(L, var_name);
  }

private:
  void _G_setmetateble() {
    lua_getglobal(L(), "_G");
    if (lua_getmetatable(L(), -1) == 0) { luaL_newmetatable(L(), "_G_mt"); }
    lua_pushcfunction(L(), _G__index);
    lua_setfield(L(), -2, "__index");
    lua_setmetatable(L(), -2);
    pop();
    lua_pushlightuserdata(L(), (void*)this);
    lua_setfield(L(), LUA_REGISTRYINDEX, "this");
  }

  static int _G__index(lua_State* L) {
    const char* name = lua_tostring(L, 2);
    lua_getfield(L, LUA_REGISTRYINDEX, "this");
    pointer_t p = (pointer_t)lua_touserdata(L, -1);
    if (!p || !p->provide(L, name)) {
      lua_pushfstring(L, "Not found: %s", name);
      lua_error(L);
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
class lua_wrapper_crtp : public lua_wrapper {
  static const std::unordered_set<std::string> lua_key_words;
  using base_t = lua_wrapper;

public:
  template <typename... Args>
  lua_wrapper_crtp(Args&&... args) : base_t(std::forward<Args>(args)...) {}

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

  DEFINE_EVAL(int, int, 0)
  DEFINE_EVAL(uint, unsigned int, 0)
  DEFINE_EVAL(llong, long long, 0)
  DEFINE_EVAL(ullong, unsigned long long, 0)
  DEFINE_EVAL(bool, bool, false)
  DEFINE_EVAL(double, double, 0)
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
const std::unordered_set<std::string> lua_wrapper_crtp<Derived>::lua_key_words{
    "nil",      "true",  "false",    "and",   "or",     "not",
    "if",       "then",  "elseif",   "else",  "end",    "for",
    "do",       "while", "repeat",   "until", "return", "break",
    "continue", "goto",  "function", "in",    "local"};

// Usage examples of lua_wrapper_crtp
// VariableProviderType should implement member function:
//     void provide(const std::vector<std::string> &vars, lua_wrapper* l);

// Usage template 1
// Inherited raw variable provider type
template <typename VariableProviderType>
class lua_wrapper_is_provider
    : public VariableProviderType,
      public lua_wrapper_crtp<lua_wrapper_is_provider<VariableProviderType>> {
  using base_t =
      lua_wrapper_crtp<lua_wrapper_is_provider<VariableProviderType>>;
  using provider_t = VariableProviderType;

public:
  lua_wrapper_is_provider() {}

  template <typename... Args>
  lua_wrapper_is_provider(lua_State* L, Args&&... args)
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
class lua_wrapper_has_provider
    : public lua_wrapper_crtp<lua_wrapper_has_provider<VariableProviderType>> {
  using base_t =
      lua_wrapper_crtp<lua_wrapper_has_provider<VariableProviderType>>;
  using provider_t = VariableProviderType;

  provider_t provider_;

public:
  template <typename... Args>
  lua_wrapper_has_provider(Args&&... args)
      : base_t(std::forward<Args>(args)...) {}

  void              provider(const provider_t& p) { provider_ = p; }
  void              provider(provider_t&& p) { provider_ = std::move(p); }
  const provider_t& provider() const { return provider_; }
  provider_t&       provider() { return provider_; }

  void provide(const std::vector<std::string>& vars) {
    __provide(vars, lua_wrapper_internal::is_ptr<provider_t>{});
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

#endif  // LUA_WRAPPER_H_
