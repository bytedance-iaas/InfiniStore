#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <assert.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <infiniband/verbs.h>

#include <cstddef>
#include <functional>
#include <vector>
#include <uv.h>

#define BLOCK_USAGE_RATIO 0.5
#define EXTEND_POOL_SIZE 10 << 30
#define EXTEND_BLOCK_SIZE 64 << 10

using AllocationCallback =
    std::function<void(void* ptr, uint32_t lkey, uint32_t rkey, int pool_idx)>;
using SimpleAllocationCallback = std::function<void(void* ptr, uint32_t lkey, uint32_t rkey)>;

class MemoryPool {
   public:
    MemoryPool(size_t pool_size, size_t block_size, struct ibv_pd* pd);

    ~MemoryPool();

    /*
    @brief size should be aligned to block size
    */
    int allocate(size_t size, size_t n, SimpleAllocationCallback callback);
    /*
    @brief size should be aligned to block size
    */
    void deallocate(void* ptr, size_t size);

    uint32_t get_lkey() const { return mr_->lkey; }
    uint32_t get_rkey() const { return mr_->rkey; }
    uint32_t get_total_blocks() const { return total_blocks_; }
    uint32_t get_allocated_blocks() const { return allocated_blocks_; }

   private:
    void* pool_;
    size_t pool_size_;
    size_t block_size_;
    size_t total_blocks_;
    size_t last_search_position_;
    size_t allocated_blocks_;

    // TODO: use judy libray to speed up the bitmap?
    std::vector<uint64_t> bitmap_;

    struct ibv_mr* mr_;
    struct ibv_pd* pd_;
};

typedef struct {
    MemoryPool *pool_ptr = NULL;
    struct ibv_pd* pd = NULL;
    std::vector<MemoryPool*> *mempools = NULL;
} mempool_wqueue_data_t;

class MM {
   private:
    std::vector<MemoryPool*> mempools_;
    float block_usage_ratio_;
    struct ibv_pd* pd_;
    uv_loop_t *loop_;
    uv_work_t *req_;
    mempool_wqueue_data_t *wqueue_data_;

   public:
    MM(size_t pool_size, size_t block_size, struct ibv_pd* pd, uv_loop_t *loop) {
        block_usage_ratio_ = BLOCK_USAGE_RATIO;
        pd_ = pd;
        loop_ = loop;
        req_ = new uv_work_t();
        wqueue_data_ = new mempool_wqueue_data_t();    
        wqueue_data_->pd = pd_;
        wqueue_data_->mempools = &mempools_;
        req_->data = (void *)wqueue_data_;
        mempools_.push_back(new MemoryPool(pool_size, block_size, pd));
    }
    MM(const MM& mm) = delete;
    bool allocate(size_t size, size_t n, AllocationCallback callback);
    void deallocate(void* ptr, size_t size, int pool_idx);
    uint32_t get_lkey(int pool_idx) const {
        assert(pool_idx >= 0 && pool_idx < mempools_.size());
        return mempools_[pool_idx]->get_lkey();
    }
    uint32_t get_rkey(int pool_idx) const {
        assert(pool_idx >= 0 && pool_idx < mempools_.size());
        return mempools_[pool_idx]->get_rkey();
    }

    ~MM() {
        for (auto& pool : mempools_) {
            delete pool;
        }
        delete req_;
        delete wqueue_data_;
    }
};

#endif  // MEMORY_POOL_H
