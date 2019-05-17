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

```

To enable append `ingress=dbh` to `--graph_opts`

2. HDFR (PowerGraph Implementation)

To enable append `ingress=hdrf` to `--graph_opts`

3. Random Edge-cut

Hash based edge-cut partitioning algorithm. A vertex and its outgoing edges assigned to a machine by hashing the vertex id.

To enable append `ingress=random_ec` to `--graph_opts`

4. LDG

To enable append `ingress=ldg` to `--graph_opts`

5. FENNEL

To enable append `ingress=fennel` to `--graph_opts`


6. Explicit Partitioning

To enable append `ingress=metis` to `--graph_opts`


#### Graph Readers

