#!/usr/bin/python

import sys
import os
import re

if len(sys.argv) < 2:
	print "Provide input directory for log parsing"

log_directory	= sys.argv[1]
log_files		= []

# get all the log files from log directory
log_files = [os.path.join(log_directory,file) for file in os.listdir(log_directory) if os.path.isfile(os.path.join(log_directory, file))]

# start processing the files
for log_file in log_files:
	# variables that we try to extract from the log
	nparts			= 0
	ingress			= ""
	rf				= 0
	ingress_time	= 0
	finalize_time	= 0
	iterations		= 0
	compute_balance	= []
	gather_call		= 0 
	apply_call		= 0
	scatter_call	= 0
	execution_time	= 0
	breakdownx		= 0
	breakdownr		= 0
	breakdowng		= 0
	breakdowna		= 0
	breakdowns		= 0
	bytes_sent		= 0
	bytes_received	= 0
	calls_sent		= 0
	calls_received	= 0
	network_sent	= 0

	with open(log_file, 'r') as logs:
		for log in logs:
			# now we need to check for each occurance of parameters that we try to parse
			match = re.search("Cluster of (.*) instances", log)
			if match != None:
				nparts = int(match.group(1))
			# match for ingress method
			match = re.search("ingress = (.*)", log)
			if match != None:
				ingress = match.group(1)
			# replication factor
			match = re.search("replication factor: (.*)", log)
			if match != None:
				rf = float(match.group(1))
			# ingress time
			match = re.search("Final Ingress \(second\): (.*)", log)
			if match != None:
				ingress_time = float(match.group(1))
			# finalize time
			match = re.search("Finalizing graph. Finished in (.*)", log)
			if match != None:
				finalize_time = float(match.group(1))
			# iterations
			match = re.search(": (\w*) iterations completed", log)
			if match != None:
				iterations = int(match.group(1))
			# compute balance array
			match = re.search("Compute Balance: (.*)", log)
			if match != None:
				compute_balance = map(float, match.group(1).split())
			# gas calls 
			match = re.search(" Total Calls\(G\|A\|S\): (.*)", log)
			if match != None:
				[gather_call, apply_call, scatter_call] = map(float, match.group(1).split("|"))
			# execution time
			match = re.search("Execution Time: (.*)", log)
			if match != None:
				execution_time = float(match.group(1))
			# Breakdown of timing
			match = re.search("Breakdown\(X\|R\|G\|A\|S\): (.*)", log)
			if match != None:
				[breakdownx, breakdownr, breakdowng, breakdowna, breakdowns] = map(float, match.group(1).split("|"))
			# bytes sent
			match = re.search("Bytes Sent: (.*)", log)
			if match != None:
				bytes_sent += int(match.group(1))
			# calls sent
			match = re.search("Calls Sent: (.*)", log)
			if match != None:
				calls_sent += int(match.group(1))
			# bytes received
			match = re.search("Bytes Received: (.*)", log)
			if match != None:
				bytes_received += int(match.group(1))
			# calls received
			match = re.search("Calls Received: (.*)", log)
			if match != None:
				calls_received += int(match.group(1))
			# network sent
			match = re.search("Network Sent: (.*)", log)
			if match != None:
				network_sent += int(match.group(1))

	# now we can report everything about this file
	print("!!! {} on {} machines !!!".format(ingress, str(nparts)))
	print("RF: {} \t Ingress: {} \t Finalize: {}".format( str(rf), str(ingress_time), str(finalize_time) ))
	print("{} iterations in {} second".format(str(iterations), str(execution_time) ))
	print("Calls received:{} sent:{} \t Bytes received:{} sent:{} \t Network sent:{}".format( str(calls_received), str(calls_sent), str(bytes_received), str(bytes_sent), str(network_sent) ))

	print("!!! Done parsing {}".format(log_file))

