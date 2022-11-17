#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>

#include <event2/util.h>
#include <event2/event.h>

#include "common/defines.h"
#include "common/logger.h"
#include "common/utils/time.h"
#include "network/defines.h"
#include "network/unpack.h"
#include "network/pool.h"
#include "libevent_ctx.h"
#include "libevent_handle.h"

namespace network {

void OnRecvNetCmdCtrl(int32_t iRecvCtrlFd, short events, void* pstVoidServerCtx);

void OnExecuteNetCmdCtrl(struct event_base* pstEventBase, ServerNetBufferCtx& rstServerCtx, int32_t iRecvCtrlFd, struct ST_NETWORK_CMD_REQUEST& rstNetCmd);

void ExecuteCmdCloseEventLoop(struct event_base* pstEventBase);

size_t ExecuteCmdSendAllPacket(ServerNetBufferCtx& rstServerCtx, struct ST_NETWORK_CMD_REQUEST& rstNetCmd);

} // namespace network
