#pragma once

//#if BUILDING_LIBFOO && HAVE_VISIBILITY
//#define LIBFOO_DLL_EXPORTED __attribute__((__visibility__("default")))
//#elif BUILDING_LIBFOO && defined _MSC_VER
//#define LIBFOO_DLL_EXPORTED __declspec(dllexport)
//#elif defined _MSC_VER
//#define LIBFOO_DLL_EXPORTED __declspec(dllimport)
//#else
//#define LIBFOO_DLL_EXPORTED
//#endif

namespace FooLib {
  int __attribute__((__visibility__("default"))) foo(bool branch = false);
}

