#pragma once

#include <netinet/in.h>
#include <unordered_map>
#include <vector>
#include <brane/net/defs.hh>

#include "seastar/net/socket_defs.hh"

namespace brane {

class actor_system;

class network_config {
    using clients_map = std::unordered_map<::in_addr_t, uint32_t>;
    using server_list = std::vector<std::pair<seastar::socket_address, uint32_t>>;
    using sharded_list = std::vector<server_list>;
    using worker_node_list = std::vector<worker_node_info>;
public:
    static network_config& get() {
        static network_config instance;
        return instance;
    }

    inline uint32_t p2p_conn_count() const {
        return _p2p_conn_count;
    }

    inline uint32_t waiting_conn_count() const {
        return _wait_conn_count;
    }

    inline uint16_t listen_port() const {
        return _listen_port;
    }

    inline uint32_t machine_id() const {
        return _machine_id;
    }

    inline uint32_t num_machines() const {
        return _num_machines;
    }

    inline const worker_node_list& node_list() const {
        return _node_list;
    }

    inline uint32_t get_client_target_shard(::in_addr_t ia) {
        if (_addr_lsid_map.find(ia) == _addr_lsid_map.end()) {
            fmt::print("WARNING: can't find {} in _addr_lsid_map\n", ia);
            return 0;
        }
        uint32_t sid = _addr_lsid_map[ia];
        _addr_lsid_map[ia] = (sid + 1) % local_shard_count();
        return sid;
    }

    inline const server_list& get_server_list(uint32_t shard_id) const {
        return _server_lists[shard_id];
    }
private:
    network_config()
        : _p2p_conn_count(1)
        , _wait_conn_count(0)
        , _machine_id(0)
        , _num_machines(1)
        , _listen_port(0)
        , _addr_lsid_map()
        , _server_lists()
        , _node_list()
    {}

    // configurable peer-to-peer connection count.
    uint32_t _p2p_conn_count;
    // waiting connection count.
    uint32_t _wait_conn_count;
    // machine id
    uint32_t _machine_id;
    // number of machines
    uint32_t _num_machines;
    // listen port
    uint16_t _listen_port;
    // As server: in_address -> local_shard map
    clients_map _addr_lsid_map;
    // As clients: per shard target server addresses.
    sharded_list _server_lists;
    // worker node list.
    worker_node_list _node_list;

    friend class actor_system;
};

} // namespace brane
