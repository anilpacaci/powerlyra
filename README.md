## PowerLyra

This is a fork of the original (https://github.com/realstolz/powerlyra)[PowerLyra] repository. Please refer to thet original (https://github.com/realstolz/powerlyra)[repository] for detailed instructions on usage and installation.

This modified version of PowerLyra has been used in an experimental analysis of streaming algorithm for graph partitioning. Please refer to our SIGMOD'19 paper for details:

```
Anil Pacaci and M. Tamer Özsu. 2019. Experimental Analysis of Streaming Algorithms for Graph Partitioning. 
In 2019 International Conference on Management of Data (SIGMOD ’19), 
June 30-July 5, 2019, Amsterdam, Netherlands. 
ACM, New York, NY, USA, 18 pages. https://doi.org/10.1145/3299869.3300076
```

## Changes

### Engines

1. Synchronus Edge-cut Engine `plsync_ec`

This engine simulates the edge-cut model as in Giraph by grouping all out-edges of a vertex together in one machine. Both the `apply` and `scatter` phases can be done without any synchronization since all out-edges required for the scatter phase are placed locally. 

To enable use `--engine plsync_ec` command line option.

#### Ingress (Partitioning Methods)

1. Degree-base Hashing

Degree based randomized vertex-cut streaming partitioning algorithm. For more details:

```
Cong Xie, Ling Yan, Wu-Jun Li, and Zhihua Zhang. 2014.
Distributed power-law graph computing: Theoretical and empirical analysis. 
In Advances in Neural Information Proc. Systems 27, Proc. 28th Annual Conf. on Neural Information Proc. Systems. 1673–1681.

```

To enable append `ingress=dbh` to `--graph_opts`

2. HDRF (PowerGraph Implementation)

High-Degree-Replicated-First (HDRF) is a greedy heuristic based vertex-cut streaming partitioning algorithm. It has been adopted from PowerGraph [implementation](https://github.com/jegonzal/PowerGraph/blob/master/src/graphlab/graph/ingress/distributed_hdrf_ingress.hpp)

To enable append `ingress=hdrf` to `--graph_opts`

3. Random Edge-cut

Hash based edge-cut partitioning algorithm. A vertex and its outgoing edges assigned to a machine by hashing the vertex id.

To enable append `ingress=random_ec` to `--graph_opts`

4. Linear Deterministic Greedy

Greedy edge-cut streaming partitioning algorithm based on:

```
Isabelle Stanton and Gabriel Kliot. 2012. Streaming Graph Partitioning for Large Distributed Graphs. 
In Proc. 18th ACM SIGKDD Int. Conf. on Knowledge Discovery and Data Mining. 1222–1230. 
https://doi.org/10. 1145/2339530.2339722
```

To enable append `ingress=ldg` to `--graph_opts`

5. FENNEL

Greedy edge-cut streaming partitioning algorithm based on:

```
Charalampos Tsourakakis, Christos Gkantsidis, Bozidar Radunovic, and Milan Vojnovic. 2014. 
FENNEL: Streaming Graph Partitioning for Massive Scale Graphs. 
In Proc. 7th ACM Int. Conf. Web Search and Data Mining. 333–342. https://doi.org/10.1145/2556195.2556213
```

To enable append `ingress=fennel` to `--graph_opts`


6. Explicit Partitioning

Partitioner that uses explicit mapping produced by another algorithm. Mappings are read from a file where each line is in the form of `vertex-id partition_number` where partitioning are numbered from `0` to `k-1`

To enable append `ingress=metis` to `--graph_opts` and provide lookup file `lookup=[absolute path to lookup file]` to `--graph_opts`


#### Graph Readers

1. Adjacency List Reader

Graph reader for adjacency list format where each line represents a vertex and its al outgoing edges:

```vertex_id neighbour_1_id ... neighbour_n_id```

To enable append `--format snap`
