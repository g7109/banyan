//
// Created by everlighting on 2020/10/12.
//
#include "berkeley_db/storage/bdb_graph_handler.hh"
#include "q12_op_5_dedup_out.act.h"
#include "q12_op_6_unionOut.act.h"

void q12_op_5_dedup_out::process(VertexBatch &&input) {
    int64_t out_neighbour;
    for (unsigned i = 0; i < input.size(); i++) {
        if (_tags_have_seen.find(input[i]) == _tags_have_seen.end()) {
            _tags_have_seen.insert(input[i]);

            if (_tag_to_tagClass.count(input[i]) > 0) {
                _ds_group_hdl->emplace_data(locate_helper::locate(_tag_to_tagClass[input[i]]),
                                            _tag_to_tagClass[input[i]], input[i]);
            } else {
                auto iter = storage::bdb_graph_handler::_local_graph->get_out_v(input[i], _out_label);
                iter.next(out_neighbour);
                _ds_group_hdl->emplace_data(locate_helper::locate(out_neighbour), out_neighbour, input[i]);
                _tag_to_tagClass[input[i]] = out_neighbour;
            }
        }
    }
    _ds_group_hdl->flush();
}

void q12_op_5_dedup_out::receive_eos(Eos &&eos) {
    if (++_eos_rcv_num == _expect_eos_num) {
        _ds_group_hdl->receive_eos();
    }
}

void q12_op_5_dedup_out::initialize() {
    auto builder = get_scope();
    auto ds_id = *reinterpret_cast<uint32_t*>(_address + brane::GActorTypeInBytes) + 1;
    _ds_group_hdl = new ds_group_t(builder, ds_id);
}

void q12_op_5_dedup_out::finalize() {
    delete _ds_group_hdl;
}
