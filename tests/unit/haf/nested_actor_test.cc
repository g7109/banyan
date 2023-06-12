/**
 * DAIL in Alibaba Group
 *
 */

#include <seastar/haf/actor_implementation.hh>
#include <seastar/haf/util/data_type.hh>
#include <seastar/core/app-template.hh>
#include <seastar/core/fstream.hh>
#include <seastar/core/print.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/sleep.hh>
#include <seastar/core/thread.hh>
#include <thread>
#include <random>
#include <ctime>

using namespace seastar;
using namespace std::chrono_literals;

std::map<std::pair<unsigned, unsigned>, unsigned> num_tsk_per_usr_op;

class out : public stateless_actor {
public:
    explicit out(execution_context_base *exec_ctx, actor::actor_reference *parent,
        const byte_t *addr, byte_t addr_len) : stateless_actor(exec_ctx, parent, addr, addr_len) {}

    future<stop_reaction> do_work(actor_message* msg) override {
        auto *real_msg = reinterpret_cast<actor_message_with_payload<simple_cstring>*>(msg);
        return do_real_work(std::move(real_msg->data));
    }

    inline future<stop_reaction> do_real_work(simple_cstring&& data) {
        return now().then([data = std::move(data)] () mutable {
            std::this_thread::sleep_for(std::chrono::microseconds(20));
            return make_ready_future<stop_reaction>(stop_reaction::no);
        }, this);
    }
};


class count : public stateful_actor {
    int count_res = 0;
public:
    explicit count(execution_context_base *exec_ctx, actor::actor_reference *parent,
        const byte_t *addr, byte_t addr_len) : stateful_actor(exec_ctx, parent, addr, addr_len) {}

    future<stop_reaction> do_work(actor_message* msg) override {
        auto *real_msg = reinterpret_cast<actor_message_with_payload<simple_cstring>*>(msg);
        return do_real_work(std::move(real_msg->data));
    }

    inline future<stop_reaction> do_real_work(simple_cstring&& data) {
        return now().then([this] {
            std::this_thread::sleep_for(std::chrono::microseconds(20));
            if (++count_res <= 5) {
                return make_ready_future<stop_reaction>(stop_reaction::no);
            } else {
                return make_ready_future<stop_reaction>(stop_reaction::yes);
            }
        }, this);
    }
};


class scope_cancel : public stateful_actor {
    int count_res = 0;
public:
    explicit scope_cancel(execution_context_base *exec_ctx, actor::actor_reference *parent,
        const byte_t *addr, byte_t addr_len) : stateful_actor(exec_ctx, parent, addr, addr_len) {}

    future<stop_reaction> do_work(actor_message* msg) override {
        auto *real_msg = reinterpret_cast<actor_message_with_payload<simple_cstring>*>(msg);
        return do_real_work(std::move(real_msg->data));
    }

    inline future<stop_reaction> do_real_work(simple_cstring&& data) {
        return async([] {
            auto f = open_file_dma("f_out_blocking.tmp", open_flags::rw | open_flags::create).get0();
            f.close().get();
        }).then([this] () mutable {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            return ++count_res == 3;
        }, this).then([this] (bool stop) {
            if (stop) {
                address scope_address{};
                scope_address.length = 4;
                // get the actor_group addr and the first byte
                memcpy(scope_address.data, _address , scope_address.length);
                fmt::print("Cancel scope: {} \n", char_arr_to_str(scope_address.data, scope_address.length));
                broadcast(message_type::CANCEL_SCOPE, scope_address);
            }
            return make_ready_future<stop_reaction>(stop_reaction::no);
        }, this);
    }
};

class dummy_actor_group_3 : public actor_group {
public:
    explicit dummy_actor_group_3(execution_context_base *exec_ctx, actor::actor_reference *parent,
        const byte_t *addr, byte_t addr_len) : actor_group(exec_ctx, parent, addr, addr_len) {
        fmt::print("dummy actor group 3: {} is created \n", char_arr_to_str(addr, addr_len));
    }
};

class dummy_actor_group_4 : public actor_group {
public:
    explicit dummy_actor_group_4(execution_context_base *exec_ctx, actor::actor_reference *parent,
        const byte_t *addr, byte_t addr_len) : actor_group(exec_ctx, parent, addr, addr_len) {
        fmt::print("dummy actor group 4: {} is created \n", char_arr_to_str(addr, addr_len));
    }
};

