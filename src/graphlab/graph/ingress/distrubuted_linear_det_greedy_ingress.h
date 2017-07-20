/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   distrubuted_linear_det_greedy_ingress.h
 * Author: vincent
 *
 * Created on July 19, 2017, 11:10 AM
 */

#ifndef DISTRUBUTED_LINEAR_DET_GREEDY_INGRESS_H
#define DISTRUBUTED_LINEAR_DET_GREEDY_INGRESS_H

#include <graphlab/rpc/buffered_exchange.hpp>
#include <graphlab/graph/graph_basic_types.hpp>
#include <graphlab/graph/ingress/distributed_ingress_base.hpp>
#include <graphlab/graph/distributed_graph.hpp>


#include <graphlab/macros_def.hpp>
namespace graphlab{
    template<typename VertexData, typename EdgeData>
    class distributed_graph;
    
    /**
     * \brief Ingress object assigning vertices by factoring shared neighbours
     * in each partition, versus the number of vertices already in the partition
     */
    template<typename VertexData, typename EdgeData>
    class distrubuted_linear_det_greedy_ingress :
        public distrubuted_ingress_base<VertexData, EdgeData> {
    public:
        typedef distributed_graph<VertexData, EdgeData> graph_type;
        /// The type of the vertex data stored in the graph 
        typedef VertexData vertex_data_type;
        /// The type of the edge data stored in the graph 
        typedef EdgeData   edge_data_type;

        typedef distributed_ingress_base<VertexData, EdgeData> base_type;

        typedef typename base_type::edge_buffer_record edge_buffer_record;
        typedef typename base_type::vertex_buffer_record vertex_buffer_record;
        
        typedef size_t Vertex;
        
        ///partitioning alg. variables
        size_t capacity_constraint;
        
        std::vector<std::map<vertex_id_type> > vertex_partition_map;
        
        
        //ctors
        distrubuted_linear_det_greedy_ingrees(distrubuted_control& dc, graph_type& graph,
                size_t capacity_constraint = 0):
        base_type(dc, graph), capacity_constraint(capacity_constraint) {
            if(base_type::rpc.numprocs() != 1) vertex_partition_map.resize(base_type::rpc.numprocs()-1);
        } //end of ctor
        
        ~distrubuted_linear_det_greedy_ingrees() {}
        
        
        /** Add an edge to the ingress object using random assignment. */
        void add_vertex(vertex_id_type vid, std::vector<vertex_id_type>& adjacency_list,
                      const VertexData& vdata) {      
            
            std::vector<float> partition_decision_vector(0, base_type::rpc.numprocs());
          
            
          for(size_t i = 0; i < vertex_partition_map.size(); i++)
          {
              //count number of vertices that are neighbours with vid and in partition i
              for(size_t adj_ind = 0; adj_ind < adjacenecy_list.size(); adj_ind++ )
              {
                  if(vertex_partition_map.at(i).find(v) != vertex_partition_map.at(i).end())
                  partition_decision_vector.at(i) += 1;
              }
                
              //multiply by weighted penalty
              float penalty;
              if(capacity_constraint == 0) penalty = 1;
              else { float penalty = 1 - (vertex_partition_map.at(i).size() / capacity_constraint); }
              partition_decision_vector.at(i) = partition_decision_vector.at(i) * penalty;

          }
            
          const procid_t owning_proc = std::max_element(partition_decision_vector.begin(), partition_decision_vector.end())
            - partition_decision_vector.begin();          
          
          vertex_partition_map.at(owning_proc).emplace(vid);

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
            
    }; //end of distrubuted_linear_det_greedy_ingress
} //end of graphlab namespace

#include <graphlab/macros_undef.hpp>

#endif /* DISTRUBUTED_LINEAR_DET_GREEDY_INGRESS_H */

