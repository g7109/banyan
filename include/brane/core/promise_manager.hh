/**
 * DAIL in Alibaba Group
 *
 */

#pragma once

#include <seastar/core/future.hh>

namespace brane {

template <typename... T>
struct pr_manager {
    struct node {
        seastar::promise<T...> pr;
        uint32_t next_avl;
        bool active;

        node(): pr(), next_avl(0), active(true) {}
        node(const node&) = delete;
        node(node&& other) noexcept
            : pr(std::move(other.pr))
            , next_avl(other.next_avl)
            , active(other.active) {}
        ~node() = default;

        inline void occupy() {
            pr = seastar::promise<T...>{};
            active = true;
        }

        inline void release() {
            active = false;
        }
    };
    std::vector<node> pr_array;
    uint32_t available;

    explicit pr_manager(const size_t init_reserve_size = 0): pr_array(), available(0) {
        pr_array.reserve(init_reserve_size + 1);
        pr_array.emplace_back();
        pr_array[0].pr.set_value(std::tuple<T...>{});
        pr_array[0].release();
    }
    ~pr_manager() = default;

    inline uint32_t acquire_pr() {
        if(available == 0) {
            pr_array.emplace_back();
            return static_cast<uint32_t>(pr_array.size() - 1);
        }
        auto pr_id = available;
        available = pr_array[pr_id].next_avl;
        pr_array[pr_id].occupy();
        return pr_id;
    }

    inline void remove_pr(uint32_t pr_id) {
        pr_array[pr_id].release();
        pr_array[pr_id].next_avl = available;
        available = pr_id;
    }

    inline uint32_t size() const {
        return static_cast<uint32_t>(pr_array.size());
    }

    inline bool is_active(uint32_t pr_id) const {
        return pr_array[pr_id].active;
    }

    inline seastar::future<T...> get_future(uint32_t pr_id) noexcept {
        return pr_array[pr_id].pr.get_future();
    }

    inline
    void set_value(uint32_t pr_id, const std::tuple<T...>& result) noexcept {
        pr_array[pr_id].pr.set_value(result);
    }

    inline
    void set_value(uint32_t pr_id, std::tuple<T...>&& result) noexcept {
        pr_array[pr_id].pr.set_value(std::move(result));
    }

    template <typename... A>
    inline
    void set_value(uint32_t pr_id, A&&... a) noexcept {
        pr_array[pr_id].pr.set_value(std::forward<A>(a)...);
    }

    inline
    void set_exception(uint32_t pr_id, std::exception_ptr ex) noexcept {
        pr_array[pr_id].pr.set_exception(std::move(ex));
    }
};

} // namespace brane
