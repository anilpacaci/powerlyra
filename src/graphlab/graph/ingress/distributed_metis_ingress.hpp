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

#ifndef GRAPHLAB_DISTRIBUTED_METIS_INGRESS_HPP
#define GRAPHLAB_DISTRIBUTED_METIS_INGRESS_HPP

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
   * \brief Ingress object assigning edges using randoming hash function.
   */
  template<typename VertexData, typename EdgeData>
  class distributed_metis_ingress : 
    public distributed_ingress_base<VertexData, EdgeData> {
  public:
    typedef distributed_graph<VertexData, EdgeData> graph_type;
    /// The type of the vertex data stored in the graph 
    typedef VertexData vertex_data_type;
    /// The type of the edge data stored in the graph 
    typedef EdgeData   edge_data_type;


    typedef distributed_ingress_base<VertexData, EdgeData> base_type;
   
    typedef typename boost::unordered_map<vertex_id_type, procid_t> lookup_table_type;
    
    const size_t tot_nedges;
    const size_t tot_nverts;
    // full path to lookup file
    std::string metis_lookup_file;
    
    lookup_table_type lookup_table;
    
  public:
    distributed_metis_ingress(distributed_control& dc, graph_type& graph, std::string metis_lookup_file) :
    base_type(dc, graph), tot_nedges(tot_nedges), tot_nverts(tot_nverts),metis_lookup_file(metis_lookup_file) {
        // populate the map from lookup table. 
        // each loader process has a full copy of the lookup table
        std::ifstream in_file(metis_lookup_file.c_str(), std::ios_base::in);
        while(in_file.good() && !in_file.eof()) {
            std::string line;
            std::getline(in_file, line);
            if(line.empty()) continue;
            if(in_file.fail()) break;
            
            
        }
        
    } // end of constructor

    ~distributed_metis_ingress() { }

    /** Add a vertex to the ingress object using lookup table. */
        void add_vertex(vertex_id_type vid, std::vector<vertex_id_type>& adjacency_list,
                const VertexData& vdata) {
  }; // end of distributed_metis_ingress
}; // end of namespace graphlab
#include <graphlab/macros_undef.hpp>


#endif