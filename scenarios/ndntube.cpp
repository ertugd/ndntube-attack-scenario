/**  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

Copyright (c) 2019, Algorithmi Research Centree, University of Minho, Portugal


 * Authors: Ertugrul Dogruluk <d7474@di.uminho.pt>, Department of #Informatics. Portugal.

**/


#include "../extensions/BadGuy.h"
#include "../extensions/content-store-privacy.hpp"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/point-to-point-module.h"
#include <boost/lexical_cast.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/plus.hpp>
#include <boost/mpl/int.hpp>
namespace mpl = boost::mpl;
#include </home/ertugrul/Desktop/ndnSIM/ns-3/src/core/model/random-variable-stream.h>
#include "ns3/node-container.h"
#include "ns3/ptr.h"
#include <ns3/ndnSIM/NFD/daemon/table/fib.hpp>

using namespace ns3;
using namespace ns3::ndn;
using namespace std;

uint32_t Run = 1;

void PrintTime (Time next, const string name)
{
  cerr << "Simulating= " << name << " " << Simulator::Now ().ToDouble (Time::S) << "s" << endl;
  Simulator::Schedule (next, PrintTime, next, name);
}

int normalNetworkConfiguration(string prefix, NodeContainer nodes) {
	StackHelper helper;
	if (prefix == "default") {
		helper.SetOldContentStore("ns3::ndn::cs::Privacy::Lru","MaxSize", "1000", "CacheProbability", "1", "UpdateInterval", "200", "CHRatioTreshold", "5"); //200=0.2secs.
		helper.Install(nodes);
		StrategyChoiceHelper::Install(nodes, "/ndntube/videos",
				"/localhost/nfd/strategy/multicast"); // nfd forwarding strategy
	} else if (prefix == "Lru1") {
		helper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "1000"); //if 0 it is unlimited
		helper.Install(nodes);
		StrategyChoiceHelper::Install(nodes, "/ndntube/videos",
				"/localhost/nfd/strategy/multicast"); // nfd forwarding strategy
	} else if (prefix == "Lru2") {
		helper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "1000"); //if 0 it is unlimited
		helper.Install(nodes);
		StrategyChoiceHelper::Install(nodes, "/ndntube/videos",
				"/localhost/nfd/strategy/multicast"); // nfd forwarding strategy
	} else if (prefix == "Fifo") {
			helper.SetOldContentStore("ns3::ndn::cs::Fifo", "MaxSize", "1000"); //if 0 it is unlimited
			helper.Install(nodes);
			StrategyChoiceHelper::Install(nodes, "/ndntube/videos",
					"/localhost/nfd/strategy/multicast"); // nfd forwarding strategy
	} else if (prefix == "Lfu") {
			helper.SetOldContentStore("ns3::ndn::cs::Lfu", "MaxSize", "1000"); //if 0 it is unlimited
			helper.Install(nodes);
			StrategyChoiceHelper::Install(nodes, "/ndntube/videos",
					"/localhost/nfd/strategy/multicast"); // nfd forwarding strategy
	} else if (prefix == "Random") {
		helper.SetOldContentStore("ns3::ndn::cs::Random", "MaxSize", "1000");
		helper.Install(nodes);
		StrategyChoiceHelper::Install(nodes, "/ndntube/videos",
				"/localhost/nfd/strategy/multicast"); // nfd forwarding strategy
	} else if (prefix == "Probability1") {
		helper.SetOldContentStore("ns3::ndn::cs::Probability::Lru","MaxSize", "100", "CacheProbability", "0.1");
		helper.Install(nodes);
		StrategyChoiceHelper::Install(nodes, "/ndntube/videos",
				"/localhost/nfd/strategy/multicast"); // nfd forwarding strategy
	} else if (prefix == "freshness") {
		helper.SetOldContentStore("ns3::ndn::cs::Freshness::Random","MaxSize", "100");
		helper.Install(nodes);
		StrategyChoiceHelper::Install(nodes, "/ndntube/videos",
				"/localhost/nfd/strategy/multicast"); // nfd forwarding strategy
	} else if (prefix == "dad1") {
		helper.SetOldContentStore("ns3::ndn::cs::Privacy::Lru","MaxSize", "100", "CacheProbability", "1.0", "UpdateInterval", "3200", "CHRatioTreshold", "0.001");
		helper.Install(nodes);
		StrategyChoiceHelper::Install(nodes, "/ndntube/videos",
				"/localhost/nfd/strategy/multicast"); // nfd forwarding strategy
	} else if (prefix == "dad2") {
		helper.SetOldContentStore("ns3::ndn::cs::Privacy::Lru","MaxSize", "100", "CacheProbability", "1.0", "UpdateInterval", "3200", "CHRatioTreshold", "0.01");
		helper.Install(nodes);
		StrategyChoiceHelper::Install(nodes, "/ndntube/videos",
				"/localhost/nfd/strategy/multicast"); // nfd forwarding strategy
	} else if (prefix == "nocache") {
		helper.SetOldContentStore("ns3::ndn::cs::Nocache");
		helper.Install(nodes);
		StrategyChoiceHelper::Install(nodes, "/ndntube/videos",
				"/localhost/nfd/strategy/multicast"); // nfd forwarding strategy
	} else {
		cerr << "Invalid scenario Content Store prefix" << endl;
		return -1;
	}
	return 0;
}


