#!/usr/bin/python

import csv
import os
import shlex

snap_dataset	= "/home/apacaci/datasets/twitter_rv/twitter_rv.net"
adj_dataset		= "/home/apacaci/datasets/twitter_rv_adj_ec_combined.txt"

#snap_dataset	= "/home/apacaci/datasets/USA-road/part-00000"
#adj_dataset	= "/home/apacaci/datasets/USA-road-adjacency/part-00000"

result_folder	= "/home/apacaci/experiments/powerlyra/results/twitter"
log_folder		= "/home/apacaci/experiments/powerlyra/logs/twitter"


# csv file should have following headers
# nodes 		: total number of powerlyra process to be created
# pernode 		: # of process per physical machine
# nedges 		: # of edges in the input graph
# nverts 		: # of vertices in the input graph
# algorithm 	: graph analytics algorithm to be run on the 
# format 		: graph format, snap for vertex cut and adj_ec for edge cut
# ingress 		: partitioning strategy
# engine 		: plsync for vertex cut and plsyncec for edgecut
# iterations 	: number of supersteps, set to 0 to wait for convergence
parameters 		= "param-twitter.csv"

# an object holding parameters for experiment and return the command line string to be executed
class PowerLyraRun:
	name			= ""
	machines		= 16
	cpu_per_node	= 1
	graph_nodes		= 1
	graph_edges		= 1
	algorithm		= "pagerank"
	graph_format	= "snap"
	ingress			= "random"
	iterations		= 0
	engine			= "plsync"
	result_file		= ""
	log_file		= ""

	def __init__(self, machines, cpu_per_node, graph_nodes, graph_edges, algorithm, graph_format, ingress, iterations, engine):
		self.machines = machines
		self.cpu_per_node = cpu_per_node
		self.graph_nodes = graph_nodes
		self.graph_edges = graph_edges
		self.algorithm = algorithm
		self.graph_format = graph_format
		self.ingress = ingress
		self.iterations = iterations
		self.engine = engine
		# generate name from parameters
		self.name = algorithm + "-" + str(machines) + "-" + engine + "-" + ingress
		self.result_file = os.path.join(result_folder, self.name)
		self.log_file = os.path.join(log_folder, self.name)

	def produceCommandString(self):
		command = " mpiexec --mca btl_tcp_if_include enp2s0f0 "
		command += "-n {} -npernode {} ".format(str(self.machines), str(self.cpu_per_node))
		command += "-hostfile ~/machines "
		command += "/hdd1/gp/tools/powerlyra/release/toolkits/graph_analytics/{} ".format(self.algorithm)
		command += "--ncpus {} ".format(str(self.cpu_per_node))
		command += "--graph_opts ingress={},nedges={},nverts={} ".format(self.ingress, str(self.graph_edges), str(self.graph_nodes))
		command += "--format {} ".format(self.graph_format)
		# provide proper graph based on the format
		if self.graph_format == "snap":
			command += "--graph {} ".format(snap_dataset)
		else:
			command += "--graph {} ".format(adj_dataset)

		# set iterations only if its provided, not equal to zero
		if self.iterations > 0:
			command += "--iterations {} ".format(str(self.iterations))

		command += "--engine {} ".format(self.engine)
		# finally record the output
		command += "--saveprefix {} ".format(self.result_file)
		command += "2>&1 | tee {} ".format(self.log_file)
		return command
		
# list to hold all the objects for this set of experiments
run_list = []

# parse csv files and populate PowerLyraRun objects
with open(parameters, 'rb') as parameters_file:
	parameters_csv = csv.DictReader(parameters_file)
	for row in parameters_csv:
		run_list.append(PowerLyraRun(int(row['nodes']), int(row['pernode']), int(row['nverts']), int(row['nedges']), row['algorithm'], row['format'], row['ingress'], int(row['iterations']), row['engine']))

	# run each command one by one
	for run in run_list:
		print "------------"
		print "!!! Executing: {}".format(run.produceCommandString())
		result = os.system(run.produceCommandString())	
		print "!!! Shell returns: {}".format(result)
		print "------------"

	print "All runs are executed"


