// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/debug/crash_logging.h"
#include "base/debug/debugger.h"
#include "base/files/file_path.h"
#include "base/i18n/rtl.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/threading/platform_thread.h"
#include "build/build_config.h"
#include "content/child/child_process.h"
#include "content/common/content_constants_internal.h"
#include "content/common/sandbox_linux/sandbox_linux.h"
#include "content/ppapi_plugin/ppapi_thread.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"
#include "ipc/ipc_sender.h"
#include "ppapi/proxy/plugin_globals.h"
#include "ppapi/proxy/proxy_module.h"
#include "ui/base/ui_base_switches.h"

#if defined(OS_WIN)
#include "base/win/win_util.h"
#include "base/win/windows_version.h"
#include "content/child/dwrite_font_proxy/dwrite_font_proxy_init_win.h"
#include "sandbox/win/src/sandbox.h"
#include "third_party/WebKit/public/web/win/WebFontRendering.h"
#include "third_party/skia/include/ports/SkTypeface_win.h"
#include "ui/display/win/dpi.h"
#include "ui/gfx/font_render_params.h"
#include "ui/gfx/win/direct_write.h"
#endif

#if defined(OS_CHROMEOS)
#include "base/files/file_util.h"
#endif

#if defined(OS_LINUX)
#include "content/public/common/sandbox_init.h"
#endif

#ifdef V8_USE_EXTERNAL_STARTUP_DATA
#include "gin/v8_initializer.h"
#endif

#if defined(OS_POSIX) && !defined(OS_ANDROID)
#include <stdlib.h>
#endif

#if defined(OS_WIN)
sandbox::TargetServices* g_target_services = NULL;
#else
void* g_target_services = 0;
#endif

namespace content {

// Main function for starting the PPAPI plugin process.
int PpapiPluginMain(const MainFunctionParams& parameters) {
  const base::CommandLine& command_line = parameters.command_line;

#if defined(OS_WIN)
  g_target_services = parameters.sandbox_info->target_services;
#endif

  // If |g_target_services| is not null this process is sandboxed. One side
  // effect is that we can't pop dialogs like ChildProcess::WaitForDebugger()
  // does.
  if (command_line.HasSwitch(switches::kPpapiStartupDialog)) {
    if (g_target_services)
      base::debug::WaitForDebugger(2*60, false);
    else
      ChildProcess::WaitForDebugger("Ppapi");
  }

  // Set the default locale to be the current UI language. WebKit uses ICU's
  // default locale for some font settings (especially switching between
  // Japanese and Chinese fonts for the same characters).
  if (command_line.HasSwitch(switches::kLang)) {
    std::string locale = command_line.GetSwitchValueASCII(switches::kLang);
    base::i18n::SetICUDefaultLocale(locale);

#if defined(OS_POSIX) && !defined(OS_ANDROID)
    // TODO(shess): Flash appears to have a POSIX locale dependency
    // outside of the existing PPAPI ICU support.  Certain games hang
    // while loading, and it seems related to datetime formatting.
    // http://crbug.com/155396
    // http://crbug.com/155671
    //
    // ICU can accept "en-US" or "en_US", but POSIX wants "en_US".
    std::replace(locale.begin(), locale.end(), '-', '_');
    locale.append(".UTF-8");
    setlocale(LC_ALL, locale.c_str());
    setenv("LANG", locale.c_str(), 0);
#endif
  }

#if defined(OS_CHROMEOS)
  // Specifies $HOME explicitly because some plugins rely on $HOME but
  // no other part of Chrome OS uses that.  See crbug.com/335290.
  base::FilePath homedir;
  PathService::Get(base::DIR_HOME, &homedir);
  setenv("HOME", homedir.value().c_str(), 1);
#endif

  base::MessageLoop main_message_loop;
  base::PlatformThread::SetName("CrPPAPIMain");
  base::trace_event::TraceLog::GetInstance()->SetProcessName("PPAPI Process");
  base::trace_event::TraceLog::GetInstance()->SetProcessSortIndex(
      kTraceEventPpapiProcessSortIndex);

#ifdef V8_USE_EXTERNAL_STARTUP_DATA
  gin::V8Initializer::LoadV8Snapshot();
  gin::V8Initializer::LoadV8Natives();
#endif

#if defined(OS_LINUX)
  //LinuxSandbox::InitializeSandbox();
#endif

  ChildProcess ppapi_process;
  ppapi_process.set_main_thread(
      new PpapiThread(parameters.command_line, false));  // Not a broker.

#if defined(OS_WIN)
  if (!base::win::IsUser32AndGdi32Available())
    gfx::win::MaybeInitializeDirectWrite();
  InitializeDWriteFontProxy();

  blink::WebFontRendering::setDeviceScaleFactor(display::win::GetDPIScale());

  const gfx::FontRenderParams font_params =
      gfx::GetFontRenderParams(gfx::FontRenderParamsQuery(), nullptr);
  blink::WebFontRendering::setAntialiasedTextEnabled(font_params.antialiasing);
  blink::WebFontRendering::setLCDTextEnabled(
      font_params.subpixel_rendering !=
      gfx::FontRenderParams::SUBPIXEL_RENDERING_NONE);
#endif

  main_message_loop.Run();

#if defined(OS_WIN)
  UninitializeDWriteFontProxy();
#endif
  return 0;
}

}  // namespace content
