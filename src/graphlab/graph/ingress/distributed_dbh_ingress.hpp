/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   distrubuted_dbh_ingress.hpp
 * Author: Vincent
 *
 * Created on July 19, 2017, 5:56 PM
 */

#ifndef DISTRIBUTED_DBH_INGRESS_HPP
#define DISTRIBUTED_DBH_INGRESS_HPP

#include <boost/functional/hash.hpp>

#include <graphlab/rpc/buffered_exchange.hpp>
#include <graphlab/graph/graph_basic_types.hpp>
#include <graphlab/graph/ingress/distributed_ingress_base.hpp>
#include <graphlab/graph/distributed_graph.hpp>
#include <graphlab/logger/logger.hpp>
#include <vector>
#include <graphlab/graph/graph_hash.hpp>
#include <graphlab/rpc/distributed_event_log.hpp>

#include <graphlab/macros_def.hpp>
namespace graphlab{
  template<typename VertexData, typename EdgeData>
  class distributed_graph;

  /**
   * \brief Ingress object assigning edges using random hash function on
   * vertex with higher degree between source and target
   */
  template<typename VertexData, typename EdgeData>
  class distributed_dbh_ingress : 
    public distributed_ingress_base<VertexData, EdgeData> {
  public:
    typedef distributed_graph<VertexData, EdgeData> graph_type;
    /// The type of the vertex data stored in the graph 
    typedef VertexData vertex_data_type;
    /// The type of the edge data stored in the graph 
    typedef EdgeData   edge_data_type;
    

    typedef distributed_ingress_base<VertexData, EdgeData> base_type;
    
    typedef typename base_type::edge_buffer_record edge_buffer_record;
    typedef typename buffered_exchange<edge_buffer_record>::buffer_type 
        edge_buffer_type;
        
    ///Variables for partition alg
    
    //store the degrees of each vertex        
    std::vector<size_t> degree_vector;
    buffered_exchange<vertex_id_type> degree_exchange;
    
    //1 edge exchange for first pass, when reading the edges to examine degree
    //the first edge exchange arbitrarily decides proc to send
    //second edge exchange used to add edge to correct proc owner,
    //that is hashing the higher degree vertex between source and target
    buffered_exchange<edge_buffer_record> edge_exchange;
    buffered_exchange<edge_buffer_record> dbh_edge_exchange;
    
    size_t nverts;   
    
    //rpc for this class, allows communication between objects through various machines
    dc_dist_object<distributed_dbh_ingress> dbh_rpc;
    
   
  public:
    distributed_dbh_ingress(distributed_control& dc, graph_type& graph, size_t nverts = 0) :
         base_type(dc, graph), 
         degree_exchange(dc), edge_exchange(dc),
         dbh_edge_exchange(dc), dbh_rpc(dc, this), nverts(nverts) {
             degree_vector.resize(nverts);
             
             dbh_rpc.barrier();             
    } // end of constructor

    ~distributed_dbh_ingress() { }

    /** Add an edge to the ingress object using random assignment.
     * This is the first pass for format SNAP, used only to count the degree of each vertex.
     * The procid for the edge is arbitrary in this function, so hashing the source will be used.
     *  */
    void add_edge(vertex_id_type source, vertex_id_type target,
                  const EdgeData& edata) {
      typedef typename base_type::edge_buffer_record edge_buffer_record;
      
      procid_t procid;
      
      //increase degree of source
      //degrees are stored/accumulated on each machine locally, then sent out in finalize
      if(nverts != 0) ++degree_vector.at(source);
      else 
      {
          if (source >= degree_vector.size) degree_vector.resize(source+1);
          ++degree_vector.at(source);
      }
      
      procid = graph_hash::hash_vertex(source) % base_type::dbh_rpc.numprocs();
      const procid_t owning_proc = procid;
      const edge_buffer_record record(source, target, edata);
      base_type::edge_exchange.send(owning_proc, record);
    } // end of add edge
    
    /** Add an edge to the ingress object by hashing the source or target vid
     * chosen by taking the vertex with higher degree, ties broken by taking source
     *  */
    void assign_edges() {
        graphlab::timer ti;
        size_t nprocs = dbh_rpc.numprocs();
        procid_t l_procid = dbh_rpc.procid();
        
        edge_buffer_type edge_buffer;
        procid_t proc = -1;
        while(edge_exchange.recv(proc, edge_buffer)) {
            for(auto it = edge_buffer.begin(); it != edge_buffer.end(); ++it){
                procid_t procid;
                if(degree_vector.at(it->source) > degree_vector.at(it->target))
                    procid = graph_hash::hash_vertex(it->source) % base_type::degree_rpc.numprocs();
                else procid = graph_hash::hash_vertex(it->target) % base_type::degree_rpc.numprocs();
                
                const procid_t owning_proc = procid;
                const edge_buffer_record record(it->source, it->target, it->edata);
                base_type::degree_edge_exchange.send(owning_proc, record);
            }                                       
        }
        edge_exchange.clear();
    } //end of assign_edges
    
    //finalize will need to combine the degree exchanges
    void finalize() {
      graphlab::timer ti;

      size_t nprocs = dbh_rpc.numprocs();
      procid_t l_procid = dbh_rpc.procid();
      size_t nedges = 0;

      dbh_rpc.full_barrier();

      if(l_procid == 0) {
        memory_info::log_usage("start finalizing");
        logstream(LOG_EMPH) << "DBH finalizing ..."
                            << " #vertices=" << graph.local_graph.num_vertices()
                            << " #edges=" << graph.local_graph.num_edges()
                            << std::endl;
      }
      /**************************************************************************/
      /*                                                                        */
      /*                       Flush any additional data                        */
      /*                                                                        */
      /**************************************************************************/
      
      edge_exchange.flush();
      dbh_edge_exchange.flush();
      
      /**
       * Fast pass for redundant finalization with no graph changes. 
       */
      {
        size_t changed_size = edge_exchange.size() + dbh_edge_exchange.size();
        dbh_rpc.all_reduce(changed_size);
        if (changed_size == 0) {
          logstream(LOG_INFO) << "Skipping Graph Finalization because no changes happened..." << std::endl;
          return;
        }
      }
      
      /**************************************************************************/
      /*                                                                        */
      /*                       Manage all degree values                        */
      /*                                                                        */
      /**************************************************************************/
      if(nprocs != 1) { 
            if(l_procid == 0) logstream(LOG_INFO) << "Collecting Degree Counts" <<std::endl;
          
            //send degree_vector values to main machine
            if(l_procid != 0) {
                //send the degree_vector values into the exchange so main computer can handle
                for(auto it = degree_vector.begin(); it != degree_vector.end(); ++it){
                    degree_exchange.send(*it, 0);
                }                      
            }
            dbh_rpc.full_barrier();
          
            //sum degree values from other machines
            if(l_procid == 0) {
                for(procid_t procid = 1; procid < nprocs; procid++) {
                    //for each procid, get the contents of its degree exchange
                    std::vector<size_t> degree_accum;
                    if(degree_exchange.recv(procid, degree_accum)) {
                        auto degree_v_it = degree_vector.begin();
                        auto degree_acc_it = degree_accum.begin();
                        while(degree_v_it != degree_vector.end() && degree_acc_it != degree_accum.end()) {
                            *degree_v_it = *degree_v_it + degree_acc_it;
                            ++degree_v_it;
                            ++degree_acc_it;
                        }
                    }
                }

                //send the final degree vector to the other machines
                for(auto it = degree_vector.begin(); it != degree_vector.end(); ++it) {
                    for(procid_t procid = 1; procid < nprocs; procid++) {
                        degree_exchange.send(procid, *it);
                    }
                }
            }
            dbh_rpc.barrier();
            
            //update degree vector for sub machines
            if(l_procid != 0) {
                std::vector<size_t> rec_degree_vector;
                degree_exchange.recv(0, rec_degree_vector);
                auto degree_v_it = degree_vector.begin();
                auto rec_degree_v_it = rec_degree_vector.begin();
                while(degree_v_it != degree_vector.end() && rec_degree_v_it != rec_degree_vector.end()) {
                    *degree_v_it = *rec_degree_v_it;
                }
            }
            dbh_rpc.barrier();  
      }
            
      /**************************************************************************/
      /*                                                                        */
      /*                       Manage all degree values                        */
      /*                                                                        */
      /**************************************************************************/
      if(nprocs != 1) {
          assign_edges();
      }
      if(l_procid == 0) {
        logstream(LOG_INFO) << "assign edges: " 
                            << ti.current_time()
                            << " secs" 
                            << std::endl;
      }
            
            
      

    }//end of finalize
  }; // end of distributed_dbh_ingress
}; // end of namespace graphlab
#include <graphlab/macros_undef.hpp>


#endif /* DISTRIBUTED_DBH_INGRESS_HPP */

