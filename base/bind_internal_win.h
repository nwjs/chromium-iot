// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Specializations of RunnableAdapter<> for Windows specific calling
// conventions.  Please see base/bind_internal.h for more info.

#ifndef BASE_BIND_INTERNAL_WIN_H_
#define BASE_BIND_INTERNAL_WIN_H_

#include <utility>

#include "build/build_config.h"

// In the x64 architecture in Windows, __fastcall, __stdcall, etc, are all
// the same as __cdecl which would turn the following specializations into
// multiple definitions.
#if !defined(ARCH_CPU_X86_64)

namespace base {
namespace internal {

template <typename Functor>
class RunnableAdapter;

// __stdcall Function.
template <typename R, typename... Args>
class RunnableAdapter<R(__stdcall *)(Args...)> {
 public:
  // MSVC 2013 doesn't support Type Alias of function types.
  // Revisit this after we update it to newer version.
  typedef R RunType(Args...);

  explicit RunnableAdapter(R(__stdcall *function)(Args...))
      : function_(function) {
  }

  template <typename... RunArgs>
  R Run(RunArgs&&... args) const {
    return function_(std::forward<RunArgs>(args)...);
  }

 private:
  R (__stdcall *function_)(Args...);
};

// __fastcall Function.
template <typename R, typename... Args>
class RunnableAdapter<R(__fastcall *)(Args...)> {
 public:
  // MSVC 2013 doesn't support Type Alias of function types.
  // Revisit this after we update it to newer version.
  typedef R RunType(Args...);

  explicit RunnableAdapter(R(__fastcall *function)(Args...))
      : function_(function) {
  }

  template <typename... RunArgs>
  R Run(RunArgs&&... args) const {
    return function_(std::forward<RunArgs>(args)...);
  }

 private:
  R (__fastcall *function_)(Args...);
};

}  // namespace internal
}  // namespace base

#endif  // !defined(ARCH_CPU_X86_64)

#endif  // BASE_BIND_INTERNAL_WIN_H_
