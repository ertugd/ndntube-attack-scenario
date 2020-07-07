#!/usr/bin/env python
# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

from subprocess import call
from sys import argv
import os
import subprocess
import workerpool
import multiprocessing
import argparse
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

#Copyright (c) 2019, Algorithmi Research Centree, University of Minho, Portugal


# * Authors: Ertugrul Dogruluk, Antonio Costa. <d7474@di.uminho.pt>, Department of #Informatics

######################################################################
######################################################################
###################################################                                 ###################

parser = argparse.ArgumentParser(description='Simulation runner')
parser.add_argument('scenarios', metavar='scenario', type=str, nargs='*',
                    help='Scenario to run')

parser.add_argument('-l', '--list', dest="list", action='store_true', default=False,
                    help='Get list of available scenarios')

parser.add_argument('-s', '--simulate', dest="simulate", action='store_true', default=False,
                    help='Run simulation and postprocessing (false by default)')

parser.add_argument('-p', '--postprocess', dest="postprocess", action='store_true', default=False,
                    help='Run postprocessing (false by default, always true if simulate is enabled)')

parser.add_argument('-g', '--no-graph', dest="graph", action='store_false', default=True,
                    help='Do not build a graph for the scenario (builds a graph by default)')

args = parser.parse_args()

if not args.list and len(args.scenarios)==0:
    print "ERROR: at least one scenario need to be specified"
    parser.print_help()
    exit (1)

if args.list:
    print "Available scenarios: "
else:
    if args.simulate:
        print "Simulating the side-channel attack for following scenario application: " + ",".join (args.scenarios)

    if args.graph:
        print "When simulation finished, will be building graphs for the following scenarios: " + ",".join (args.scenarios)

######################################################################
######################################################################
######################################################################

class SimulationJob (workerpool.Job):
    "Job to simulate things"
    def __init__ (self, cmdline):
        self.cmdline = cmdline
    def run (self):
        print (" ".join (self.cmdline))
        subprocess.call (self.cmdline)

pool = workerpool.WorkerPool(size = multiprocessing.cpu_count())

class Processor:
    def run (self):
        if args.list:
            print "    " + self.name
            return

        if "all" not in args.scenarios and self.name not in args.scenarios:
            return

        if args.list:
            pass
        else:
            if args.simulate:
                self.simulate ()
                pool.join ()
            if args.simulate or args.postprocess:
                self.postprocess ()
                pool.join ()
            if args.graph:
                self.graph ()

    def graph (self):
        subprocess.call ("./graphs/%s.R" % self.name, shell=True)

class ConvertTopologies (Processor):
    def __init__ (self, name, runs, topology, buildGraph = False):
        self.name = name
        self.runs = runs
        self.topology = topology
        self.buildGraph = buildGraph

        for run in runs:
            try:
                os.mkdir ("topology/bw-delay-rand-%d" % run)
            except:
                pass # ignore the error

    def simulate (self):
        for topology in self.topology:
            for run in self.runs:
                cmdline = ["./build/rocketfuel-maps-cch-to-annotaded",
                           "--topology=topology/rocketfuel_maps_cch/%s.cch" % topology,
                           "--run=%d" % run,
                           "--output=topology/bw-delay-rand-%d/%s" % (run, topology),
                           "--buildGraph=%d" % self.buildGraph,
                           "--keepLargestComponent=1",
                           "--connectBackbones=1",
                           "--clients=3",
                           ]
                job = SimulationJob (cmdline)
                pool.put (job)

    def postprocess (self):
        # any postprocessing, if any
        pass

    def graph (self):
        pass

