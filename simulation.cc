#include "ns3/traffic-control-module.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"

#include "ns3/point-to-point-module.h"
#include <iomanip>
#include "ns3/applications-module.h"

#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"

#include <cstring>
#include <math.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("networkSimulation");

FlowMonitorHelper flowmon;
Ptr<FlowMonitor> flowmonitor;

double percentCBR = 0;
double b = 0;
double simDuration = 0;
double tOn = 0.001;
double tOff = 0;
double D = 0;

double blockSize = 10;
double nbBlocks;

double x1sum1 = 0;
double x1sum2 = 0;
double x2sum1 = 0;
double x2sum2 = 0;

double respTime0;
double respTime1;

double x1respTime;
double x2respTime;
double x1sumRespTime;
double x2sumRespTime;
double x1pcktCnt;
double x2pcktCnt;

void intConf()
{
	Ptr<Ipv4FlowClassifier> classif = DynamicCast<Ipv4FlowClassifier>(
		flowmon.GetClassifier());
	FlowMonitor::FlowStatsContainer statistics = flowmonitor->GetFlowStats();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = statistics.begin(); i != statistics.end(); ++i)
	{
		if (i->first == 1)
		{
			x1respTime = (i->second.delaySum.GetSeconds() - x1sumRespTime) / (i->second.txPackets - x1pcktCnt);
			x1sumRespTime = i->second.delaySum.GetSeconds();
			x1pcktCnt = i->second.txPackets;
			x1sum1 += x1respTime;
			x1sum2 += x1respTime * x1respTime;
		}
		if (i->first == 2)
		{
			x2respTime = (i->second.delaySum.GetSeconds() - x2sumRespTime) / (i->second.txPackets - x2pcktCnt);
			x2sumRespTime = i->second.delaySum.GetSeconds();
			x2pcktCnt = i->second.txPackets;
			x2sum1 += x2respTime;
			x2sum2 += x2respTime * x2respTime;
		}
	}
	Simulator::Schedule(Seconds(simDuration / nbBlocks), &intConf);
}

