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

#include <graphlab/logger/logger.hpp>
#include <graphlab/logger/assertions.hpp>

#include <graphlab/rpc/buffered_exchange.hpp>
#include <graphlab/graph/graph_basic_types.hpp>
#include <graphlab/graph/ingress/distributed_ingress_base.hpp>
#include <graphlab/graph/distributed_graph.hpp>

#include <graphlab/macros_def.hpp>

#include <string>
#include <sstream>

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
    
    dc_dist_object<distributed_fennel_ingress> metis_rpc;

    
    const size_t tot_nedges;
    const size_t tot_nverts;
    // full path to lookup file
    std::string metis_lookup_file;
    
    lookup_table_type lookup_table;
    
  public:
    distributed_metis_ingress(distributed_control& dc, graph_type& graph, std::string metis_lookup_file) :
    base_type(dc, graph), metis_rpc(dc, this) tot_nedges(tot_nedges), tot_nverts(tot_nverts),metis_lookup_file(metis_lookup_file) {
        // populate the map from lookup table. 
        // each loader process has a full copy of the lookup table
        std::ifstream in_file(metis_lookup_file.c_str(), std::ios_base::in);
        while(in_file.good() && !in_file.eof()) {
            std::string line;
            std::getline(in_file, line);
            if(line.empty()) continue;
            if(in_file.fail()) break;
            
            std::stringstream ls(line);
            
            vertex_id_type vid << line;
            procid_t owning_proc << line;
            
            lookup_table[vid] = owning_proc;
        }
        
        logstream(LOG_INFO) << "Lookup table populated using: " << metis_lookup_file << std::endl;
        
    } // end of constructor

    ~distributed_metis_ingress() { }

    /** Add a vertex to the ingress object using lookup table. */
    void add_vertex(vertex_id_type vid, std::vector<vertex_id_type>& adjacency_list,
            const VertexData& vdata) {

        procid_t owning_proc;
        if (dht_placement_table.find(vid) == dht_placement_table.end()) {
            owning_proc = 0;
            logstream(LOG_WARNING) << "Lookup entry cannot be found for vertex: " << vid << std::endl;
        } else {
            owning_proc = dht_placement_table[vid];
        }

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
    }
    
    protected:
        virtual void determine_master(vid2lvid_map_type& vid2lvid_buffer) {

        /**************************************************************************/
      /*                                                                        */
      /*        assign vertex data and allocate vertex (meta)data  space        */
      /*                                                                        */
      /**************************************************************************/
            std::cout << "METIS DETERMINE MASTER" << std::endl;
       // Determine masters for all negotiated vertices
        const size_t local_nverts = base_type::graph.vid2lvid.size() + vid2lvid_buffer.size();
        base_type::graph.lvid2record.reserve(local_nverts);
        base_type::graph.lvid2record.resize(local_nverts);
        base_type::graph.local_graph.resize(local_nverts);
        foreach(const vid2lvid_pair_type& pair, vid2lvid_buffer) {
            vertex_record& vrec = base_type::graph.lvid2record[pair.second];
            vrec.gvid = pair.first;
            vrec.owner = dht_placement_table[pair.first];
        }
        ASSERT_EQ(local_nverts, base_type::graph.local_graph.num_vertices());
        ASSERT_EQ(base_type::graph.lvid2record.size(), base_type::graph.local_graph.num_vertices());
        if(metis_rpc.procid() == 0)       
          memory_info::log_usage("Finihsed allocating lvid2record");
      }
  }; // end of distributed_metis_ingress
}; // end of namespace graphlab
#include <graphlab/macros_undef.hpp>


#endif