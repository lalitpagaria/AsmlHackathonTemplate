//
//! \file
//
// Created by Sander van Woensel / Umut Uyumaz
// Copyright (c) 2018 ASML Netherlands B.V. All rights reserved.
//
//! Mesh Network wrapper class to provide a container to add specific
//! mesh network behaviour.

#include "Facilities_MeshNetwork.hpp"

#include "Debug.hpp"
#include "painlessMesh.h"
#include <DNSServer.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"

#define DNS_PORT 53

AsyncWebServer server(80);
DNSServer dnsserver;

namespace Facilities {

const uint16_t MeshNetwork::PORT = 5555;

//! Construct only.
//! \note Does not construct and initialize in one go to be able to initialize after serial debug port has been opened.
MeshNetwork::MeshNetwork()
{
    curIp = "0.0.0.0";
   m_mesh.onReceive(std::bind(&MeshNetwork::receivedCb, this, std::placeholders::_1, std::placeholders::_2));
   //m_mesh.onNewConnection(...);
  m_mesh.onChangedConnections(std::bind(&MeshNetwork::changedCb,this));
   //m_mesh.onNodeTimeAdjusted(...);
}

// Initialize mesh network.
void MeshNetwork::initialize(const __FlashStringHelper *prefix, const __FlashStringHelper *password, Scheduler& taskScheduler)
{

    
    dnsserver.setTTL(300);
    dnsserver.setErrorReplyCode(DNSReplyCode::ServerFailure);
    
   // Set debug messages before init() so that you can see startup messages.
   m_mesh.setDebugMsgTypes( ERROR | STARTUP );  // To enable all: ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE

    m_mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages
    m_mesh.init( prefix, password, PORT, WIFI_AP_STA, 6 );
    m_mesh.stationManual(STATION_SSID, STATION_PASSWORD);
    m_mesh.setHostname(HOSTNAME);
    IPAddress myAPIP(0,0,0,0);
    myAPIP = IPAddress(m_mesh.getAPIP());

    server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request){
        request->send(200, "text/html", "<form>Text to Broadcast<br><input type='text' name='BROADCAST'><br><br><input type='submit' value='Submit'></form>");
        if (request->hasArg("BROADCAST")){
            String msg = request->arg("BROADCAST");
            MY_DEBUG_PRINTLN("arg: " + msg);
            // m_mesh.sendBroadcast("REQST " + msg);
        }
    });
    server.begin();
}

//! Update mesh; forward call to painlessMesh.
void MeshNetwork::update()
{
    m_mesh.update();
    IPAddress newIp = m_mesh.getStationIP();
    String strIp = newIp.toString();
    if (strIp != curIp) {
        MY_DEBUG_PRINTLN(strIp);
        curIp = strIp;
        if (curIp != F("0.0.0.0")) {
            dnsserver.stop();
        }
        dnsserver.start(DNS_PORT, "www.webserver.com", newIp);
    }
}

void MeshNetwork::sendBroadcast(String &message)
{
//   MY_DEBUG_PRINT("Broadcasting message: "); MY_DEBUG_PRINTLN(message);
   m_mesh.sendBroadcast(message, false); // false: Do not include self.
}

MeshNetwork::NodeId MeshNetwork::getMyNodeId()
{
   return m_mesh.getNodeId();
}

std::list<MeshNetwork::NodeId> MeshNetwork::getNodeList() {
    return m_mesh.getNodeList();
}

std::vector<MeshNetwork::NodeId> MeshNetwork::getSortedNodeVector() {
    std::list<NodeId> node_list = getNodeList();
    node_list.sort();
    return std::vector<NodeId>(node_list.begin(), node_list.end());
}

void MeshNetwork::onReceive(receivedCallback_t receivedCallback)
{
   m_mesh.onReceive(receivedCallback);
}

void MeshNetwork::receivedCb(NodeId transmitterNodeId, String& msg)
{
   MY_DEBUG_PRINTF("Data received from node: %u; msg: %s\n", transmitterNodeId, msg.c_str());
}


void MeshNetwork::changedCb()
{
    MY_DEBUG_PRINTF("Connections Changed");
   MeshNetwork::nodes_present = MeshNetwork::getSortedNodeVector();
   
}

} // namespace Facilities
