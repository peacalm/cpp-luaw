# Lua Wrapper for C++

[![Build](https://github.com/peacalm/cpp-lua_wrapper/actions/workflows/ci.yml/badge.svg)](https://github.com/peacalm/cpp-lua_wrapper/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

This library is a simple wrapper of Lua which makes interaction between Lua and 
C++ easier.

* Lua version >= 5.4
* C++ version >= C++14

Features:
* set or get values from Lua
* eval string Lua expression in C++ to get values



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