int main(int argc, char *argv[])
{
	CommandLine cmd(__FILE__);
	cmd.AddValue("simDuration", "Simulation duration", simDuration);
	cmd.AddValue("percentCBR", "Percentage of CBR traffic", percentCBR);
	cmd.AddValue("b", "Burstiness", b);
	cmd.Parse(argc, argv);

	// update tOff and D as functions of b
	tOff = tOn * (b - 1);
	D = b * (80000000 * (1 - percentCBR));

	nbBlocks = simDuration / blockSize;
	Time::SetResolution(Time::NS);

	// Nodes
	NodeContainer nodes, n0n2, n1n2, n2n3;
	nodes.Create(4);
	n0n2.Add(nodes.Get(0));
	n0n2.Add(nodes.Get(2));
	n1n2.Add(nodes.Get(1));
	n1n2.Add(nodes.Get(2));
	n2n3.Add(nodes.Get(2));
	n2n3.Add(nodes.Get(3));

	// Links
	PointToPointHelper pointToPoint;
	pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
	pointToPoint.SetDeviceAttribute("DataRate", StringValue("1000000Mbps"));
	pointToPoint.SetChannelAttribute("Delay", StringValue("0.000000ms"));
	NetDeviceContainer devices02;
	devices02 = pointToPoint.Install(n0n2);

	pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
	pointToPoint.SetDeviceAttribute("DataRate", StringValue("1000000Mbps"));
	pointToPoint.SetChannelAttribute("Delay", StringValue("0.000000ms"));
	NetDeviceContainer devices12;
	devices12 = pointToPoint.Install(n1n2);

	pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
	pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
	pointToPoint.SetChannelAttribute("Delay", StringValue("0.000000ms"));
	NetDeviceContainer devices23;
	devices23 = pointToPoint.Install(n2n3);

	// IP
	InternetStackHelper stack;
	stack.Install(nodes);

	TrafficControlHelper n0;
	n0.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", StringValue("100000000p"));
	QueueDiscContainer qd0 = n0.Install(devices02);
	TrafficControlHelper n1;
	n1.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", StringValue("100000000p"));
	QueueDiscContainer qd1 = n1.Install(devices12);
	TrafficControlHelper n2;
	n2.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", StringValue("100000000p"));
	QueueDiscContainer qd2 = n2.Install(devices23);

	Ipv4AddressHelper address02, address12, address23;
	address02.SetBase("10.1.0.0", "255.255.255.0");
	address12.SetBase("10.1.1.0", "255.255.255.0");
	address23.SetBase("10.1.2.0", "255.255.255.0");
	Ipv4InterfaceContainer inter02 = address02.Assign(devices02);
	Ipv4InterfaceContainer inter12 = address12.Assign(devices12);
	Ipv4InterfaceContainer inter23 = address23.Assign(devices23);

	// Flow monitor
	flowmonitor = flowmon.InstallAll();

	// ON/OFF
	uint16_t portOnOff = 50000;
	Address localAddressOnOff(InetSocketAddress(Ipv4Address::GetAny(), portOnOff));
	PacketSinkHelper packetSinkHelperOnOff("ns3::UdpSocketFactory", localAddressOnOff);
	ApplicationContainer serverAppOnOff = packetSinkHelperOnOff.Install(nodes.Get(3));
	serverAppOnOff.Start(Seconds(0.0));
	serverAppOnOff.Stop(Seconds(simDuration + 1.0));

	OnOffHelper onoffSource("ns3::UdpSocketFactory", InetSocketAddress(inter02.GetAddress(0), portOnOff));
	std::ostringstream strs1;
	strs1 << tOn;
	std::string paramOnTime = "ns3::ExponentialRandomVariable[Mean=" + strs1.str() + "]";
	onoffSource.SetAttribute("OnTime", StringValue(paramOnTime));
	std::ostringstream strs2;
	strs2 << tOff;
	std::string paramOffTime = "ns3::ExponentialRandomVariable[Mean=" + strs2.str() + "]";
	onoffSource.SetAttribute("OffTime", StringValue(paramOffTime));
	onoffSource.SetAttribute("DataRate", DataRateValue(D));
	onoffSource.SetAttribute("PacketSize", UintegerValue(1000));
	AddressValue remoteAddressOnOff(InetSocketAddress(inter23.GetAddress(1), portOnOff));
	onoffSource.SetAttribute("Remote", remoteAddressOnOff);
	ApplicationContainer clientAppOnOff = onoffSource.Install(nodes.Get(0));
	clientAppOnOff.Start(Seconds(1.0));
	clientAppOnOff.Stop(Seconds(simDuration));

	// CBR
	uint16_t portCBR = 50001;
	Address localAddressCBR(InetSocketAddress(Ipv4Address::GetAny(), portCBR));
	PacketSinkHelper packetSinkHelperCBR("ns3::UdpSocketFactory", localAddressCBR);
	ApplicationContainer serverAppCBR = packetSinkHelperCBR.Install(nodes.Get(3));
	serverAppCBR.Start(Seconds(0.0));
	serverAppCBR.Stop(Seconds(simDuration + 1.0));

	OnOffHelper cbrSource("ns3::UdpSocketFactory", InetSocketAddress(inter12.GetAddress(0), portCBR));
	cbrSource.SetAttribute("PacketSize", UintegerValue(1000));
	std::ostringstream strs3;
	strs3 << 80 * percentCBR << "Mbps";
	std::string paramCBRrate = strs3.str();
	cbrSource.SetConstantRate(DataRate(paramCBRrate)); //bit/s
	AddressValue remoteAddressCBR(InetSocketAddress(inter23.GetAddress(1), portCBR));
	cbrSource.SetAttribute("Remote", remoteAddressCBR);
	ApplicationContainer clientAppCBR = cbrSource.Install(nodes.Get(1));
	clientAppCBR.Start(Seconds(1.0));
	clientAppCBR.Stop(Seconds(simDuration));

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	Simulator::Stop(Seconds(simDuration));
	Simulator::Schedule(Seconds(blockSize), &intConf);
	Simulator::Run();

	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
	FlowMonitor::FlowStatsContainer stats = flowmonitor->GetFlowStats();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i)
	{
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
		std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ") ";
		std::cout << " Tx Packets: " << i->second.txPackets << " ";
		std::cout << " Mean delay: " << i->second.delaySum.GetSeconds() / i->second.rxPackets << " ";
		if (i->first == 1)
			respTime0 = i->second.delaySum.GetSeconds() / i->second.rxPackets;
		if (i->first == 2)
			respTime1 = i->second.delaySum.GetSeconds() / i->second.rxPackets;
	}

	Simulator::Destroy();

	std::cout << std::endl;
	std::cout << "b: " << b;

	double T = simDuration;
	double t = simDuration / nbBlocks;

	double variance1 = (1 / nbBlocks) * (x1sum2) - ((1 / nbBlocks) * x1sum1) * ((1 / nbBlocks) * x1sum1);
	double stdDeviation1 = sqrt(variance1);
	double epsilon1 = 4.5 * stdDeviation1;
	double confidenceInterval1 = sqrt(t / T) * epsilon1;
	double cIx1sum1 = (confidenceInterval1 * nbBlocks) / (x1sum1);
	double rT1 = (x1sumRespTime / x1pcktCnt) * cIx1sum1;
	std::cout << " RT1: " << rT1;
	std::cout << " CI ON/OFF: " << confidenceInterval1;

	double variance2 = (1 / nbBlocks) * (x2sum2) - ((1 / nbBlocks) * x2sum1) * ((1 / nbBlocks) * x2sum1);
	double stdDeviation2 = sqrt(variance2);
	double epsilon2 = 4.5 * stdDeviation2;
	double confidenceInterval2 = sqrt(t / T) * epsilon2;
	double cIx2sum1 = (confidenceInterval2 * nbBlocks) / (x2sum1);
	double rT2 = (x2sumRespTime / x2pcktCnt) * cIx2sum1;
	std::cout << " RT2: " << rT2;
	std::cout << " CI CBR: " << confidenceInterval2 << std::endl;

	return 0;
}