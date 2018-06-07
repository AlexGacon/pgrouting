/*PGR-GNU*****************************************************************
File: bellman_ford.c

Generated with Template by:
Copyright (c) 2015 pgRouting developers
Mail: project@pgrouting.org

Function's developer:
Copyright (c) 2018 Sourabh Garg
Mail: sourabh.garg.mat@gmail.com


------

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

********************************************************************PGR-GNU*/

/** @file bellman_ford.c
 * @brief Conecting code with postgres.
 *
 * This file is fully documented for understanding
 *  how the postgres connectinon works
 *
 * TODO Remove unnecessary comments before submiting the function.
 * some comments are in form of PGR_DBG message
 */

/**
 *  postgres_connection.h
 *
 *  - should allways be first in the C code
 */
#include "c_common/postgres_connection.h"
#include "utils/array.h"

/* for macro PGR_DBG */
#include "c_common/debug_macro.h"
/* for pgr_global_report */
#include "c_common/e_report.h"
/* for time_msg & clock */
#include "c_common/time_msg.h"
/* for functions to get edges informtion */
#include "c_common/edges_input.h"
#include "c_common/arrays_input.h"
#include "drivers/bellman_ford/bellman_ford_driver.h"  // the link to the C++ code of the function

PGDLLEXPORT Datum bellman_ford(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(bellman_ford);


/******************************************************************************/
/*                          MODIFY AS NEEDED                                  */
static
void
process(
        char* edges_sql,
        ArrayType *starts,
        ArrayType *ends,
        bool directed,
        bool only_cost,

        General_path_element_t **result_tuples,
        size_t *result_count) {
    /*
     *  https://www.postgresql.org/docs/current/static/spi-spi-connect.html
     */
    pgr_SPI_connect();

        
    PGR_DBG("Initializing arrays");
    int64_t* start_vidsArr = NULL;
    size_t size_start_vidsArr = 0;
    start_vidsArr = (int64_t*)
        pgr_get_bigIntArray(&size_start_vidsArr, starts);
    PGR_DBG("start_vidsArr size %ld ", size_start_vidsArr);

    int64_t* end_vidsArr = NULL;
    size_t size_end_vidsArr = 0;
    end_vidsArr = (int64_t*)
        pgr_get_bigIntArray(&size_end_vidsArr, ends);
    PGR_DBG("end_vidsArr size %ld ", size_end_vidsArr);

    // start_vid, end_vid : Pass to driver file
    int64_t start_vid = start_vidsArr[0];
    int64_t end_vid = end_vidsArr[0];

    (*result_tuples) = NULL;
    (*result_count) = 0;

    PGR_DBG("Load data");
    pgr_edge_t *edges = NULL;
    size_t total_edges = 0;

    if (total_edges == 0) {
        if (end_vidsArr) pfree(end_vidsArr);
        if (start_vidsArr) pfree(start_vidsArr);
        pgr_SPI_finish();
        return;
    }

    pgr_get_edges(edges_sql, &edges, &total_edges);
    PGR_DBG("Total %ld edges in query:", total_edges);

    if (total_edges == 0) {
        PGR_DBG("No edges found");
        pgr_SPI_finish();
        return;
    }

    PGR_DBG("Starting processing");
    clock_t start_t = clock();
    char *log_msg = NULL;
    char *notice_msg = NULL;
    char *err_msg = NULL;
    do_pgr_bellman_ford(
            edges,
            total_edges,
            start_vid,
            end_vid,
#if 0
    /*
     *  handling arrays example
     */

            start_vidsArr, size_start_vidsArr,
            end_vidsArr, size_end_vidsArr,
#endif

            directed,
            only_cost,
            result_tuples,
            result_count,
            &log_msg,
            &notice_msg,
            &err_msg);

    time_msg(" processing pgr_bellman_ford", start_t, clock());
    PGR_DBG("Returning %ld tuples", *result_count);

    if (err_msg) {
        if (*result_tuples) pfree(*result_tuples);
    }
    pgr_global_report(log_msg, notice_msg, err_msg);

    if (edges) pfree(edges);
    if (log_msg) pfree(log_msg);
    if (notice_msg) pfree(notice_msg);
    if (err_msg) pfree(err_msg);


    if (end_vidsArr) pfree(end_vidsArr);
    if (start_vidsArr) pfree(start_vidsArr);


    pgr_SPI_finish();
}
/*                                                                            */
/******************************************************************************/

PGDLLEXPORT Datum bellman_ford(PG_FUNCTION_ARGS) {
    FuncCallContext     *funcctx;
    TupleDesc           tuple_desc;

    /**************************************************************************/
    /*                          MODIFY AS NEEDED                              */
    /*                                                                        */
    General_path_element_t  *result_tuples = NULL;
    size_t result_count = 0;
    /*                                                                        */
    /**************************************************************************/

    if (SRF_IS_FIRSTCALL()) {
        MemoryContext   oldcontext;
        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);


        /**********************************************************************/
        /*                          MODIFY AS NEEDED                          */
        /*
           TEXT,
    BIGINT,
    BIGINT,
    directed BOOLEAN DEFAULT true,
    only_cost BOOLEAN DEFAULT false,
         **********************************************************************/


        PGR_DBG("Calling process");
        process(
                text_to_cstring(PG_GETARG_TEXT_P(0)),
                PG_GETARG_ARRAYTYPE_P(1),
                PG_GETARG_ARRAYTYPE_P(2),
                PG_GETARG_BOOL(3),
                PG_GETARG_BOOL(4),
                &result_tuples,
                &result_count);


        /*                                                                    */
        /**********************************************************************/

#if PGSQL_VERSION > 94
        funcctx->max_calls = result_count;
#else
        funcctx->max_calls = (uint32_t)result_count;
#endif
        funcctx->user_fctx = result_tuples;
        if (get_call_result_type(fcinfo, NULL, &tuple_desc)
                != TYPEFUNC_COMPOSITE) {
            ereport(ERROR,
                    (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                     errmsg("function returning record called in context "
                         "that cannot accept type record")));
        }

        funcctx->tuple_desc = tuple_desc;
        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();
    tuple_desc = funcctx->tuple_desc;
    result_tuples = (General_path_element_t*) funcctx->user_fctx;

    if (funcctx->call_cntr < funcctx->max_calls) {
        HeapTuple    tuple;
        Datum        result;
        Datum        *values;
        bool*        nulls;

        /**********************************************************************/
        /*                          MODIFY AS NEEDED                          */
        /*
               OUT seq INTEGER,
    OUT path_seq INTEGER,
    OUT node BIGINT,
    OUT edge BIGINT,
    OUT cost FLOAT,
    OUT agg_cost FLOAT
         ***********************************************************************/

        values = palloc(6 * sizeof(Datum));
        nulls = palloc(6 * sizeof(bool));


        size_t i;
        for (i = 0; i < 6; ++i) {
            nulls[i] = false;
        }

        // postgres starts counting from 1
        values[0] = Int32GetDatum(funcctx->call_cntr + 1);
        values[1] = Int32GetDatum(result_tuples[funcctx->call_cntr].seq);
        values[2] = Int64GetDatum(result_tuples[funcctx->call_cntr].node);
        values[3] = Int64GetDatum(result_tuples[funcctx->call_cntr].edge);
        values[4] = Float8GetDatum(result_tuples[funcctx->call_cntr].cost);
        values[5] = Float8GetDatum(result_tuples[funcctx->call_cntr].agg_cost);
        /**********************************************************************/

        tuple = heap_form_tuple(tuple_desc, values, nulls);
        result = HeapTupleGetDatum(tuple);
        SRF_RETURN_NEXT(funcctx, result);
    } else {
        /**********************************************************************/
        /*                          MODIFY AS NEEDED                          */

        PGR_DBG("Clean up code");

        /**********************************************************************/

        SRF_RETURN_DONE(funcctx);
    }
}
