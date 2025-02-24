// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/apps/glass_app_window_frame_view_win.h"

#include "base/win/windows_version.h"
#include "extensions/browser/app_window/native_app_window.h"
#include "ui/base/hit_test.h"
#include "ui/display/win/dpi.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace {

const int kResizeAreaCornerSize = 16;

}  // namespace

const char GlassAppWindowFrameViewWin::kViewClassName[] =
    "ui/views/apps/GlassAppWindowFrameViewWin";

GlassAppWindowFrameViewWin::GlassAppWindowFrameViewWin(views::Widget* widget)
    : widget_(widget) {}

GlassAppWindowFrameViewWin::~GlassAppWindowFrameViewWin() {
}

gfx::Insets GlassAppWindowFrameViewWin::GetGlassInsets() const {
  int caption_height = display::win::GetSystemMetricsInDIP(SM_CYSIZEFRAME) +
                       display::win::GetSystemMetricsInDIP(SM_CYCAPTION);

  int frame_size = base::win::GetVersion() < base::win::VERSION_WIN10
                       ? display::win::GetSystemMetricsInDIP(SM_CXSIZEFRAME)
                       : 0;

  return gfx::Insets(caption_height, frame_size, frame_size, frame_size);
}

gfx::Insets GlassAppWindowFrameViewWin::GetClientAreaInsets() const {
  gfx::Insets insets;
  if (base::win::GetVersion() < base::win::VERSION_WIN10) {
    // This tells Windows that most of the window is a client area, meaning
    // Chrome will draw it. Windows still fills in the glass bits because of the
    // DwmExtendFrameIntoClientArea call in |UpdateDWMFrame|.
    // Without this 1 pixel offset on the right and bottom:
    //   * windows paint in a more standard way, and
    //   * we get weird black bars at the top when maximized in multiple monitor
    //     configurations.
    int border_thickness = 1;
    insets.Set(0, 0, border_thickness, border_thickness);
  } else {
    // On Windows 10 we use a 1 pixel non client border which is too thin as a
    // resize target. This inset extends the resize region.
    int resize_border = display::win::GetSystemMetricsInDIP(SM_CXSIZEFRAME);
    insets.Set(0, resize_border, resize_border, resize_border);
  }
  return insets;
}

gfx::Rect GlassAppWindowFrameViewWin::GetBoundsForClientView() const {
#if 1
  return bounds();
#else
  if (widget_->IsFullscreen())
    return bounds();

  gfx::Insets insets = GetGlassInsets();
  return gfx::Rect(insets.left(),
                   insets.top(),
                   std::max(0, width() - insets.left() - insets.right()),
                   std::max(0, height() - insets.top() - insets.bottom()));
#endif
}

gfx::Rect GlassAppWindowFrameViewWin::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  if (widget_->IsFullscreen())
    return bounds();

  gfx::Insets insets = GetGlassInsets();
  insets += GetClientAreaInsets();
  gfx::Rect window_bounds(
      client_bounds.x() - insets.left(), client_bounds.y() - insets.top(),
      client_bounds.width() + insets.left() + insets.right(),
      client_bounds.height() + insets.top() + insets.bottom());

  // Prevent the window size from being 0x0 during initialization.
  window_bounds.Union(gfx::Rect(0, 0, 1, 1));
  return window_bounds;
}

int GlassAppWindowFrameViewWin::NonClientHitTest(const gfx::Point& point) {
  if (widget_->IsFullscreen())
    return HTCLIENT;

  if (!bounds().Contains(point))
    return HTNOWHERE;

  int client_component = widget_->client_view()->NonClientHitTest(point);
  if (client_component != HTNOWHERE)
    return client_component;


  // Check the frame first, as we allow a small area overlapping the contents
  // to be used for resize handles.
  bool can_ever_resize = widget_->widget_delegate()
                             ? widget_->widget_delegate()->CanResize()
                             : false;
  // Don't allow overlapping resize handles when the window is maximized or
  // fullscreen, as it can't be resized in those states.
  int resize_border = display::win::GetSystemMetricsInDIP(SM_CXSIZEFRAME);
  int frame_component =
      GetHTComponentForFrame(point,
                             resize_border,
                             resize_border,
                             kResizeAreaCornerSize - resize_border,
                             kResizeAreaCornerSize - resize_border,
                             can_ever_resize);
  if (frame_component != HTNOWHERE)
    return frame_component;

  // Caption is a safe default.
  return HTCAPTION;
}

void GlassAppWindowFrameViewWin::GetWindowMask(const gfx::Size& size,
                                               gfx::Path* window_mask) {
  // We got nothing to say about no window mask.
}

gfx::Size GlassAppWindowFrameViewWin::GetPreferredSize() const {
  gfx::Size pref = widget_->client_view()->GetPreferredSize();
  gfx::Rect bounds(0, 0, pref.width(), pref.height());
  return widget_->non_client_view()
      ->GetWindowBoundsForClientBounds(bounds)
      .size();
}

const char* GlassAppWindowFrameViewWin::GetClassName() const {
  return kViewClassName;
}

gfx::Size GlassAppWindowFrameViewWin::GetMinimumSize() const {
  gfx::Size min_size = widget_->client_view()->GetMinimumSize();

  gfx::Insets insets = GetGlassInsets();
  min_size.Enlarge(insets.left() + insets.right(),
                   insets.top() + insets.bottom());

  return min_size;
}

gfx::Size GlassAppWindowFrameViewWin::GetMaximumSize() const {
  gfx::Size max_size = widget_->client_view()->GetMaximumSize();

  gfx::Insets insets = GetGlassInsets();
  if (max_size.width())
    max_size.Enlarge(insets.left() + insets.right(), 0);
  if (max_size.height())
    max_size.Enlarge(0, insets.top() + insets.bottom());

  return max_size;
}
