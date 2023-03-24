
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
#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
// This comment to avoid clang-format mix includes before sort
#include <lua.hpp>

static_assert(LUA_VERSION_NUM >= 504, "Lua version at least 5.4");

namespace peacalm {

class lua_wrapper {
  static const std::unordered_set<std::string> lua_key_words;
  lua_State*                                   L_;

public:
  lua_wrapper() { init(); }
  lua_wrapper(lua_State* L) { L_ = L; }
  ~lua_wrapper() {
    if (L_) close();
  }

  void init() {
    L_ = luaL_newstate();
    luaL_openlibs(L_);
  }
  void close() {
    lua_close(L_);
    L_ = nullptr;
  }
  void reset() {
    close();
    init();
  }

  lua_State* L() const { return L_; }
  void       L(lua_State* L) { L_ = L; }

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

  int pcall(int n, int r, int f) { return lua_pcall(L_, n, r, f); }

  int         type(int i) const { return lua_type(L_, i); }
  const char* type_name(int i) const { return lua_typename(L_, type(i)); }

  ///////////////////////// error log //////////////////////////////////////////

  void log_error(const char* s) const {
    std::cerr << "Lua: " << s << std::endl;
  }

  void log_error_in_stack(int i = -1) const {
    std::cerr << "Lua: " << lua_tostring(L_, i) << std::endl;
  }

  void log_type_convert_error(int i, const char* to) {
    std::cerr << "Lua: Can't convert to " << to << " by ";
    if (lua_isnumber(L_, i) || lua_isstring(L_, i) || lua_isboolean(L_, i) ||
        lua_isnil(L_, i) || lua_isinteger(L_, i)) {
      std::cerr << type_name(i) << ": ";
    }
    if (lua_isstring(L_, i)) {
      std::cerr << lua_tostring(L_, i) << std::endl;
    } else {
      std::cerr << luaL_tolstring(L_, i, NULL) << std::endl;
      pop();
    }
  }

  ///////////////////////// type conversions ///////////////////////////////////

  // NOTICE: Type conversions may different to Lua!
  // HIGHLIGNT: We mainly use C++'s type conversion strategy, in addition,
  //            a conversion strategy between number (float number) and
  //            number-literal-string, which is supported by Lua.
  //
  // Details:
  // 1. Implicitly conversion between integer, number, boolean using
  //    C++'s static_cast
  // 2. Implicitly conversion between number and number-literal-string by Lua
  // 3. When convert number 0 to boolean, will get false (not true as Lua does)
  // 4. NONE and NIL won't convert to any value, default will be returned
  // 5. Non-number-literal-string, including empty string, can't convert to any
  //    other types, default will be returned
  // 6. Integer's precision won't lost if it's value is representable by 64bit
  //    signed integer type, i.e. between [-2^63, 2^63 -1], which is
  //    [-9223372036854775808, 9223372036854775807]
  //
  // Examples:
  //   number 2.5 -> string "2.5" (By Lua)
  //   string "2.5" -> double 2.5 (By Lua)
  //   double 2.5 -> int 2 (By C++)
  //   string "2.5" -> int 2 (Firstly "2.5"->2.5 by lua, then 2.5->2 by C++)
  //   bool true -> int 1 (By C++)
  //   int 0 -> bool false (By C++)
  //   double 2.5 -> bool true (By C++)
  //   string "2.5" -> bool true ("2.5"->2.5 by Lua, then 2.5->true by C++)
  //

