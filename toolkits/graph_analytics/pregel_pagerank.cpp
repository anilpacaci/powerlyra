/*
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

#include <vector>
#include <string>
#include <fstream>

#include <graphlab.hpp>
// #include <graphlab/macros_def.hpp>

// Global random reset probability
float RESET_PROB = 0.15;

float TOLERANCE = 1.0E-2;

size_t ITERATIONS = 0;

bool USE_DELTA_CACHE = false;

// The vertex data is just the pagerank value (a float)
typedef float vertex_data_type;

// There is no edge data in the pagerank application
typedef float edge_data_type;

// The graph type is determined by the vertex and edge data types
typedef graphlab::distributed_graph<vertex_data_type, edge_data_type> graph_type;

typedef struct pagerank_message : public graphlab::IS_POD_TYPE {
    double value;
    double priority() const { return std::fabs(value); }
    pagerank_message& operator+=(const pagerank_message& other) {
      value += other.value; 
      return *this;
    }
} message_type;

/*
 * A simple function used by graph.transform_vertices(init_vertex);
 * to initialize the vertes data.
 */
void init_vertex(graph_type::vertex_type& vertex) { vertex.data() = 1; }



/*
 * The factorized page rank update function extends ivertex_program
 * specifying the:
 *
 *   1) graph_type
 *   2) gather_type: float (returned by the gather function). Note
 *      that the gather type is not strictly needed here since it is
 *      assumed to be the same as the vertex_data_type unless
 *      otherwise specified
 *
 * In addition ivertex program also takes a message type which is
 * assumed to be empty. Since we do not need messages no message type
 * is provided.
 *
 * pagerank also extends graphlab::IS_POD_TYPE (is plain old data type)
 * which tells graphlab that the pagerank program can be serialized
 * (converted to a byte stream) by directly reading its in memory
 * representation.  If a vertex program does not exted      std::cout << "Iteration: " << context.iteration() << std::endl;

 * graphlab::IS_POD_TYPE it must implement load and save functions.
 */
class pagerank :
public graphlab::ivertex_program<graph_type, graphlab::empty, pagerank_message>,
  public graphlab::IS_POD_TYPE {
  float last_change;
  double message_value;
public:
    
  void init(icontext_type& context, const vertex_type& vertex, 
            const message_type& msg) { 
    message_value = msg.value;
  } 
    
    /**
   * No gather because of vertex messages during scatter phase.
   */
  edge_dir_type gather_edges(icontext_type& context,
                              const vertex_type& vertex) const {
    return graphlab::NO_EDGES;
  }
    

  /* Use the total rank of adjacent pages to update this page */
  void apply(icontext_type& context, vertex_type& vertex,
             const gather_type& total) {
    const double newval = RESET_PROB + (1 - RESET_PROB) * message_value;
    last_change = std::fabs(newval - vertex.data());
    vertex.data() = newval;
  }

  /* The scatter edges depend on whether the pagerank has converged */
  edge_dir_type scatter_edges(icontext_type& context,
                              const vertex_type& vertex) const {
    // if (last_change > TOLERANCE || context.iteration() == 0) return graphlab::OUT_EDGES;
    // else return graphlab::NO_EDGES;
      return graphlab::OUT_EDGES;
  }

  /* The scatter function just signal adjacent pages */
  void scatter(icontext_type& context, const vertex_type& vertex,
               edge_type& edge) const {
    pagerank_message msg;
    // update edge data with the vertex value then activate the neighbour verte
    msg.value = vertex.data() / vertex.num_out_edges();
    context.signal(edge.target(), msg);
  }
}; // end of factorized_pagerank update functor


/*
 * We want to save the final graph so we define a write which will be
 * used in graph.save("path/prefix", pagerank_writer()) to save the graph.
 */
struct pagerank_writer {
  std::string save_vertex(graph_type::vertex_type v) {
    std::stringstream strm;
    strm << v.id() << "\t" << v.data() << "\n";
    return strm.str();
  }
  std::string save_edge(graph_type::edge_type e) { return ""; }
}; // end of pagerank writer

double map_rank(const graph_type::vertex_type& v) { return v.data(); }


double pagerank_sum(graph_type::vertex_type v) {
  return v.data();
}

