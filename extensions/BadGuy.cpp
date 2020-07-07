#include "BadGuy.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/object-vector.h"
#include "ns3/node-container.h"


#include "ns3/names.h"
#include <ndn-cxx/lp/tags.hpp>


NS_LOG_COMPONENT_DEFINE("ndn.BadGuy");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(BadGuy);

TypeId
BadGuy::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::BadGuy")
      .SetGroupName("Ndn")
      .SetParent<ConsumerCbr>()
      .AddConstructor<BadGuy>()

      .AddAttribute("BadBehavior", "Am i a good guy or a bad guy",
                    BooleanValue(true),
                    MakeBooleanAccessor(&BadGuy::m_bad_behavior), MakeBooleanChecker())

      .AddAttribute("SpySeq", "The sequence number to Spy",
                    IntegerValue(0),
                    MakeIntegerAccessor(&BadGuy::m_spy_seq), MakeIntegerChecker<uint32_t>())

	  .AddAttribute("SpyPrefix", "The prefix that we want to spy",
			StringValue("/"),
			MakeStringAccessor(&BadGuy::m_spy_prefix), MakeStringChecker())

	  .AddAttribute("SpyLeafName", "The name of the leaf name to spy on",
			StringValue("/"),
			MakeStringAccessor(&BadGuy::m_spy_leafname), MakeStringChecker())

	  .AddAttribute("SpyLeafCount", "Number of good guys to check",
			IntegerValue(1),
            MakeIntegerAccessor(&BadGuy::m_spy_leafcount), MakeIntegerChecker<uint32_t>())

	  .AddAttribute("SpyAllNames", "A list of names to spy on ",
					ObjectVectorValue(),
		            MakeObjectVectorAccessor(&BadGuy::m_spy_allnames),
					MakeObjectVectorChecker<Node>())

	   .AddAttribute("BruteForce", "brute force? yes or no",
			IntegerValue(0),
		    MakeIntegerAccessor(&BadGuy::m_spy_bruteforce), MakeIntegerChecker<uint32_t>())

		.AddAttribute("InterestRepeat", "number of repetitions for each interest",
			IntegerValue(1),
			MakeIntegerAccessor(&BadGuy::m_interest_repeat), MakeIntegerChecker<uint32_t>())
    ;

  return tid;
}

BadGuy::BadGuy()
  : m_bad_behavior(true)
  , m_spy_prefix("/")
  , m_spy_seq(0)
  , m_spy_leafname("")
  , m_spy_leafcount(1)
  , m_spy_bruteforce(0)
  , m_interest_repeat(1)
{
  NS_LOG_FUNCTION_NOARGS();
}