  /**
   * @brief Convert a value in Lua stack to C++ type value
   *
   * @param [in] i Index of Lua stack where the in
   * @param [in] default The default value returned if convert failed
   * @param [in] enable_log Whether print a log when exception occurs
   * @param [out] failed Will be set whether the convertion is failed if this
   * pointer is not nullptr
   *
   * @{
   */

#define DEFINE_TYPE_CONVERSION(typename, type, default)        \
  type to_##typename(int   i,                                  \
                     type  def        = default,               \
                     bool  enable_log = true,                  \
                     bool* failed     = nullptr) {                 \
    /* check integer before number to avoid precision lost. */ \
    if (lua_isinteger(L_, i)) {                                \
      if (failed) *failed = false;                             \
      return static_cast<type>(lua_tointeger(L_, i));          \
    }                                                          \
    if (lua_isnumber(L_, i)) {                                 \
      if (failed) *failed = false;                             \
      /* try integer first to avoid precision lost */          \
      long long t = lua_tointeger(L_, i);                      \
      if (t != 0) return t;                                    \
      return static_cast<type>(lua_tonumber(L_, i));           \
    }                                                          \
    if (lua_isboolean(L_, i)) {                                \
      if (failed) *failed = false;                             \
      return static_cast<type>(lua_toboolean(L_, i));          \
    }                                                          \
    if (lua_isnoneornil(L_, i)) {                              \
      if (failed) *failed = false;                             \
      return def;                                              \
    }                                                          \
    if (failed) *failed = true;                                \
    if (enable_log) log_type_convert_error(i, #type);          \
    return def;                                                \
  }

  DEFINE_TYPE_CONVERSION(int, int, 0)
  DEFINE_TYPE_CONVERSION(uint, unsigned int, 0)
  DEFINE_TYPE_CONVERSION(llong, long long, 0)
  DEFINE_TYPE_CONVERSION(ullong, unsigned long long, 0)
  DEFINE_TYPE_CONVERSION(bool, bool, false)
  DEFINE_TYPE_CONVERSION(double, double, 0)
#undef DEFINE_TYPE_CONVERSION

  // NOTICE: Lua will implicitly convert number to string
  // boolean can't convert to string
  const char* to_c_str(int         i,
                       const char* def        = "",
                       bool        enable_log = true,
                       bool*       failed     = nullptr) {
    if (lua_isstring(L_, i)) {  // include number
      if (failed) *failed = false;
      return lua_tostring(L_, i);
    }
    if (lua_isnoneornil(L_, i)) {
      if (failed) *failed = false;
      return def;
    }
    if (failed) *failed = true;
    if (enable_log) log_type_convert_error(i, "string");
    return def;
  }

  std::string to_string(int                i,
                        const std::string& def        = "",
                        bool               enable_log = true,
                        bool*              failed     = nullptr) {
    return std::string{to_c_str(i, def.c_str(), enable_log, failed)};
  }

  /** @}*/

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
   * @brief Get a variable in Lua and Convert it to C++ type
   *
   * @param [in] name The variable's name
   * @param [in] default The default value returned if failed
   * @param [in] enable_log Whether print a log when exception occurs
   * @param [out] failed Will be set whether the operation is failed if this
   * pointer is not nullptr
   *
   * @{
   */

#define DEFINE_GLOBAL_GET(typename, type, default)                \
  type get_##typename(const char* name,                           \
                      const type& def        = default,           \
                      bool        enable_log = true,              \
                      bool*       failed     = nullptr) {                   \
    lua_getglobal(L_, name);                                      \
    type ret = to_##typename(-1, def, enable_log, failed);        \
    pop();                                                        \
    return ret;                                                   \
  }                                                               \
  type get_##typename(const std::string& name,                    \
                      const type&        def        = default,    \
                      bool               enable_log = true,       \
                      bool*              failed     = nullptr) {                   \
    return get_##typename(name.c_str(), def, enable_log, failed); \
  }

  DEFINE_GLOBAL_GET(int, int, 0)
  DEFINE_GLOBAL_GET(uint, unsigned int, 0)
  DEFINE_GLOBAL_GET(llong, long long, 0)
  DEFINE_GLOBAL_GET(ullong, unsigned long long, 0)
  DEFINE_GLOBAL_GET(bool, bool, false)
  DEFINE_GLOBAL_GET(double, double, 0)
  DEFINE_GLOBAL_GET(string, std::string, "")
#undef DEFINE_GLOBAL_GET

  // NO POP! Cause the c_str body is in stack
  // Leave the duty of popping stack to caller
  const char* get_c_str(const char* name,
                        const char* def        = "",
                        bool        enable_log = true,
                        bool*       failed     = nullptr) {
    lua_getglobal(L_, name);
    return to_c_str(-1, def, enable_log, failed);
  }
  const char* get_c_str(const std::string& name,
                        const char*        def        = "",
                        bool               enable_log = true,
                        bool*              failed     = nullptr) {
    return get_c_str(name.c_str(), def, enable_log, failed);
  }

  /** @}*/

  //////////////////////// evaluate expression /////////////////////////////////

  /**
   * @brief Evaluate a Lua expression and get the result in C++ type
   *
   * @param [in] expr Lua expression, which must have a return value
   * @param [in] default The default value returned if failed
   * @param [in] enable_log Whether print a log when exception occurs
   * @param [out] failed Will be set whether the operation is failed if this
   * pointer is not nullptr
   *
   * @{
   */

#define DEFINE_EVAL(typename, type, default)                       \
  type eval_##typename(const char* expr,                           \
                       const type& def        = default,           \
                       bool        enable_log = true,              \
                       bool*       failed     = nullptr) {                   \
    int sz = gettop();                                             \
    if (dostring(expr) != LUA_OK) {                                \
      if (failed) *failed = true;                                  \
      if (enable_log) log_error_in_stack();                        \
      settop(sz);                                                  \
      return def;                                                  \
    }                                                              \
    assert(gettop() >= sz);                                        \
    if (gettop() <= sz) {                                          \
      if (failed) *failed = true;                                  \
      if (enable_log) log_error("No return");                      \
      return def;                                                  \
    }                                                              \
    type ret = to_##typename(-1, def, enable_log, failed);         \
    settop(sz);                                                    \
    return ret;                                                    \
  }                                                                \
  type eval_##typename(const std::string& expr,                    \
                       const type&        def        = default,    \
                       bool               enable_log = true,       \
                       bool*              failed     = nullptr) {                   \
    return eval_##typename(expr.c_str(), def, enable_log, failed); \
  }

  DEFINE_EVAL(int, int, 0)
  DEFINE_EVAL(uint, unsigned int, 0)
  DEFINE_EVAL(llong, long long, 0)
  DEFINE_EVAL(ullong, unsigned long long, 0)
  DEFINE_EVAL(bool, bool, false)
  DEFINE_EVAL(double, double, 0)
  DEFINE_EVAL(string, std::string, "")