namespace auto_registeration {

using namespace seastar::registration;
actor_registration<out> _out_op(1);
actor_registration<count> _count_op(2);
actor_registration<dummy_actor_group_3> _dag_3(3);
actor_registration<dummy_actor_group_4> _dag_4(4);
// 5100 in char array of this test;
actor_registration<scope_cancel> _scope_cancel_op(261);

} // namespace auto_registeration

future<> generate_input_messages_random(int max_ag_num, int max_op_id, int max_msg) {
    std::default_random_engine rand_engine(
            static_cast<unsigned long>(std::chrono::system_clock::now().time_since_epoch().count()));
    return now().then([max_ag_num, max_op_id, max_msg, rand_engine] () mutable {
        return parallel_for_each(boost::irange(0, max_msg), [&] (int idx) {
            std::uniform_int_distribution<unsigned> rand_ag_num(1, max_ag_num);
            std::uniform_int_distribution<unsigned> rand_op(1, max_op_id);

            unsigned tsk_id = 0;
            unsigned ag_num = rand_ag_num(rand_engine);
            unsigned op_id = rand_op(rand_engine);
            auto search = num_tsk_per_usr_op.find(std::make_pair(ag_num, op_id));
            if (search == num_tsk_per_usr_op.end()) {
                num_tsk_per_usr_op.insert(std::make_pair(std::make_pair(ag_num, op_id), ++tsk_id));
            } else {
                tsk_id = ++search->second;
            }
            if(ag_num == 1) {
                if (op_id == 3) { op_id = 1; }
                std::string data{"#Actor_Group: " + std::to_string(ag_num) + ", op " + std::to_string(op_id) +
                              ", message " + std::to_string(tsk_id - 1) + " id: " + std::to_string(idx)};
                address dst_addr{};
                unsigned dst_id[2] = {3, op_id};
                dst_addr.length = sizeof(unsigned) * 2;
                memcpy(dst_addr.data, dst_id, dst_addr.length);

                auto *msg = make_actor_message(1, message_type::USER, dst_addr,
                    simple_cstring{data.c_str(), data.size() + 1});
                return actor::channel::send(msg);
            } else {
                if (op_id == 3) { op_id = 261; }
                std::string data{"#Actor_Group: " + std::to_string(ag_num) + ", op " + std::to_string(op_id) +
                                 ", message " + std::to_string(tsk_id - 1) + " id: " + std::to_string(idx)};
                address dst_addr{};
                unsigned dst_id[3] = {3, 4, op_id};
                dst_addr.length = sizeof(unsigned) * 3;
                memcpy(dst_addr.data, dst_id, dst_addr.length);

                auto *msg = make_actor_message(1, message_type::USER, dst_addr,
                    simple_cstring{data.c_str(), data.size() + 1});
                return actor::channel::send(msg);
            }
        });
    }).then([] {
        fmt::print("Completed dispatching messages to parent actors, start executing on local reactor...\n");
    });
}

void print_msg_summary() {
    for (auto &it : num_tsk_per_usr_op) {
        fmt::print("Usr {}, op {} has msg {} in total\n", it.first.first, it.first.second, it.second);
    }
}

future<> send_peace_stop_message() {
    return parallel_for_each(boost::irange<unsigned>(0, 4), [=] (unsigned id) {
        address dst_addr{};
        dst_addr.length = 0;
        auto *msg = make_actor_message(id, message_type::PEACE_STOP, dst_addr);
        return actor::channel::send(msg);
    });
}

int main(int ac, char** av) {
    app_template().run(ac, av, [] {
        fmt::print("Actor test started.\n");
        return now().then([] {
            generate_input_messages_random(
                    2   /** =max_actor_group_num */,
                    3   /** =max_op_id */,
                    100 /** =num_msg */);
        }).then([] {
            return sleep(1s);
        }).then([] {
            generate_input_messages_random(
                    2   /** =max_user_id */,
                    3   /** =max_op_id */,
                    100 /** =num_msg */);
        }).then([] {
            send_peace_stop_message();
            return sleep(1s);
        }).then([] {
            print_msg_summary();
            fmt::print("**** Test Success!!! ****\n");
        });
    });
    return 0;
}