BadGuy::~BadGuy()
{
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
void
BadGuy::StartApplication()
{
	//App::StartApplication();
	//BadGuy::ScheduleNextPacket();
	ConsumerCbr::StartApplication();

	// prepare a list of good guys to attack...
	rand = CreateObject<UniformRandomVariable>();
	//for (uint32_t i=1; i<=m_spy_leafcount; i++)
	//	m_spy_list.push_back(i);

	// Get all nodes from topology... attack all that have name match with spy_leafname
	NodeContainer allNodes = NodeContainer::GetGlobal();
	for (NodeContainer::Iterator node = allNodes.Begin (); node != allNodes.End (); node++)
	      if (Names::FindName (*node).find(m_spy_leafname)!=std::string::npos)
	    	  m_spy_list.push_back(Names::FindName(*node));
	//BadGuy::dumpRttEstimate();
}



void
BadGuy::dumpRttEstimate()

{
  if (m_rtt) {
	 // std::cout << Now() << "\t" << "Malicious Node" << "\t" << m_rtt->GetCurrentEstimate() << "\n";
	//ObjectBase::ConstructSelf(AttributeConstructionList());
	 // NS_LOG_DEBUG ("ESTIMATED VALUE: " << m_rtt->GetCurrentEstimate()<<"sec.");
  }
  Simulator::Schedule(Seconds(0.1), &BadGuy::dumpRttEstimate, this);
 }

Time
BadGuy::GetRTTValue(void) const
{
	std::cout << "Getting the value";

	return m_rtt->GetCurrentEstimate();
	// return m_rtt_value;
}

void
BadGuy::SetRTTValue(Time newValue)
{
//	std::cout << "Setting the value to " << newValue << "\n";

	//m_rtt_value = newValue;
}

void
BadGuy::OnData(shared_ptr<const Data> data)
{
	  App::OnData(data); // tracing inside

	//  std::cout << "BadGuy received a data packet. Name: " << data->getFullName() << "\n"; //burasi output for terminal

	  const ndn::Signature& sig = data->getSignature();
	//  std::cout << "Signature and signature info: " << sig.getValue() << " - " << sig.getSignatureInfo() << "\n"; //burasi output for terminal
	  const ndn::KeyLocator& keyloc = sig.getKeyLocator();
	//  std::cout << "Key locator name is: " << keyloc.getName() <<  "\n"; //burasi output for terminal

	//if (!ndn::security::v2::verifySignature(data, keyloc)) {
	//	std::cout << "Signature is valid \n"; //burasi output for terminal
	//}

	//if (!security::verifySignature(reply, state->m_ca.m_anchor)) {
	//    errorCallback("Cannot verify data from " + state->m_ca.m_caName.toUri());
	//    return;
	//  }
	// keychain.sign(your unsigned Data packet, security:: signingByCertificate(your certificate name));

	  //NS_LOG_FUNCTION(this << data);

	  // NS_LOG_INFO ("Received content object: " << boost::cref(*data));

	  // This could be a problem......
	  //uint32_t seq = data->getName().at(-1).toSequenceNumber();
	  //NS_LOG_INFO("< DATA for " << seq);
	  uint32_t seq = m_spy_seq;

	  int hopCount = 0;
	  Time fullDelay = Seconds(0);
	  Time lastDelay = Seconds(0);


	  auto hopCountTag = data->getTag<lp::HopCountTag>();
	  if (hopCountTag != nullptr) { // e.g., packet came from local node's cache
	    hopCount = *hopCountTag;
	  }
	  NS_LOG_DEBUG("Hop count: " << hopCount);

	  SeqTimeoutsContainer::iterator entry = m_seqLastDelay.find(seq);
	  if (entry != m_seqLastDelay.end()) {
	    m_lastRetransmittedInterestDataDelay(this, seq, Simulator::Now() - entry->time, hopCount);
	    lastDelay = Simulator::Now() - entry->time;
	  }

	  entry = m_seqFullDelay.find(seq);
	  if (entry != m_seqFullDelay.end()) {
	    m_firstInterestDataDelay(this, seq, Simulator::Now() - entry->time, m_seqRetxCounts[seq], hopCount);
	    fullDelay = Simulator::Now() - entry->time;
	  }

	  //  report the times for the prefix...
	//  std::cout << "[BadGuy] Prefix:" << data->getName() << " HopCount:  " << hopCount << " FullDelay: " << fullDelay << " LastDelay: " << lastDelay << "\n"; //burasi output for terminal
	  m_seqRetxCounts.erase(seq);
	  m_seqFullDelay.erase(seq);
	  m_seqLastDelay.erase(seq);

	  m_seqTimeouts.erase(seq);
	  m_retxSeqs.erase(seq);

	  m_rtt->AckSeq(SequenceNumber32(seq));
}

void
BadGuy::SendPacket()
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  uint32_t seq = m_spy_seq;
  // we set the prefix with the sequence number included! No need to add one...
  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_spy_prefix);
  if (m_spy_bruteforce ==1) {
	  std::string leaf_name = m_spy_leafname;
	  if (!m_repeat_list.empty()) {  // if we have interestes to repeat, send them first...
		  leaf_name = m_repeat_list.back();
		  m_repeat_list.pop_back();
	  } else if (!m_spy_list.empty()) {  // if attack list not empty, choose another guy to attack
		  // peak one random for the available
		  uint32_t j = rand->GetValue(0,m_spy_list.size());
		  leaf_name = m_spy_list.at(j);
		  m_spy_list.erase(m_spy_list.begin()+j);
		  // for this guy create repeat number in repeat list...
		  for (uint32_t i=0; i < m_interest_repeat; i++)
			  m_repeat_list.push_back(leaf_name);
	  } else if (m_spy_list.empty())  // if attack lits is empty, give up
			  return;
	  nameWithSequence = make_shared<Name>(m_spy_prefix+"/"+leaf_name);
  }
  //if (m_spy_seq > 0)
	//  nameWithSequence->appendSequenceNumber(m_spy_seq);
  //else
	 // try to obtain the seq number from the name...
  //zz	 seq = nameWithSequence->at(-1).toSequenceNumber();
//  std::cout << "Bad guy Requesting Interest with seq =" << seq << "\n";

  // FIX ME: Always appending  sequence number - or assuming it is already there !!
  // TRY: only with prefix

  // shared_ptr<Interest> interest = make_shared<Interest> ();
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*nameWithSequence);
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  NS_LOG_INFO("> Interest for " );
  //  std::cout << Now() << "\t" << "MailiciousNode" << "\t" << "Sending interest for " << nameWithSequence << "\n";

  WillSendOutInterest(seq);

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);

  ScheduleNextPacket();
}



///////////////////////////////////////////////////////////////////////////////////////////////////////

void
BadGuy::ScheduleNextPacket()
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  // std::cout << "next: " << Simulator::Now().ToDouble(Time::S) + mean << "s\n";

  if (m_firstTime) {
	if (m_bad_behavior)
		m_sendSpyEvent = Simulator::Schedule(Seconds(0.0), &BadGuy::SendPacket, this);
	else
		m_sendEvent = Simulator::Schedule(Seconds(0.0), &Consumer::SendPacket, this);
    m_firstTime = false;
  }
  else {
	  if (!m_bad_behavior && !m_sendEvent.IsRunning())
		  m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
                                                      : Seconds(m_random->GetValue()),
                                      &Consumer::SendPacket, this);
      if (m_bad_behavior && !m_sendSpyEvent.IsRunning())
    	m_sendSpyEvent = Simulator::Schedule(Seconds(1.0 / m_frequency),
                                    &BadGuy::SendPacket, this);
  }
}


} // namespace ndn
} // namespace ns3
