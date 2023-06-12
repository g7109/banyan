//
// Created by everlighting on 2020/9/29.
//
#pragma once

#include <brane/util/common-utils.hh>
#include <brane/actor/reference_base.hh>
#include <brane/actor/actor_implementation.hh>
#include "berkeley_db/storage/bdb_graph_handler.hh"
#include "common/downstream_handlers.hh"

class i_q6_op_1x0_loopOut;
class i_q6_op_2_loopLeave_dedup_in;

class ANNOTATION(actor:reference) i_q6_op_1x1_loopUntil : public brane::reference_base {
public:
    void process(VertexBatch&& input);
    void receive_eos(Eos&& eos);

    // Constructor.
    ACTOR_ITFC_CTOR(i_q6_op_1x1_loopUntil);
    // Destructor
    ACTOR_ITFC_DTOR(i_q6_op_1x1_loopUntil);
};

class ANNOTATION(actor:implement) q6_op_1x1_loopUntil : public brane::stateful_actor {
public:
    void process(VertexBatch&& input);
    void receive_eos(Eos&& eos);

    void initialize() override;
    void finalize() override;

    // Constructor.
    ACTOR_IMPL_CTOR(q6_op_1x1_loopUntil);
    // Destructor
    ACTOR_IMPL_DTOR(q6_op_1x1_loopUntil);
    // Do work
    ACTOR_DO_WORK();

private:
    unsigned _eos_rcv_num = 0;
    const unsigned _expect_eos_num = brane::global_shard_count();
    bool _notify_ds = false;

    const unsigned _max_loop_times = 2;
    unsigned _cur_loop_times = 0;
    const bool _emit_flag = true;

    std::unordered_set<int64_t> _vertices_have_seen{};

    using next_loop_t = downstream_handler<i_q6_op_1x0_loopOut, false, VertexBatch>;
    using loop_sync_t = downstream_handler<i_q6_op_2_loopLeave_dedup_in, true, VertexBatch>;
    next_loop_t* _next_loop_hdl = nullptr;
    loop_sync_t* _loop_sync_hdl = nullptr;
};

