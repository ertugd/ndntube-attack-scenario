/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "CA-cert.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"


#include <memory>

NS_LOG_COMPONENT_DEFINE("ndn.CA_Cert");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(CA_Cert);

TypeId
CA_Cert::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::CA_Cert")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<CA_Cert>()
      .AddAttribute("Prefix", "Prefix, for which CA_Cert has the data", StringValue("/"),
                    MakeNameAccessor(&CA_Cert::m_prefix), MakeNameChecker())
      .AddAttribute(
         "Postfix",
         "Postfix that is added to the output data (e.g., for adding CA_Cert-uniqueness)",
         StringValue("/"), MakeNameAccessor(&CA_Cert::m_postfix), MakeNameChecker())
      .AddAttribute("PayloadSize", "Virtual payload size for Content packets", UintegerValue(1024),
                    MakeUintegerAccessor(&CA_Cert::m_virtualPayloadSize),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                    TimeValue(Seconds(0)), MakeTimeAccessor(&CA_Cert::m_freshness),
                    MakeTimeChecker())
      .AddAttribute(
         "Signature",
         "Fake signature, 0 valid signature (default), other values application-specific",
         UintegerValue(0), MakeUintegerAccessor(&CA_Cert::m_signature),
         MakeUintegerChecker<uint32_t>())
      .AddAttribute("KeyLocator",
                    "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), MakeNameAccessor(&CA_Cert::m_keyLocator), MakeNameChecker());
  return tid;
}

CA_Cert::CA_Cert()
{
  NS_LOG_FUNCTION_NOARGS();
}

// inherited from Application base class.
void
CA_Cert::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
}

void
CA_Cert::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}

void
CA_Cert::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside

  NS_LOG_FUNCTION(this << interest);

  if (!m_active)
    return;

  Name dataName(interest->getName());
  // dataName.append(m_postfix);
  // dataName.appendVersion();

  // get the interest name and find out wich request
  // interest names start witk a prefix for KEY like "/ndn/domain/KEY/+NodeName"
  // suffix identifies the type of request:
  //     -- ndn/domain/KEY/<NodeName>/ID-CERT      --> answer with the certificate if exists.
  //     -- ndn/domain/KEY/<NodeName>/NEW-CERT/...  --> create a new certificate for key

  std::string uri = dataName.toUri();
  auto data = make_shared<Data>();
  data->setName(dataName);
  data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

  std::cout << "CA-CERT Interest received:" << data->getName() << "\n";
  if (uri.find("ID-CERT") !=std::string::npos ) {
	  // locate a certificate in database and send it back
	  std::map<Name,Certificate>::iterator it = certificates.find(dataName);
	  if (it != certificates.end()) {
		  // it exists...  get the certificate
		  Certificate c = it->second;
		  data->setContent(c.wireEncode());
	  } else {
		  // not found ... send NACK?
		  auto control = make_shared<ControlResponse>();
		  control->setCode(404);
		  control->setText("NACK");
		  data->setContent(control->wireEncode());
	  }

  } else if (uri.find("NEW-CERT") !=std::string::npos ) {
	  // create a certficiate for key and store it in database

  }

  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }

  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::makeNonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

  data->setSignature(signature);

  NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());

  // to create real wire encoding
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data);
}

} // namespace ndn
} // namespace ns3
