--这是一个标准的Lua PassTcp脚本
--提供发送事件的封装和接收到数据事件的封装
--你可以在这里组织你的发送数据和验证接收数据
--这个接口这样设计，主要是考虑到多线程操作，尽量减少不必要的线程锁
--所以这里的数据块不要在脚本里声明出来，外层不管释放
--目前最大支持100K的数据块大小
--add by freeeyes

--要发送数据的事件，由PassTCP调用，这里实现你的发送数据封包
function PassTcp_CreateSendData(szData, nLen)
    --创建发送数据
	--szData为char* 是你要填充的缓冲块。
	--nLen是当前缓冲块最大长度
	--如果nNextIndex返回-1，则说明赋值错误,否则返回下一次的起始的位置
	
	--测试打印
	LuaFn_Tcp_Print(szData, nLen)
	
	--添加4字节包头
	nPacketSize = 10
	nStartIndex  = 0
	nNextIndex  = 0
	nNextIndex = Lua_Tcp_Buffer_In_Int32(szData, nPacketSize, nStartIndex, nLen)
	
	--添加2字节的命令字
	nCommandID = 4096
	nNextIndex = Lua_Tcp_Buffer_In_Int16(szData, nCommandID, nNextIndex, nLen)
	
	--添加数据块
	szBuff = '12345678'
	nNextIndex = Lua_Tcp_Buffer_In_Block(szData, szBuff, 8, nNextIndex, nLen)
	
	--这里必须返回填充数据块的长度，让PassTCP知道发送数据总长度
	return nNextIndex
end

--接收到的数据事件
function PassTcp_GetRecvData(szData, nLen)
    --得到接收数据
	--szData是当前收到的数据块，
	--nLen是收到的数据库块长度
	
	return true;
end


