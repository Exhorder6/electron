// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/node_bindings_win.h"

#include <windows.h>

#include "base/logging.h"
#include "base/sys_info.h"

extern "C" {
#include "vendor/node/deps/uv/src/win/internal.h"
}

namespace atom {

NodeBindingsWin::NodeBindingsWin(BrowserEnvironment browser_env)
    : NodeBindings(browser_env) {
  // on single-core the io comp port NumberOfConcurrentThreads needs to be 2
  // to avoid cpu pegging likely caused by a busy loop in PollEvents
  if (base::SysInfo::NumberOfProcessors() == 1) {
    // the expectation is the uv_loop_ has just been initialized
    // which makes iocp replacement safe
    CHECK_EQ(0u, uv_loop_->active_handles);
    CHECK_EQ(0u, uv_loop_->active_reqs.count);

    if (uv_loop_->iocp && uv_loop_->iocp != INVALID_HANDLE_VALUE)
      CloseHandle(uv_loop_->iocp);
    uv_loop_->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);
  }
}

NodeBindingsWin::~NodeBindingsWin() {}

void NodeBindingsWin::PollEvents() {
  // If there are other kinds of events pending, uv_backend_timeout will
  // instruct us not to wait.
  DWORD bytes, timeout;
  ULONG_PTR key;
  OVERLAPPED* overlapped;

  timeout = uv_backend_timeout(uv_loop_);

  GetQueuedCompletionStatus(uv_loop_->iocp, &bytes, &key, &overlapped, timeout);

  // Give the event back so libuv can deal with it.
  if (overlapped != NULL)
    PostQueuedCompletionStatus(uv_loop_->iocp, bytes, key, overlapped);
}

// static
NodeBindings* NodeBindings::Create(BrowserEnvironment browser_env) {
  return new NodeBindingsWin(browser_env);
}

}  // namespace atom
