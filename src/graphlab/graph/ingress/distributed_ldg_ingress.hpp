/**  
 * Copyright (c) 2009 Carnegie Mellon University. 
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 * For more about this software visit:
 *
 *      http://www.graphlab.ml.cmu.edu
 *
 */

#ifndef GRAPHLAB_DISTRIBUTED_LDG_INGRESS_HPP
#define GRAPHLAB_DISTRIBUTED_LDG_INGRESS_HPP

#include <boost/functional/hash.hpp>

#include <graphlab/rpc/buffered_exchange.hpp>
#include <graphlab/graph/graph_basic_types.hpp>
#include <graphlab/graph/ingress/distributed_ingress_base.hpp>
#include <graphlab/graph/distributed_graph.hpp>

#include <graphlab/macros_def.hpp>

namespace graphlab {
    template<typename VertexData, typename EdgeData>
    class distributed_graph;

    size_t PLACEMENT_BUFFER_THRESHOLD = 5000;
    
    /**
     * \brief Ingress object assigning vertices using LDG heurisic.
     */
    template<typename VertexData, typename EdgeData>
    class distributed_ldg_ingress :
    public distributed_ingress_base<VertexData, EdgeData> {
    public:
        typedef distributed_graph<VertexData, EdgeData> graph_type;
        /// The type of the vertex data stored in the graph 
        typedef VertexData vertex_data_type;
        /// The type of the edge data stored in the graph 
        typedef EdgeData edge_data_type;

        typedef distributed_ingress_base<VertexData, EdgeData> base_type;

        typedef typename base_type::edge_buffer_record edge_buffer_record;
        typedef typename base_type::vertex_buffer_record vertex_buffer_record;

        typedef typename boost::unordered_map<vertex_id_type, procid_t>
        placement_hash_table_type;
        typedef typename std::pair<vertex_id_type, procid_t>
        placement_pair_type;

        placement_hash_table_type dht_placement_table;
        std::vector<placement_pair_type> placement_buffer;
        rwlock dht_placement_table_lock;
        
        
        std::vector<size_t> partition_edge_capacity;
        std::vector<size_t> partition_vertex_capacity;

        const size_t tot_nedges;
        const size_t tot_nverts;
        const size_t nprocs;
        procid_t self_pid;
        size_t edge_capacity_constraint;
        size_t vertex_capacity_constraint;

        dc_dist_object<distributed_ldg_ingress> ldg_rpc;
        

    public:

        distributed_ldg_ingress(distributed_control& dc, graph_type& graph,
                size_t tot_nedges = 0, size_t tot_nverts = 0) :
                base_type(dc, graph), ldg_rpc(dc, this), nprocs(base_type::rpc.numprocs()), tot_nedges(tot_nedges), tot_nverts(tot_nverts), partition_edge_capacity(base_type::rpc.numprocs(), 0),
                partition_vertex_capacity(base_type::rpc.numprocs(), 0) {
            
            self_pid = base_type::rpc.procid();

            double balance_slack = 0.05;
            edge_capacity_constraint = (tot_nedges / nprocs) * (1 + balance_slack);
            vertex_capacity_constraint = (tot_nverts / nprocs) * (1 + balance_slack);
        } // end of constructor

        ~distributed_ldg_ingress() {
        }

        /** Add an edge to the ingress object using random assignment. */
        void add_vertex(vertex_id_type vid, std::vector<vertex_id_type>& adjacency_list,
                const VertexData& vdata) {
            // initialize all partition scores with 0
            std::vector<float> partition_scores(nprocs, 0);
            std::vector<float> neighbour_count(nprocs, 0);

            std::cout << "### Process ID" << self_pid << "    Vertex Id" << vid << std::endl;
            
            // query partition id of each neighbour and count neighbours in each partition
            for (size_t i = 0; i < adjacency_list.size(); i++) {
                procid_t neighbour_owner = get_vertex_partition(adjacency_list[i]);
                std::cout << "Neighbourhood : " << adjacency_list[i] << "   partition:" << neighbour_owner << std::endl;
                if (neighbour_owner !=  ((size_t)-1)) {
                    neighbour_count[neighbour_owner]++;
                }
            }

            for (size_t i = 0; i < nprocs; i++) {
                // get current capacity for partition i
                size_t current_partition_capacity = partition_vertex_capacity[i];

                // compute partition i score
                partition_scores[i] = neighbour_count[i] * (1 - (current_partition_capacity / vertex_capacity_constraint));
                
                std::cout << "Partition:" << i << "     Score:" << partition_scores[i] << std::endl;
            }

            float best_score = partition_scores[0];
            procid_t best_proc = 0;
            for (size_t i = 1; i < nprocs; ++i) {
                if (partition_scores[i] > best_score) {
                    best_score = partition_scores[i];
                    best_proc = i;
                }
            }


            const procid_t owning_proc = best_proc;
            set_vertex_partition(vid, owning_proc);

            const vertex_buffer_record record(vid, vdata);

            base_type::vertex_exchange.send(owning_proc, record, omp_get_thread_num());

            for (size_t i = 0; i < adjacency_list.size(); i++) {
                vertex_id_type target = adjacency_list[i];
                if (vid == target) {
                    return;
                }
                const edge_buffer_record record(vid, target);
                base_type::edge_exchange.send(owning_proc, record, omp_get_thread_num());
            }

        } // end of add vertex
        
    private:
            /**
         * Acquires read lock on the distributed table and returns partition procid for given vertex
         * @param vid
         * @return -1 if entry does not exist
         */
        procid_t get_vertex_partition(vertex_id_type vid) {
            procid_t partition;
            dht_placement_table_lock.readlock();
            if (dht_placement_table.find(vid) == dht_placement_table.end()) {
                partition = -1;
            } else {
                partition = dht_placement_table[vid];
            }
            dht_placement_table_lock.rdunlock();
            return partition;
        }

        /**
         * Acquires write lock on the table and populated the partition entry for given vertex
         * @param vid
         * @param procid
         */
        void set_vertex_partition(vertex_id_type vid, procid_t procid) {
            dht_placement_table_lock.writelock();
            dht_placement_table[vid] = procid;
            placement_buffer.push_back(placement_pair_type(vid, procid));
            // increase local partition capacity
            partition_vertex_capacity[procid]++;
            dht_placement_table_lock.wrunlock();
            
            std::cout << "Vertex:" << vid << "  Partition:" << procid << std::endl;
            
            // check whether we need to sync blocks
            if(placement_buffer.size() > PLACEMENT_BUFFER_THRESHOLD) {
                for(size_t i = 0 ; i < nprocs ; i++) {
                    // only populate the the ones that do not belong to this process
                    if(i != self_pid) {
                        // need remote call to populate dht
                        ldg_rpc.remote_call(i, &distributed_ldg_ingress::block_add_placement_pair, self_pid, placement_buffer);
                    } 
                }
                placement_buffer.clear();
            }
        }
        
        void block_add_placement_pair(procid_t pid, std::vector<placement_pair_type>& placement_buffer) {
            dht_placement_table_lock.writelock();
            
            foreach( placement_pair_type& placement, placement_buffer ) {
                dht_placement_table[placement.first] = placement.second;
                // update partition capacity
                partition_vertex_capacity[placement.second]++;
            }
            
            dht_placement_table_lock.wrunlock();
        }

    }; // end of distributed_ldg_ingress
}; // end of namespace graphlab
#include <graphlab/macros_undef.hpp>


#endif
