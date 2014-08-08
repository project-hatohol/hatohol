# Coding conventions

## C++
### Naming
Variable: lower camel case.

	domainName

Class: upper camel case
File name: upper camel case

	DatabaseConnector

Function argument:
An input parameter shall be 'const TYPE &'.

### Standards & Libraries
#### Using
- STL
  * Containers such as list, vector and map.
  * String (We use UTF-8. So wstring is not used)
- GLib
  * Event loop
  * I/O functions associated with the event loop such as timer, idle, and GIOChannel.
- pthread
  * Actually features are wrapped by HatoholThreadBase class and MLPL (a library for Hatohol).

#### Not using
- Boost
- C++11
However, we'll begin to use features of C++11 in near future. Probably it is
when we support CentOS7 (g++4.7) instead of CentOS6 (g++4.4) series.
Of course, we can now use features supported by g++4.4.

### Others
- Don't use 'using namespace' in a header file.
- Use 'static const' instead of #define to define a constant.
- Don't return const object such as 'const string func()'.
NOTE: If a function returns a reference, this convention is not applied.
      E.g. 'const string &func()' is OK

- Use '#include <hoge.h>' (not #include "hoge.h") for MLPL headers.
- Add 'override' at the end of the prototype of the overridden method as

    virtual overriddenMetho(void) override

This keyword causes a build error if there's no corresponding virtual method
in the base classes. As a result, we can detect the mistake of the definition.

When the compiler without C++11 features is used, the 'override' is just ignored
by a trick in a header of Hatohol.

- Basically use Pimpl idiom
Many existing classes have private members in the structure named PrivateContext,
which is defined in the source (.cc) file.
However, for classes that need performance, it is allowed to define private
members in the heder file.

## Python
Basically follow PEP 8

Indent: 4 spaces
String: Single quotation

## JavaScript

Indent: 2 spaces, no tabs.
String: Single quotation
