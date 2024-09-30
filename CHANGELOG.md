# Change Log (Version - Release Time)


## Currently Unreleased - TBD

(nothing)


## v1.2.0 - 2024.9.29

* Check accessing member or dynamic member by empty smart pointer. Raise error, do not abort.
* When getting a registered member by an object, add same cv- property on the member as the object.
* New feature: register member's pointer and (low-level) const pointer. (`register_member_ptr`, `register_member_cptr`)
* New feature: register member's reference and (low-level) const reference. (`register_member_ref`, `register_member_cref`)
* Use "constexpr if statement" if supports C++17.
* New feature: set raw pointer by wrapper. The wrapped pointer will be a full userdata, and can access members like raw pointer. (`set_ptr_by_wrapper`)
* Add method: get metatable name of a global variable or its subfild/member. (`get_metatable_name`)


## v1.1.2 - 2024.9.22

* Add a macro switch for user to determine whether support volatile class objects 
(access members by a volatile qualified class object, pointer or smart pointer). 
* * Do not support this by default.
* * Could add a macro definition `PEACALM_LUAW_NEED_VOLATILE` to support it during compilation.

* By default, when registering fake member variables, the type declaration for the first argument could be `const Obj*`, not `const volatile Obj*` required anymore, of course, if the macro switch for volatile class objects is not enabled.
* * Of course, using `auto *` is still ok.


## v1.1.1 - 2024.9.11

* Support using `auto*` as the first argument's type declaration, not only `const volatile Obj*`, when registering fake member variables by lambda.


## v1.1.0 - 2024.9.8

* Support registering static member variables and static member functions.


## v1.0.2 - 2024.9.6

* Add a member function `result_enough()` to check whether got enough results for `luaw::function`.
* `std::unique_ptr` with user defined deleter does not share metatable with that with default deleter anymore. Enable its own destructor at `__gc`.
* Add operation APIs for light userdata's metatable.
* Do not support nested smart pointers.
* Bugfix: object destruction & metatable name conflict by type T and cv-T.


## v1.0.1 - 2024.8.27

* Check error of calling member function by wrong syntax: `obj.func()`. (The correct way is: `obj:func()`)


## v1.0.0 - 2023.7.17

* Initial release