class SideChannelAttack (Processor):
    def __init__ (self, name, algorithms, topology, evils, good, runs, folder, producer="gw", defaultRtt="250ms"):
        self.name = name
        self.algorithms = algorithms
        self.topology = topology
        self.evils = evils
        self.good = good
        self.runs = runs
        self.folder = folder
        self.producer = producer
        self.defaultRtt = defaultRtt

    def simulate (self):
        for algorithm in self.algorithms:
            for topology in self.topology:
                for evil in self.evils:
                    for run in self.runs:
                        cmdline = ["./build/ndntube", #here  scenario for ndntube !!!
                                   #"./waf --run=tesbed --vis",
                                   "--algorithm=%s" % algorithm,
                                   "--run=%d" % run,
                                   "--topology=%s" % topology,
                                   "--badCount=%d" % evil,
                                   "--goodCount=%d" % self.good,
                                   "--folder=%s" % self.folder,
                                   "--producer=%s" % self.producer,
                                   "--defaultRtt=%s" % self.defaultRtt,
                                   ]
                        job = SimulationJob (cmdline)
                        pool.put (job)

    def postprocess (self):
        print "Convert data to sqlite and reduce data size"


        for algorithm in self.algorithms:
            for topology in self.topology:
                for evil in self.evils:
                    for run in self.runs:


                        name = "results/%s/%s-topo-%s-evil-%d-good-%d-producer-%s-run-%d" % (
                            self.folder,
                            algorithm,
                            topology,
                            evil,
                            self.good,
                            self.producer,
                            run)
                        print "Converting %s" % name

                        bzip = "%s.txt.bz2" % name
                        sqlite = "%s.db" % name

                        subprocess.call ("rm -f \"%s\"" % sqlite, shell=True)
                        #cmd = "bzcat \"%s\" | tail -n +2 | awk -F\"\t\" '{if (NF == 4) {print $1\"|\"$2\"|\"$3\"|\"$4}}' | sqlite3 -cmd \"create table data_tmp (Time int, Node text, Type text, Packets real)\" -cmd \".import /dev/stdin data_tmp\" \"%s\"" % (bzip, sqlite)
                        cmd = "bzcat \"%s\" | tail -n +2 | awk -F\"\t\" '{if (NF == 9) {print $1\"|\"$2\"|\"$5\"|\"$6\"|\"$9}}' | sqlite3 -cmd \"create table data_tmp (Time int, Node text, Type text, DelayS int, HopCount real)\" -cmd \".import /dev/stdin data_tmp\" \"%s\"" % (bzip, sqlite)
                        
                        subprocess.call (cmd, shell=True)

                        #cmd = "sqlite3 -cmd \"create table data as select Time,Node,Type,sum(Packets) as Packets from data_tmp where Type in ('CacheHits', 'CacheMisses') group by Time,Node,Type\" -cmd \"drop table data_tmp\" -cmd \"vacuum\" \"%s\" </dev/null"  % sqlite
                        cmd = "sqlite3 -cmd \"create table data as select Time,Node,Type,DelayS,avg(HopCount) as HopCount from data_tmp where HopCount group by Time,Node,Type,DelayS,HopCount\" -cmd \"drop table data_tmp\" -cmd \"vacuum\" \"%s\" </dev/null"  % sqlite
                        # print cmd
                        subprocess.call (cmd, shell=True)

    def graph (self):
        print "graph_rate_verification"

        for algorithm in self.algorithms:
             for topology in self.topology:
                 for evil in self.evils:
                   # print "Scenario [%s], topology [%s], evils [%d], run [%d]" % (prefix, topology, evil, run)
                     cmdline = ["./graphs/rates.R",
                               algorithm,
                               topology,
                               str(evil),
                               ",".join([str(run) for run in self.runs]),
                               self.folder,
                               str(self.good),
                               self.producer,
                               ]
                     job = SimulationJob (cmdline)
                     pool.put (job)

class CachehitRatioAlgorithms (SideChannelAttack):
    def __init__ (self, name, scenario):
        self.name = name
        self.algorithms = scenario.algorithms
        self.topology = scenario.topology
        self.evils = scenario.evils
        self.good = scenario.good
        self.runs = scenario.runs
        self.folder = scenario.folder
        self.producer = scenario.producer
        self.defaultRtt = scenario.defaultRtt

    def simulate (self):
        # Here we just assume that the relevant simulation data is already exists
        pass

    def postprocess (self):
        print "Aggregate data"

        for algorithm in self.algorithms:
            for topology in self.topology:
                for evil in self.evils:
                    cmdline = ["./graphs/preprocess.R",
                               algorithm,
                               topology,
                               str(evil),
                               ",".join([str(run) for run in self.runs]),
                               self.folder,
                               str(self.good),
                               self.producer,
                               ]
                    job = SimulationJob (cmdline)
                    print ">>> "+" ".join (cmdline)+"<<< \n";
                    pool.put (job)

    def graph (self):
        print "CachehitRatioAlgorithms"

        for topology in self.topology:
            cmdline = ["./graphs/cachehitratio-min-max-vs-time.R",
                       ",".join(self.algorithms),
                       topology,
                       ",".join([str(evil) for evil in self.evils]),
                       ",".join([str(run) for run in self.runs]),
                       self.folder,
                       str(self.good),
                       self.producer
                       ]
            job = SimulationJob (cmdline)
            pool.put (job)