int main(int argc, char** argv) {
  // Initialize control plain using mpi
  graphlab::mpi_tools::init(argc, argv);
  graphlab::distributed_control dc;
  global_logger().set_log_level(LOG_INFO);

  // Parse command line options -----------------------------------------------
  graphlab::command_line_options clopts("PageRank algorithm.");
  std::string graph_dir;
  std::string format = "adj";
  std::string exec_type = "synchronous";
  clopts.attach_option("graph", graph_dir,
                       "The graph file.  If none is provided "
                       "then a toy graph will be created");
  clopts.add_positional("graph");
  clopts.attach_option("engine", exec_type,
                       "The engine type synchronous or asynchronous");
  clopts.attach_option("tol", TOLERANCE,
                       "The permissible change at convergence.");
  clopts.attach_option("format", format,
                       "The graph file format");
  size_t powerlaw = 0;
  clopts.attach_option("powerlaw", powerlaw,
                       "Generate a synthetic powerlaw out-degree graph. ");
  clopts.attach_option("iterations", ITERATIONS,
                       "If set, will force the use of the synchronous engine"
                       "overriding any engine option set by the --engine parameter. "
                       "Runs complete (non-dynamic) PageRank for a fixed "
                       "number of iterations. Also overrides the iterations "
                       "option in the engine");
  clopts.attach_option("use_delta", USE_DELTA_CACHE,
                       "Use the delta cache to reduce time in gather.");
  std::string saveprefix;
  clopts.attach_option("saveprefix", saveprefix,
                       "If set, will save the resultant pagerank to a "
                       "sequence of files with prefix saveprefix");

  if(!clopts.parse(argc, argv)) {
    dc.cout() << "Error in parsing command line arguments." << std::endl;
    return EXIT_FAILURE;
  }


  // Enable gather caching in the engine
  clopts.get_engine_args().set_option("use_cache", USE_DELTA_CACHE);

  if (ITERATIONS) {
    // make sure this is the synchronous engine
    dc.cout() << "--iterations set. Forcing Synchronous engine, and running "
              << "for " << ITERATIONS << " iterations." << std::endl;
    //clopts.get_engine_args().set_option("type", "synchronous");
    clopts.get_engine_args().set_option("max_iterations", ITERATIONS);
    clopts.get_engine_args().set_option("sched_allv", true);
  }

  // Build the graph ----------------------------------------------------------
  dc.cout() << "Loading graph." << std::endl;
  graphlab::timer timer; 
  graph_type graph(dc, clopts);
  if(powerlaw > 0) { // make a synthetic graph
    dc.cout() << "Loading synthetic Powerlaw graph." << std::endl;
    graph.load_synthetic_powerlaw(powerlaw, false, 2.1, 100000000);
  }
  else if (graph_dir.length() > 0) { // Load the graph from a file
    dc.cout() << "Loading graph in format: "<< format << std::endl;
    graph.load_format(graph_dir, format);
  }
  else {
    dc.cout() << "graph or powerlaw option must be specified" << std::endl;
    clopts.print_description();
    return 0;
  }
  const double loading = timer.current_time();
  dc.cout() << "Loading graph. Finished in " 
            << loading << std::endl;


  // must call finalize before querying the graph
  dc.cout() << "Finalizing graph." << std::endl;
  timer.start();
  graph.finalize();
  const double finalizing = timer.current_time();
  dc.cout() << "Finalizing graph. Finished in " 
            << finalizing << std::endl;

  // NOTE: ingress time = loading time + finalizing time
  const double ingress = loading + finalizing;
  dc.cout() << "Final Ingress (second): " << ingress << std::endl;

  dc.cout() << "#vertices: " << graph.num_vertices()
            << " #edges:" << graph.num_edges() << std::endl;

  // Initialize the vertex data
  graph.transform_vertices(init_vertex);

  // Running The Engine -------------------------------------------------------
  graphlab::omni_engine<pagerank> engine(dc, graph, exec_type, clopts);
  engine.signal_all();
  timer.start();
  engine.start();
  const double runtime = timer.current_time();
  dc.cout() << "----------------------------------------------------------"
            << std::endl
            << "Final Runtime (seconds):   " << runtime 
            << std::endl
            << "Updates executed: " << engine.num_updates() << std::endl
            << "Update Rate (updates/second): " 
            << engine.num_updates() / runtime << std::endl;

  const double total_rank = graph.map_reduce_vertices<double>(map_rank);
  std::cout << "Total rank: " << total_rank << std::endl;

  // Save the final graph -----------------------------------------------------
  if (saveprefix != "") {
    graph.save(saveprefix, pagerank_writer(),
               false,    // do not gzip
               true,     // save vertices
               false, 1);   // do not save edges
  }

  // Tear-down communication layer and quit -----------------------------------
  graphlab::mpi_tools::finalize();
  return EXIT_SUCCESS;
} // End of main



