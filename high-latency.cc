
#include <iostream>
#include <fstream>

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("HighLatencyTcpVsUdp");

int main(int argc, char* argv[])
{
    double simulationTime = 20.0;
    uint32_t payloadSize = 1024;

    NodeContainer nodes;
    nodes.Create(4);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("100ms"));

    NetDeviceContainer devices01t = pointToPoint.Install(nodes.Get(0), nodes.Get(1));
    NetDeviceContainer devices12t = pointToPoint.Install(nodes.Get(1), nodes.Get(2));
    NetDeviceContainer devices31u = pointToPoint.Install(nodes.Get(3), nodes.Get(1));
    //NetDeviceContainer devices12u = pointToPoint.Install(nodes.Get(1), nodes.Get(2));

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces01t = address.Assign(devices01t);
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces12t = address.Assign(devices12t);
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces31u = address.Assign(devices31u);
    //address.SetBase("10.1.4.0", "255.255.255.0");
    //Ipv4InterfaceContainer interfaces12u = address.Assign(devices12u);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t tcpPort = 7;
    uint16_t udpPort = 9;

    PacketSinkHelper tcpSinkHelper(
        "ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), tcpPort));
    ApplicationContainer tcpSinkApp = tcpSinkHelper.Install(nodes.Get(2));
    tcpSinkApp.Start(Seconds(0.0));
    tcpSinkApp.Stop(Seconds(simulationTime));

    PacketSinkHelper udpSinkHelper(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), udpPort));
    ApplicationContainer udpSinkApp = udpSinkHelper.Install(nodes.Get(2));
    udpSinkApp.Start(Seconds(0.0));
    udpSinkApp.Stop(Seconds(simulationTime));

    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));

    OnOffHelper tcpSender("ns3::TcpSocketFactory", Ipv4Address::GetAny());
    tcpSender.SetAttribute("OnTime",
                           StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    tcpSender.SetAttribute("OffTime",
                           StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    tcpSender.SetAttribute("PacketSize", UintegerValue(payloadSize));
    tcpSender.SetAttribute("DataRate", StringValue("1.1Mbps"));
    tcpSender.SetAttribute("MaxBytes", UintegerValue(0));

    InetSocketAddress tcpRemote(interfaces12t.GetAddress(1), tcpPort);
    tcpSender.SetAttribute("Remote", AddressValue(tcpRemote));

    ApplicationContainer tcpApps = tcpSender.Install(nodes.Get(0));
    tcpApps.Start(Seconds(1.0));
    tcpApps.Stop(Seconds(simulationTime));

    OnOffHelper udpSender("ns3::UdpSocketFactory", Ipv4Address::GetAny());
    udpSender.SetAttribute("OnTime",
                           StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    udpSender.SetAttribute("OffTime",
                           StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    udpSender.SetAttribute("PacketSize", UintegerValue(payloadSize));
    udpSender.SetAttribute("DataRate", StringValue("1.1Mbps"));
    udpSender.SetAttribute("MaxBytes", UintegerValue(0));

    InetSocketAddress udpRemote(interfaces12t.GetAddress(1), udpPort);
    udpSender.SetAttribute("Remote", AddressValue(udpRemote));

    ApplicationContainer udpApps = udpSender.Install(nodes.Get(3));
    udpApps.Start(Seconds(1.0));
    udpApps.Stop(Seconds(simulationTime));

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    std::ofstream csvFile;
    csvFile.open("high-latency-output.csv");
    csvFile << "Protocol,SourceAddress,DestinationAddress,SourcePort,DestinationPort,"
               "TxPackets,RxPackets,LostPackets,TxBytes,RxBytes,ThroughputKbps,"
               "MeanDelayMs,MeanJitterMs\n";

    for (const auto& entry : stats)
    {
        FlowId flowId = entry.first;
        FlowMonitor::FlowStats flowStats = entry.second;
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flowId);

        std::string protocolName = "OTHER";

        if (t.destinationPort == tcpPort)
        {
            protocolName = "TCP";
        }
        else if (t.destinationPort == udpPort)
        {
            protocolName = "UDP";
        }
        else
        {
            continue;
        }

        double throughputKbps = 0.0;
        if (flowStats.timeLastRxPacket.GetSeconds() > flowStats.timeFirstTxPacket.GetSeconds())
        {
            throughputKbps =
                (flowStats.rxBytes * 8.0) /
                (flowStats.timeLastRxPacket.GetSeconds() -
                 flowStats.timeFirstTxPacket.GetSeconds()) /
                1024.0;
        }

        double meanDelayMs = 0.0;
        if (flowStats.rxPackets > 0)
        {
            meanDelayMs =
                (flowStats.delaySum.GetSeconds() / flowStats.rxPackets) * 1000.0;
        }

        double meanJitterMs = 0.0;
        if (flowStats.rxPackets > 1)
        {
            meanJitterMs =
                (flowStats.jitterSum.GetSeconds() / (flowStats.rxPackets - 1)) * 1000.0;
        }


	if (throughputKbps != 0)
        csvFile << protocolName << ","
                << t.sourceAddress << ","
                << t.destinationAddress << ","
                << t.sourcePort << ","
                << t.destinationPort << ","
                << flowStats.txPackets << ","
                << flowStats.rxPackets << ","
                << flowStats.lostPackets << ","
                << flowStats.txBytes << ","
                << flowStats.rxBytes << ","
                << throughputKbps << ","
                << meanDelayMs << ","
                << meanJitterMs << "\n";
    }

    csvFile.close();
    Simulator::Destroy();
    return 0;
}