int specialNetworkConfiguration(string prefix, NodeContainer nodes) {
	StackHelper helper;
	// bad guys will not use their own content store during the attack. Therefore, here turns off the cs for evil nodes..
	helper.SetOldContentStore("ns3::ndn::cs::Nocache");
	helper.Install(nodes);
	StrategyChoiceHelper::Install(nodes, "/ndntube/videos",
			"/localhost/nfd/strategy/multicast"); //defult nfd forwarding strategy
	return 0;
}

int
main(int argc, char* argv[])
{
	  string topology = "7018.r0";
	  string prefix = "default";
	  string producerLocation = "bb";
	  string consumerLocation = "leaf";
	  Time evilGap = Time::FromDouble (0.02, Time::MS);
	  Time defaultRtt = Seconds (0.25);
	  uint32_t badCount = 30;
	  uint32_t goodCount = 100;
	  uint32_t prodCount = 1;
	  string folder = "tmp";

  // 1. Process command line arguments...
  CommandLine cmd;
  cmd.AddValue ("topology", "Topology", topology);
  cmd.AddValue ("run", "Run", Run);
  cmd.AddValue ("algorithm", "Side-Channel Timing attack mitigation algorithms", prefix);
  cmd.AddValue ("producer", "Producer location: gw or bb", producerLocation);
  cmd.AddValue ("consumer", "Consumer location: leaf or ...", consumerLocation);
  cmd.AddValue ("badCount", "Number of bad guys", badCount);
  cmd.AddValue ("goodCount", "Number of good guys", goodCount);
  cmd.AddValue ("prodCount", "Number of producer guys", prodCount);
  cmd.AddValue ("folder", "Folder where results will be placed", folder);
  cmd.AddValue ("defaultRtt", "Default RTT for BDP limits", defaultRtt);
  cmd.Parse (argc, argv);

  Config::SetGlobal ("RngRun", IntegerValue (Run));

  // 2. Define the name of the files to output the results...
  string name = prefix;
  name += "-topo-" + topology;
  name += "-evil-" + boost::lexical_cast<string> (badCount);
  name += "-good-" + boost::lexical_cast<string> (goodCount);
  name += "-producer-" + producerLocation;
  name += "-run-"  + boost::lexical_cast<string> (Run);

  string results_file = "results/" + folder + "/" + name + ".txt";
  string meta_file    = "results/" + folder + "/" + name + ".meta";
  string graph_dot_file    = "results/" + folder + "/" + name + ".dot";
  string graph_pdf_file    = "results/" + folder + "/" + name + ".pdf";

  system (("mkdir -p \"results/" + folder + "\"").c_str ());
  ofstream os (meta_file.c_str(), ios_base::out | ios_base::trunc);
  os << "Streaming Attack Scenario run: " << Run << " is starting..." << endl;


  // 3. Read the topology file to memory...
  AnnotatedTopologyReader topologyReader ("", 1.0);
  topologyReader.SetFileName ("topology/" + topology + ".txt");
  os << "Reading topology file for Streaming Attack scenario: " << "topology/" + topology + ".txt" << endl;
  topologyReader.Read ();
  //os << "OSPF> " << endl;
  topologyReader.ApplyOspfMetric ();

  // 4. Create a random number generator
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();

  // 5. Create containers and select potential consumers and producers using names...
  NodeContainer allNodes = topologyReader.GetNodes();
  vector<Ptr<Node>> producerCandidates;
  vector<Ptr<Node>> consumerCandidates;
  set<Ptr<Node>> producers;
  set<Ptr<Node>> consumers;
  set<Ptr<Node>> badGuys;
  for (NodeContainer::Iterator node = allNodes.Begin (); node != allNodes.End (); node++) {
      if (Names::FindName (*node).find(producerLocation)!=std::string::npos) {
    	  // found a node with name containing the string defined in producerLocation
    	  producerCandidates.push_back(*node);
      }
      else if (Names::FindName (*node).find(consumerLocation)!=std::string::npos) {
    	  // found a node with name containing the string defined in consumerLocation
    	  consumerCandidates.push_back(*node);
      }
  }
  os << "THE TOTAL NUMBER OF THE NODE IS:           " << allNodes.GetN()  << endl;
  os << "NUMBER OF PRODUCER NODE CANDIDATE IS:      " << producerCandidates.size()  << endl;
  os << "NUMBER OF CONSUMER NODE CANDIDATE IS:      " << consumerCandidates.size()  << endl;

  // when goodCount is zero that means that all nodes are good guys except the evil ones
  if ( goodCount == 0 )
	  goodCount = consumerCandidates.size() - badCount;

  // sanity check to avoid problems...
  if (producerCandidates.size() < prodCount) {
	  os << "Error: not enough PRODUCER candidates available." << endl;
	  exit (1);
  }
  if (consumerCandidates.size() < goodCount + badCount) {
	  os << "Error: not enough CONSUMER candidates available for good and bad guys." << endl;
	  exit (1);
  }

  // 6. Place the producers randomly...
  while (producers.size () < prodCount) {
	  uint32_t node_number = rand->GetValue(0,producerCandidates.size());
	//  os << "PRODUCER: Random value between 0 and " << producerCandidates.size() << " --> " << node_number << endl;
	  Ptr<Node> node = producerCandidates.at(node_number);
	  if (producers.find(node) != producers.end ())
		  continue;
	  producers.insert (node);
	  string name = Names::FindName (node);
	  Names::Rename (name, "producer-"+name);
	  os << "Selected PRODUCER node is: \t " << name << endl;
  }

  // 7. Place the bad guys randomly...
  while (badGuys.size () < badCount) {
	  uint32_t node_number = rand->GetValue(0,consumerCandidates.size());
	//  os << "BADGUY: Random value between 0 and " << consumerCandidates.size() << " --> " << node_number << endl;
	  Ptr<Node> node = consumerCandidates.at(node_number);
	  if (badGuys.find(node) != badGuys.end ())
		  continue;
	  badGuys.insert (node);
	  string name = Names::FindName (node);
	  Names::Rename (name, "evil-"+name);
	  os << "Randomly selected EVIL node is: \t " << name << " ---> Adversary node for the side-channel timing attack" << endl;

  }

  // 8. Place the good guys randomly...
  while (consumers.size () < goodCount) {
	  uint32_t node_number = rand->GetValue(0,consumerCandidates.size());
	//  os << "CONSUMER: Random value between 0 and " << consumerCandidates.size() << " --> " << node_number << endl;
	  Ptr<Node> node = consumerCandidates.at (node_number);
      if (consumers.find (node) != consumers.end () ||
          badGuys.find (node) != badGuys.end ())
        continue;

      consumers.insert (node);
      string name = Names::FindName (node);
      Names::Rename (name, "good-"+name);
      os << "Randomly selected GOOD node is: \t " << name << " ---> Legitimate consumer node for Streaming" << endl;

    }

  // 9. Network Configuration: special config for badGusy, normal config for everybody else
  NodeContainer normalNodes;
  NodeContainer specialNodes;
  for (NodeContainer::Iterator node = allNodes.Begin (); node != allNodes.End (); node++) {
	  if (badGuys.find(*node) != badGuys.end ())
		  specialNodes.Add(*node);
	  else
		  normalNodes.Add(*node);

  }
  normalNetworkConfiguration(prefix, normalNodes);
  specialNetworkConfiguration(prefix, specialNodes);

  // 10. Installing global routing interface on all nodes
  GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // 11. Application configurations: BadGuys
  AppHelper BadGuy("ns3::ndn::BadGuy");
  BadGuy.SetAttribute("Frequency", StringValue("100"));
  BadGuy.SetAttribute("BadBehavior", BooleanValue(true));
  //BadGuy.SetAttribute ("LifeTime", StringValue ("2s"));
  BadGuy.SetAttribute("SpySeq", IntegerValue(1));

  for (set<Ptr<Node>>::iterator node = badGuys.begin(); node != badGuys.end(); node++)
    {
      ApplicationContainer evilApp;
      //BadGuy.SetAttribute("SpyPrefix", StringValue("/evil"+Names::FindName (*node)));
      //BadGuy.SetAttribute("SpyPrefix", StringValue("/ndntube/videos/good-leaf-2/%FE%02%81"));
    //  BadGuy.SetAttribute("SpyPrefix", StringValue("/ndntube/videos"));

     // BadGuy.SetAttribute("SpyPrefix", StringValue("/ndntube/videos/good-leaf-1"));
     // BadGuy.SetPrefix("/ndntube/videos");

      BadGuy.SetAttribute("SpyPrefix", StringValue("/ndntube/videos"));
      BadGuy.SetAttribute("SpyLeafName", StringValue("good"));
      BadGuy.SetAttribute("SpyLeafCount", IntegerValue(goodCount));
      BadGuy.SetAttribute("BruteForce", IntegerValue(1));
      BadGuy.SetAttribute("InterestRepeat", IntegerValue(4));

      evilApp.Add (BadGuy.Install (*node));
    // evilApp.Start (Seconds (300.0));
     //evilApp.Stop (Seconds (600.0));
      evilApp.Start (Seconds (21.0) + Time::FromDouble (rand->GetValue (0, 1), Time::MS));//MS>S//////////////////////////////////////////////////////////////////////////
      evilApp.Stop (Seconds (40.0) + Time::FromDouble (rand->GetValue (0, 1), Time::MS));//MS>S
    }

  // 12. Application configurations: Goodguys as consumers
  AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  //consumerHelper.SetAttribute ("LifeTime", StringValue ("2s"));
  consumerHelper.SetAttribute("Frequency", StringValue("100"));

  for (set<Ptr<Node>>::iterator node = consumers.begin(); node != consumers.end(); node++) {
		ApplicationContainer goodApp;
		consumerHelper.SetPrefix("/ndntube/videos/" + Names::FindName(*node));
	    os << "Streaming consumer is: \t "  << Names::FindName(*node) << ", prefix ="<< "/ndntube/videos/" + Names::FindName(*node) + "/stream-user-id/"<< rand->GetInteger(100000,9000000) << endl;

		//goodAppHelper.SetAttribute ("AvgGap", TimeValue (Seconds (1.100 / maxNonCongestionShare)));
		goodApp.Add(consumerHelper.Install(*node));
		//os << "Adding a good guy: install on node " << endl;
		goodApp.Start(Seconds(0.0) + Time::FromDouble(rand->GetValue(0, 1), Time::S));
   }

  // 13. Application configurations: Goodguys as consumers
  AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetPrefix ("/ndntube/videos");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
  producerHelper.SetAttribute("Freshness", StringValue("100ms"));
  for (set<Ptr<Node>>::iterator node = producers.begin(); node != producers.end(); node++) {
		ApplicationContainer producerApp;
		ndnGlobalRoutingHelper.AddOrigins ("/", *node);
		producerHelper.SetAttribute("Signature", UintegerValue(1));
		producerHelper.SetAttribute("KeyLocator", StringValue("/ndntube/videos/domain/KEY/"+Names::FindName(*node)));
		producerApp.Add(producerHelper.Install(*node));
		//os << "Adding a good guy: install on node " << endl;
		producerApp.Start(Seconds(0.0));
   }

  // 14. Calculate and install FIBs
  ndnGlobalRoutingHelper.CalculateRoutes ();   // Calculate and install FIBs


  // 15. Output information only...

   //os << "Number of evil nodes: " << badCount << endl;
  //os << "Number of good nodes: " << goodCount << endl;

  os << "EVIL node quantity is:  " << badGuys.size() << endl;
  for (set<Ptr<Node>>::iterator node = badGuys.begin(); node != badGuys.end(); node++) {
	  os << Names::FindName(*node) << " ";
  }
  os << endl;
  os << "GOOD node quantity is:  " << consumers.size() << endl;
  for (set<Ptr<Node>>::iterator node = consumers.begin(); node != consumers.end(); node++) {
	  os << Names::FindName(*node) << " ";
  }
  os << endl;
  os << "PRODUCER node quantity is:  " << producers.size() << endl;
  for (set<Ptr<Node>>::iterator node = producers.begin(); node != producers.end(); node++) {
	  os << Names::FindName(*node) << " ";
  }
  os << endl;


    // 	double maxNonCongestionShare = 0.8 * calculateNonCongestionFlows (goodNodes, producerNodes);
    // os << "maxNonCongestionShare   " << maxNonCongestionShare << endl;

     // saveActualGraph(graph_dot_file, NodeContainer (goodNodes, evilNodes));

     	// system (("twopi -Tpdf \"" + graph_dot_file + "\" > \"" + graph_pdf_file + "\"").c_str ());
      	 // cout << "Write effective topology graph to: " << graph_pdf_file << endl;
      	 // cout << "Max non-congestion share:   " << maxNonCongestionShare << endl;

  // 16. Trace and simulate ...


 //L3RateTracer::InstallAll (results_file, Seconds (1.0));
   CsTracer::InstallAll(results_file, Seconds (0.1));
// AppDelayTracer::InstallAll (results_file);
 //L2RateTracer::InstallAll(results_file);
 //L3RateTracer::InstallAll(results_file);

  Simulator::Schedule (Seconds (10.0), PrintTime, Seconds (10.0), name);

  Simulator::Stop (Seconds (60.0));
  Simulator::Run ();
  Simulator::Destroy ();
  //L3RateTracer::Destroy ();
 // AppDelayTracer::Destroy ();

 CsTracer::Destroy();

  cerr << "Archiving to: " << results_file << ".bz2" << endl;
  system (("rm -f \"" + results_file + ".bz2" + "\"").c_str() );
  system (("bzip2 \"" + results_file + "\"").c_str() );

  return 0;
}