class AttackersFigures (CachehitRatioAlgorithms):
    def simulate (self):
        # Here we just assume that the relevant simulation data is already exists
        pass

    def postprocess (self):
        pass

    def graph (self):
        print "AttackersFigures"

        for topology in self.topology:
            cmdline = ["./graphs/cachehitratio-vs-number-of-attackers.R",
                       ",".join(self.algorithms),
                       topology,
                       ",".join([str(evil) for evil in self.evils]),
                       ",".join([str(run) for run in self.runs]),
                       self.folder,
                       str(self.good),
                       self.producer
                       ]
            job = SimulationJob (cmdline)
            pool.put (job)

try:
    conversion = ConvertTopologies (name="convert-topologies",
                                    runs=[1],
                                    topology=["1221.r0",
                                                "1239.r0",
                                                "1755.r0",
                                                "2914.r0",
                                                "3257.r0",
                                                "3356.r0",
                                                "3967.r0",
                                                "4755.r0",
                                                "6461.r0",
                                                "7018.r0",],
                                    buildGraph = True)
    # conversion.run ()

    smallTree = SideChannelAttack (name="attack-small-tree",
                                    algorithms = ["Lru", "Fifo","Lfu","Probability", "Random", "dad"],
                                    topology = ["small-tree"],
                                    evils = [2],
                                    #evils = [1,2],
                                    good  = 0, # number of client nodes minus number of evil nodes
                                    runs = range(1,11), 
                                    folder = "attackSmallTree",
                                    producer = "bb",
                                    defaultRtt = "80ms")
    smallTree.run ()

    tree = SideChannelAttack (name="attack-tree",
                               algorithms = ["dad"],
                               topology = ["tree"],
                               evils = [4],
                               #evils = range(1,10,2),
                               good  = 0, # number of client nodes minus number of evil nodes
                               runs = range(1,11), 
                               folder = "attackTree",
                               producer = "bb",
                               defaultRtt = "80ms")
    tree.run ()

    isp = SideChannelAttack (name="attack-isp",
                              algorithms = ["Lru1", "Lfu", "Fifo"],                                
                              topology = ["7018.r0"],
                              evils = [int(296*0.5)],#50% of consumer candidates.
                              good  = 0, # number of client nodes minus number of evil nodes
                              runs=[1],
                             # runs = range(1,11), 
                              folder = "attackISP",
                              producer = "bb",
                              defaultRtt = "330ms")
    isp.run ()


    testbed = SideChannelAttack (name="attack-vondn",
                              #algorithms = ["Lru1","Lru2","Probability1","Probability2", "dad1","dad2"],                                 
                              algorithms = ["dad2"],
                              topology = ["testbed"],
                              evils = [int(420*0.4)],#40% of consumer candidates.
                              good  = 0, # number of client nodes minus number of evil nodes
                              runs=[1],
                              #runs = range(1,11), 
                              folder = "attack_vondn",
                              producer = "bb",
                              defaultRtt = "330ms")
    testbed.run ()

    CachehitRatioAlgorithms (name="ratios-small-tree", scenario=smallTree).run ()
    CachehitRatioAlgorithms (name="ratios-tree", scenario=tree).run ()
    CachehitRatioAlgorithms (name="ratios-isp", scenario=isp).run ()
    CachehitRatioAlgorithms (name="ratios-vondn", scenario=testbed).run ()
    AttackersFigures (name="attacker-small-tree", scenario=smallTree).run ()
    AttackersFigures (name="attacker-tree",  scenario=tree).run ()
    AttackersFigures (name="attacker-isp", scenario=isp).run ()
    AttackersFigures (name="attacker-vondn", scenario=testbed).run ()

finally:
    pool.join ()
    pool.shutdown ()
