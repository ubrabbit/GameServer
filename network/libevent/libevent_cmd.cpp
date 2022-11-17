#include "libevent_cmd.h"

namespace network {

void OnRecvNetCmdCtrl(int32_t iRecvCtrlFd, short events, void* pstVoidEventHandle)
{
	char buf[ST_NETWORK_CMD_REQUEST_MAX_SIZE] = {0};
	int ret = read(iRecvCtrlFd, buf, sizeof(buf));
	if(ret < 0){
		LOGINFO("recv net cmd error: {}", ret);
		return;
	}

	ST_LibeventEventHandle* pstEventHandle = (ST_LibeventEventHandle*)pstVoidEventHandle;
	assert(pstEventHandle);

	struct ST_NETWORK_CMD_REQUEST* pstNetCmd = (struct ST_NETWORK_CMD_REQUEST*)buf;
	//LOGINFO("recv net cmd: {}", pstNetCmd->m_Header[0]);
	assert(pstEventHandle->m_pstEventBase);
	assert(pstEventHandle->m_pstServerCtx);
	OnExecuteNetCmdCtrl(pstEventHandle->m_pstEventBase, *(pstEventHandle->m_pstServerCtx), iRecvCtrlFd, *pstNetCmd);
}

void OnExecuteNetCmdCtrl(struct event_base* pstEventBase, ServerContext& rstServerCtx, int32_t iRecvCtrlFd, struct ST_NETWORK_CMD_REQUEST& rstNetCmd)
{
	BYTE bchNetCmd = rstNetCmd.GetCmd();

	switch(bchNetCmd)
	{
		case NETWORK_CMD_REQUEST_SEND_ALL_PACKET:
            ExecuteCmdSendAllPacket(rstServerCtx, rstNetCmd);
			break;
		case NETWORK_CMD_REQUEST_CLOSE_SERVER:
			ExecuteCmdCloseEventLoop(pstEventBase);
			break;
		default:
			LOGERROR("unknown net cmd: {}", (char)bchNetCmd);
			break;
	}
}

#define NETWORK_CMD_SEND_PACKET_CACHE_SIZE (100)
size_t ExecuteCmdSendAllPacket(ServerContext& rstServerCtx, struct ST_NETWORK_CMD_REQUEST& rstNetCmd)
{
	size_t dwStartTime = GetMilliSecond();
	//LOGINFO("ExecuteCmdSendAllPacket Start: {}", iStart);

	std::vector<NetPacketBuffer> vecNetPacketSendBuffer;
	rstServerCtx.CopyPacketsFromLogicToNet(vecNetPacketSendBuffer);

	int32_t iTotalPackets = vecNetPacketSendBuffer.size();
	for(auto it=vecNetPacketSendBuffer.begin(); it!=vecNetPacketSendBuffer.end(); it++)
	{
		NetPacketBuffer& rstPacket = *it;
		int32_t iClientFd = rstPacket.m_stHeader.m_iSockFd;
		LibeventClientCtx* pstClientCtx = (LibeventClientCtx*)rstServerCtx.m_stClientPool.GetClient(iClientFd);
		if(NULL == pstClientCtx)
		{
			//LOGERROR("send packet to client<{}> but connect closed.", iClientFd);
			continue;
		}
		assert(pstClientCtx->m_pstBufferEvent);

		BYTE chHeaderArray[sizeof(int16_t)*2] = {'\0'};
		BYTE chTailArray[NETWORK_PACKET_TAIL_SIZE] = {'\0'}; 		// 尾部增加一个时间戳

		PackInt(chHeaderArray, (int16_t)(rstPacket.GetPacketSize() + sizeof(chTailArray)));
		PackInt(chHeaderArray + sizeof(int16_t), (int16_t)rstPacket.GetProtoNo());
		PackInt(chTailArray, (size_t)GetMilliSecond());

		bufferevent_write(pstClientCtx->m_pstBufferEvent, chHeaderArray, sizeof(chHeaderArray));
		bufferevent_write(pstClientCtx->m_pstBufferEvent, rstPacket.GetBuffer(), rstPacket.GetBufferSize());
		bufferevent_write(pstClientCtx->m_pstBufferEvent, chTailArray, sizeof(chTailArray));
	}
	vecNetPacketSendBuffer.clear();

	size_t dwCostTime = GetMilliSecond() - dwStartTime;
	if(iTotalPackets > 0 && dwCostTime > 10)
	{
		LOGINFO("ExecuteCmdSendAllPacket Finished: SendPackets: {} Cost: {} ms", iTotalPackets, dwCostTime);
	}
    return iTotalPackets;
}

void ExecuteCmdCloseEventLoop(struct event_base* pstEventBase)
{
	LOGINFO("event loop quit by main thread signal");
	assert(pstEventBase);
	event_base_loopbreak(pstEventBase);
}

} // namespace network
