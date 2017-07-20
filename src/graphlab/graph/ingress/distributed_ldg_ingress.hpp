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
    typedef EdgeData   edge_data_type;
    
    typedef distributed_ingress_base<VertexData, EdgeData> base_type;
    
    typedef typename base_type::edge_buffer_record edge_buffer_record;
    typedef typename base_type::vertex_buffer_record vertex_buffer_record;
    
    typedef typename boost::unordered_map<vertex_id_type, procid_t> 
        placement_hash_table_type;

    placement_hash_table_type pht;
    
    std::vector<size_t> proc_balance;
    std::vector<size_t> proc_score_incr;
    
    size_t tot_nedges;
    size_t tot_nverts;
   
  public:
    distributed_ldg_ingress(distributed_control& dc, graph_type& graph, 
            size_t tot_nedges = 0, size_t tot_nverts = 0) :
      base_type(dc, graph), tot_nedges(tot_nedges), tot_nverts(tot_nverts) {
    } // end of constructor

    ~distributed_ldg_ingress() { }
    

    /** Add an edge to the ingress object using random assignment. */
    void add_vertex(vertex_id_type vid, std::vector<vertex_id_type>& adjacency_list,
                  const VertexData& vdata) {      
      const procid_t owning_proc = ldg_to_proc(vid, adjacency_list);
      pht[vid] = owning_proc;
      
      const vertex_buffer_record record(vid, vdata);      

      base_type::vertex_exchange.send(owning_proc, record, omp_get_thread_num());
      
      for(size_t i = 0 ; i < adjacency_list.size(); i++) {
          vertex_id_type target = adjacency_list[i];
          if(vid == target) { 
	      return;
	  }
	  const edge_buffer_record record(vid, target);
          base_type::edge_exchange.send(owning_proc, record, omp_get_thread_num() );
      }
      
    } // end of add vertex
    
    procid_t ldg_to_proc(vertex_id_type vid, std::vector<vertex_id_type>& adjacency_list) {
        
        
        return 0;
    }
    
  }; // end of distributed_ldg_ingress
}; // end of namespace graphlab
#include <graphlab/macros_undef.hpp>


#endif