#undef DEFINE_EVAL

  // Caller is responsible for popping stack if succeeds
  const char* eval_c_str(const char* expr,
                         const char* def        = "",
                         bool        enable_log = true,
                         bool*       failed     = nullptr) {
    int sz = gettop();
    if (dostring(expr) != LUA_OK) {
      if (failed) *failed = true;
      if (enable_log) log_error_in_stack();
      settop(sz);
      return def;
    }
    assert(gettop() >= sz);
    if (gettop() <= sz) {
      if (failed) *failed = true;
      if (enable_log) log_error("No return");
      return def;
    }
    return to_c_str(-1, def, enable_log, failed);
  }
  const char* eval_c_str(const std::string& expr,
                         const char*        def        = "",
                         bool               enable_log = true,
                         bool*              failed     = nullptr) {
    return eval_c_str(expr.c_str(), def, enable_log, failed);
  }

  /** @}*/

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
              // package or table
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
            while (isalnum(*s) || *s == '_' || *s == '.') ++s;
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
};  // namespace peacalm

const std::unordered_set<std::string> lua_wrapper::lua_key_words{
    "nil",      "true",  "false",    "and",   "or",     "not",
    "if",       "then",  "elseif",   "else",  "end",    "for",
    "do",       "while", "repeat",   "until", "return", "break",
    "continue", "goto",  "function", "in",    "local"};

////////////////////////////////////////////////////////////////////////////////

// CRTP: curious recurring template pattern
// Derived class needs to implement:
//     void provide_variables(const std::vector<std::string>& vars)
template <typename Derived>
class lua_wrapper_crtp : public lua_wrapper {
  using base_t = lua_wrapper;

public:
  template <typename... Args>
  lua_wrapper_crtp(Args&&... args) : base_t(std::forward<Args>(args)...) {}

  // Set global variables to Lua
  void prepare(const char* expr) {
    std::vector<std::string> vars = base_t::detect_variable_names(expr);
    static_cast<Derived*>(this)->provide_variables(vars);
  }
  void prepare(const std::string& expr) { prepare(expr.c_str()); }

  /**
   * @brief Evaluate a Lua expression meanwhile can retrieve variables needed
   * from variable provider automatically, then get the result in C++ type
   *
   * @param [in] expr Lua expression, which must have a return value
   * @param [in] default The default value returned if failed
   * @param [in] enable_log Whether print a log when exception occurs
   * @param [out] failed Will be set whether the operation is failed if this
   * pointer is not nullptr
   *
   * @{
   */

  // auto eval: prepare variables automatically
#define DEFINE_EVAL(typename, type, default)                                  \
  type auto_eval_##typename(const char* expr,                                 \
                            const type& def        = default,                 \
                            bool        enable_log = true,                    \
                            bool*       failed     = nullptr) {                         \
    prepare(expr);                                                            \
    return base_t::eval_##typename(expr, def, enable_log, failed);            \
  }                                                                           \
  type auto_eval_##typename(const std::string& expr,                          \
                            const type&        def        = default,          \
                            bool               enable_log = true,             \
                            bool*              failed     = nullptr) {                         \
    return this->auto_eval_##typename(expr.c_str(), def, enable_log, failed); \
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
                              const char* def        = "",
                              bool        enable_log = true,
                              bool*       failed     = nullptr) {
    prepare(expr);
    return base_t::eval_c_str(expr, def, enable_log, failed);
  }
  const char* auto_eval_c_str(const std::string& expr,
                              const char*        def        = "",
                              bool               enable_log = true,
                              bool*              failed     = nullptr) {
    return this->auto_eval_c_str(expr.c_str(), def, enable_log, failed);
  }

  /** @}*/
};

// Usage examples of lua_wrapper_crtp
// VariableProviderType should implement member function:
//     void provide_variables(const std::vector<std::string> &vars,
//                            lua_wrapper* l);

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

  void provide_variables(const std::vector<std::string>& vars) {
    provider().provide_variables(vars, this);
  }
};

namespace lua_wrapper_internal {
template <typename T>
struct __is_ptr : std::false_type {};
template <typename T>
struct __is_ptr<T*> : std::true_type {};
template <typename T>
struct __is_ptr<std::shared_ptr<T>> : std::true_type {};
template <typename T>
struct __is_ptr<std::unique_ptr<T>> : std::true_type {};
template <typename T>
struct is_ptr : __is_ptr<typename std::decay<T>::type> {};
}  // namespace lua_wrapper_internal

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

  void provide_variables(const std::vector<std::string>& vars) {
    __provide_variables(vars, lua_wrapper_internal::is_ptr<provider_t>{});
  }

private:
  void __provide_variables(const std::vector<std::string>& vars,
                           std::true_type) {
    provider()->provide_variables(vars, this);
  }
  void __provide_variables(const std::vector<std::string>& vars,
                           std::false_type) {
    provider().provide_variables(vars, this);
  }
};

}  // namespace peacalm

#endif  // LUA_WRAPPER_H_
