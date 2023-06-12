#include "cq5x_op_3_out.act.h"
#include "berkeley_db/storage/bdb_graph_handler.hh"

void cq5x_op_3_out::process(vertex_friendId_Batch&& input) {
    int64_t out_neighbour;
    for (unsigned i = 0; i < input.size(); i++) {
        auto iter = storage::bdb_graph_handler::_local_graph->get_out_v(input[i].v_id, _out_label);
        while (iter.next(out_neighbour)) {
            _ds_group_hdl->emplace_data(locate_helper::locate(out_neighbour), out_neighbour, input[i].friend_id);
        }
    }
    _ds_group_hdl->flush();
}

void cq5x_op_3_out::receive_eos(Eos &&eos) {
    if (++_eos_rcv_num == _expect_eos_num) {
        _ds_group_hdl->receive_eos();
    }
}

void cq5x_op_3_out::initialize() {
    auto builder = get_scope();
    auto ds_id = *reinterpret_cast<uint32_t*>(_address + brane::GActorTypeInBytes) + 1;
    _ds_group_hdl = new ds_group_t(builder, ds_id);
}

void cq5x_op_3_out::finalize() {
    delete _ds_group_hdl;
}
