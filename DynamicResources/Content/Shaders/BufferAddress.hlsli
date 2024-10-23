//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef ADDR_BUFFER_SLOT
#define ADDR_BUFFER_SLOT	u0
#endif

#ifndef ADDR_BUFFER_SPACE
#define ADDR_BUFFER_SPACE	space2147420893
#endif

RWByteAddressBuffer g_memoryBuffer : register(ADDR_BUFFER_SLOT, ADDR_BUFFER_SPACE);

template<typename T>
T LoadMemory(uint64_t addr)
{
	return g_memoryBuffer.Load<T>(uint(addr & 0xffffffff));
}

template<typename T>
T LoadMemory2(uint2 addr)
{
	return g_memoryBuffer.Load<T>(addr.x);
}

template<typename T>
T StoreMemory(uint64_t addr, T data)
{
	return g_memoryBuffer.Store<T>(uint(addr & 0xffffffff), data);
}

template<typename T>
T StoreMemory2(uint2 addr, T data)
{
	return g_memoryBuffer.Store<T>(addr.x, data);
}
