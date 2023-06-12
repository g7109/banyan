//
// Created by everlighting on 2020/10/12.
//
#pragma once

#include <brane/util/common-utils.hh>
#include <brane/actor/reference_base.hh>
#include <brane/actor/actor_implementation.hh>
#include "common/downstream_handlers.hh"

class i_q12_op_6_unionOut;

class ANNOTATION(actor:reference) i_q12_op_5_dedup_out : public brane::reference_base {
public:
    void process(VertexBatch&& input);
    void receive_eos(Eos&& eos);

    // Constructor.
    ACTOR_ITFC_CTOR(i_q12_op_5_dedup_out);
    // Destructor
    ACTOR_ITFC_DTOR(i_q12_op_5_dedup_out);
};

class ANNOTATION(actor:implement) q12_op_5_dedup_out : public brane::stateful_actor {
public:
    void process(VertexBatch&& input);
    void receive_eos(Eos&& eos);

    void initialize() override;
    void finalize() override;

    // Constructor.
    ACTOR_IMPL_CTOR(q12_op_5_dedup_out);
    // Destructor
    ACTOR_IMPL_DTOR(q12_op_5_dedup_out);
    // Do work
    ACTOR_DO_WORK();

private:
    unsigned _eos_rcv_num = 0;
    const unsigned _expect_eos_num = brane::global_shard_count();

    using ds_group_t = downstream_group_handler<i_q12_op_6_unionOut, vertex_tagId_Batch>;
    ds_group_t* _ds_group_hdl = nullptr;

    std::unordered_set<int64_t> _tags_have_seen{};
    std::unordered_map<int64_t, int64_t> _tag_to_tagClass{};

    const std::string _out_label = "hasType";
};
