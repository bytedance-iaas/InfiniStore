import infinistore
import uuid
import asyncio
import ctypes
import time


def generate_uuid():
    return str(uuid.uuid4())


config = infinistore.ClientConfig(
    host_addr="127.0.0.1",
    service_port=12345,
    log_level="warning",
    connection_type=infinistore.TYPE_RDMA,
    ib_port=1,
    link_type=infinistore.LINK_ETHERNET,
    dev_name="mlx5_0",
)


def get_ptr(mv: memoryview):
    return ctypes.addressof(ctypes.c_char.from_buffer(mv))


async def main():
    rdma_conn = infinistore.InfinityConnection(config)

    # FIXME: This is a blocking call, should be async
    await rdma_conn.connect_async()

    key = generate_uuid()

    # src = torch.randn(4096, device="cpu", dtype=torch.float32)
    # dst = torch.zeros(4096, device="cpu", dtype=torch.float32)
    size = 128 * 1024
    src = bytearray(size)
    dst = memoryview(bytearray(size))

    def register_mr():
        rdma_conn.register_mr(get_ptr(src), len(src))
        rdma_conn.register_mr(get_ptr(dst), len(dst))

    await asyncio.to_thread(register_mr)

    is_exist = await asyncio.to_thread(rdma_conn.check_exist, key)
    assert not is_exist

    now = time.time()
    tasks = []
    for i in range(1000):
        # tasks.append(
        #     rdma_conn.rdma_write_cache_single_async(
        #         key + str(i), get_ptr(src), len(src)
        #     )
        # )
        await rdma_conn.rdma_write_cache_single_async(
            key + str(i), get_ptr(src), len(src)
        )
    # await asyncio.gather(*tasks, return_exceptions=True)

    # await asyncio.gather(*tasks, return_exceptions=True)
    print("write Time taken: ", time.time() - now)

    now = time.time()
    tasks = []
    for i in range(1000):
        tasks.append(
            rdma_conn.read_cache_single_async(key + str(i), get_ptr(dst), len(dst))
        )
    # try:
    #     await rdma_conn.read_cache_single_async(key+"0", get_ptr(dst), len(dst))
    # except infinistore.InfiniStoreKeyNotFound:
    #     print("Key not found")

    await asyncio.gather(*tasks, return_exceptions=True)
    print("read Time taken: ", time.time() - now)

    assert src == dst

    rdma_conn.close()


asyncio.run(main())
