#pragma once

#include "common/configs.hh"
#include "common/data_unit_type.hh"
#include "common/db_initializer.hh"
#include "common/query_helpers.hh"
#include "q2_op_0_V.act.h"

inline void benchmark_exec_query_2(int64_t start_vertex) {
    write_log(seastar::format(
            "[Exec query 2] query id = {}, start vertex = {}\n",
            q2_helper.current_query_id(), start_vertex));
    brane::scope_builder builder(locate_helper::locate(start_vertex),
                                 brane::scope<brane::actor_group<> >(q2_helper.current_query_id()));
    auto actor_ref_V = builder.build_ref<i_q2_op_0_V>(0);

    q2_helper.record_start_time();
    actor_ref_V.trigger(StartVertex{start_vertex});
};
