/*
 * TcpWorkLoad.cc
 *
 *  Created on: 2019Äê1ÔÂ16ÈÕ
 *      Author: Angrydudu
 */
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "inet/applications/base/ApplicationPacket_m.h"
#include "TcpWorkLoadApp.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/chunk/ByteCountChunk.h"
#include "inet/common/packet/chunk/BytesChunk.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"

namespace inet {
Define_Module(TcpWorkloadApp);

#define MSGKIND_CONNECT    1
#define MSGKIND_SEND       2
#define MSGKIND_CLOSE      3
TcpWorkloadApp::~TcpWorkloadApp()
{
    cancelAndDelete(timeoutMsg);
}

void TcpWorkloadApp::initialize(int stage)
{
    TcpAppBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        activeOpen = par("active");
        tOpen = par("tOpen");
        tSend = par("tSend");
        tThreshold = par("tThreshold");
        tClose = par("tClose");
        sendBytes = par("sendBytes");
        SavgBytes = par("SavgBytes");
        MavgBytes = par("MavgBytes");
        LavgBytes = par("LavgBytes");
        XLavgBytes = par("XLavgBytes");
        randomSending = par("randomSending");
        sumFlows = par("sumFlows");
        SbyteJitter = par("SbyteJitter");
        MbyteJitter = par("MbyteJitter");
        LbyteJitter = par("LbyteJitter");
        XLbyteJitter = par("XLbyteJitter");
        loadS = par("loadS");
        loadM = par("loadM");
        loadL = par("loadL");
        loadXL = par("loadXL");
        workload = par("workload");
        workloadspeed = par("workloadspeed");
        byte_seed = par("byteSeed");
        byte_seed = par("loadSeed");
        time_seed = par("timeSeed");
        threshold_seed = par("thresholdSeed");
        JitterExtent = par("JitterExtent");
        commandIndex = 0;

        const char *script = par("sendScript");
        parseScript(script);

        if (randomSending)
        {
            if (sumFlows <= 0)
            {
                throw cRuntimeError("Please define at least 1 flow for random sending");
            }
            else
            {
                if (loadS+loadM+loadL+loadXL!=100)
                {
                    throw cRuntimeError("Undefined full workload!");
                }
                else
                {
                    double _loadS = double(loadS)/100.0;
                    double _loadM = double(loadM)/100.0;
                    double _loadL = double(loadL)/100.0;
                    double _loadXL = double(loadXL)/100.0;
                for (int i=0;i<=sumFlows;i++)
                {
                    if (i==0)//initialize tSend
                    {
                        srand(threshold_seed);
                        double _threshold = rand()%(int(tThreshold*1000000));
                        tSend = tOpen + _threshold/1000000;
                        //Arrive randomly from time tOpen to time tThreshold
                    }
                    srand(load_seed*i);
                    double _load = rand()%100/100;
                    srand(byte_seed*i);
                    double _byte = rand()%1000000;
                    if (_load<=_loadS)//S-load
                    {
                        sendBytes = SavgBytes - SbyteJitter + _byte/1000000*(SbyteJitter*2);
                        //Generate a (SavgBytes-SbyteJitter)to(SavgBytes+SbyteJitter) length packet
                        commands.push_back(Command(tSend,sendBytes));
                        srand(time_seed*i);
                        double timeJitter = (sendBytes/workloadspeed)/workload*JitterExtent;
                        double _time = rand()%(int(timeJitter*1000000*2));
                        tSend = tSend + (sendBytes/workloadspeed)/workload - timeJitter + _time/1000000;

                        EV<<" _byte and _time = "<<_byte<<" and "<<_time<<endl;
                    }
                    else if(_load<=_loadS+_loadM)
                    {
                        sendBytes = MavgBytes - MbyteJitter + _byte/1000000*(MbyteJitter*2);
                        //Generate a (MavgBytes-MbyteJitter)to(MavgBytes+MbyteJitter) length packet
                        commands.push_back(Command(tSend,sendBytes));
                        srand(time_seed*i);
                        double timeJitter = (sendBytes/workloadspeed)/workload*JitterExtent;
                        double _time = rand()%(int(timeJitter*1000000*2));
                        tSend = tSend + (sendBytes/workloadspeed)/workload - timeJitter + _time/1000000;
                        EV<<" _byte and _time = "<<_byte<<" and "<<_time<<endl;
                    }
                    else if(_load<=_loadS+_loadM+_loadL)
                     {
                         sendBytes = LavgBytes - LbyteJitter + _byte/1000000*(LbyteJitter*2);
                         //Generate a (LavgBytes-LbyteJitter)to(LavgBytes+LbyteJitter) length packet
                         commands.push_back(Command(tSend,sendBytes));
                         srand(time_seed*i);
                         double timeJitter = (sendBytes/workloadspeed)/workload*JitterExtent;
                         double _time = rand()%(int(timeJitter*1000000*2));
                         tSend = tSend + (sendBytes/workloadspeed)/workload - timeJitter + _time/1000000;
                         EV<<" _byte and _time = "<<_byte<<" and "<<_time<<endl;
                      }
                    else
                    {
                        sendBytes = XLavgBytes - XLbyteJitter + _byte/1000000*(XLbyteJitter*2);
                        //Generate a (XLavgBytes-XLbyteJitter)to(XLavgBytes+XLbyteJitter) length packet
                        commands.push_back(Command(tSend,sendBytes));
                        srand(time_seed*i);
                        double timeJitter = (sendBytes/workloadspeed)/workload*JitterExtent;
                        double _time = rand()%(int(timeJitter*1000000*2));
                        tSend = tSend + (sendBytes/workloadspeed)/workload - timeJitter + _time/1000000;
                        EV<<" _byte and _time = "<<_byte<<" and "<<_time<<endl;
                    }
                }
            }
        }
        }
        else
        {
            if (sendBytes > 0 && commands.size() > 0)
                throw cRuntimeError("Cannot use both sendScript and tSend+sendBytes");
            if (sendBytes > 0)
                commands.push_back(Command(tSend, sendBytes));
            if (commands.size() == 0)
                throw cRuntimeError("sendScript is empty");
        }
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        timeoutMsg = new cMessage("timer");
        nodeStatus = dynamic_cast<NodeStatus *>(findContainingNode(this)->getSubmodule("status"));

        if (isNodeUp()) {
            timeoutMsg->setKind(MSGKIND_CONNECT);
            scheduleAt(tOpen, timeoutMsg);
        }
    }
}
      bool TcpWorkloadApp::isNodeUp()
       {
       return !nodeStatus || nodeStatus->getState() == NodeStatus::UP;
       }
      bool TcpWorkloadApp::handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback)
      {
          Enter_Method_Silent();
          if (dynamic_cast<NodeStartOperation *>(operation)) {
              if (static_cast<NodeStartOperation::Stage>(stage) == NodeStartOperation::STAGE_APPLICATION_LAYER) {
                  if (simTime() <= tOpen) {
                      timeoutMsg->setKind(MSGKIND_CONNECT);
                      scheduleAt(tOpen, timeoutMsg);
                  }
              }
          }
          else if (dynamic_cast<NodeShutdownOperation *>(operation)) {
              if (static_cast<NodeShutdownOperation::Stage>(stage) == NodeShutdownOperation::STAGE_APPLICATION_LAYER) {
                  cancelEvent(timeoutMsg);
                  if (socket.getState() == TcpSocket::CONNECTED || socket.getState() == TcpSocket::CONNECTING || socket.getState() == TcpSocket::PEER_CLOSED)
                      close();
                  // TODO: wait until socket is closed
              }
          }
          else if (dynamic_cast<NodeCrashOperation *>(operation)) {
              if (static_cast<NodeCrashOperation::Stage>(stage) == NodeCrashOperation::STAGE_CRASH)
                  cancelEvent(timeoutMsg);
          }
          else
              throw cRuntimeError("Unsupported lifecycle operation '%s'", operation->getClassName());
          return true;
      }
      void TcpWorkloadApp::handleTimer(cMessage *msg)
      {
          switch (msg->getKind()) {
              case MSGKIND_CONNECT:
                  if (activeOpen)
                      connect(); // sending will be scheduled from socketEstablished()
                  else
                      ; //TODO
                  break;

              case MSGKIND_SEND:
                  sendData();
                  break;

              case MSGKIND_CLOSE:
                  close();
                  break;

              default:
                  throw cRuntimeError("Invalid timer msg: kind=%d", msg->getKind());
          }
      }
      void TcpWorkloadApp::sendData()
      {
          long numBytes = commands[commandIndex].numBytes;
          EV_INFO << "sending data with " << numBytes << " bytes\n";
          sendPacket(createDataPacket(numBytes));

          if (++commandIndex < (int)commands.size()) {
              simtime_t tSend = commands[commandIndex].tSend;
              scheduleAt(std::max(tSend, simTime()), timeoutMsg);
          }
          else {
              timeoutMsg->setKind(MSGKIND_CLOSE);
              scheduleAt(std::max(tClose, simTime()), timeoutMsg);
          }
      }
      Packet *TcpWorkloadApp::createDataPacket(long sendBytes)
      {
          Packet *packet = new Packet("data1");
          const char *dataTransferMode = par("dataTransferMode");
          Ptr<Chunk> payload;
          if (!strcmp(dataTransferMode, "bytecount")) {
              payload = makeShared<ByteCountChunk>(B(sendBytes));
          }
          else if (!strcmp(dataTransferMode, "object")) {
              const auto& applicationPacket = makeShared<ApplicationPacket>();
              applicationPacket->setChunkLength(B(sendBytes));
              payload = applicationPacket;
          }
          else if (!strcmp(dataTransferMode, "bytestream")) {
              const auto& bytesChunk = makeShared<BytesChunk>();
              std::vector<uint8_t> vec;
              vec.resize(sendBytes);
              for (int i = 0; i < sendBytes; i++)
                  vec[i] = (bytesSent + i) & 0xFF;
              bytesChunk->setBytes(vec);
              payload = bytesChunk;
          }
          else
              throw cRuntimeError("Invalid data transfer mode: %d", dataTransferMode);
          auto creationTimeTag = payload->addTag<CreationTimeTag>();
          creationTimeTag->setCreationTime(simTime());
          packet->insertAtBack(payload);
          //packet->setTimestamp(simTime());
          return packet;
      }
      void TcpWorkloadApp::socketEstablished(TcpSocket *socket)
      {
          TcpAppBase::socketEstablished(socket);

          ASSERT(commandIndex == 0);
          timeoutMsg->setKind(MSGKIND_SEND);
          simtime_t tSend = commands[commandIndex].tSend;
          scheduleAt(std::max(tSend, simTime()), timeoutMsg);
      }

      void TcpWorkloadApp::socketDataArrived(TcpSocket *socket, Packet *msg, bool urgent)
      {
          auto creationTimeTag = msg->addTagIfAbsent<CreationTimeTag>();
          EV << "socketDataArrived(), flow_transmission_time = "<< simTime() - creationTimeTag->getCreationTime()<<", creation time = "<<creationTimeTag->getCreationTime()<<endl;
          if (last_flow_creation_time != creationTimeTag->getCreationTime())
          {
              rcved_flows++;
              flow_completion_time[rcved_flows] = last_flow_time - last_flow_creation_time;
              avgFCT = (avgFCT * (rcved_flows - 1) + (last_flow_time - last_flow_creation_time))/rcved_flows;
              EV<<"socketDataArrived(), avgFCT = "<<avgFCT<<endl;
          }
          last_flow_creation_time = creationTimeTag->getCreationTime();
          last_flow_time = simTime();
          TcpAppBase::socketDataArrived(socket, msg, urgent);
      }

      void TcpWorkloadApp::socketClosed(TcpSocket *socket)
      {
          TcpAppBase::socketClosed(socket);
          cancelEvent(timeoutMsg);
      }

      void TcpWorkloadApp::socketFailure(TcpSocket *socket, int code)
      {
          TcpAppBase::socketFailure(socket, code);
          cancelEvent(timeoutMsg);
      }

      void TcpWorkloadApp::parseScript(const char *script)
      {
          const char *s = script;

          EV_DEBUG << "parse script \"" << script << "\"\n";
          while (*s) {
              // parse time
              while (isspace(*s))
                  s++;

              if (!*s || *s == ';')
                  break;

              const char *s0 = s;
              simtime_t tSend = strtod(s, &const_cast<char *&>(s));

              if (s == s0)
                  throw cRuntimeError("Syntax error in script: simulation time expected");

              // parse number of bytes
              while (isspace(*s))
                  s++;

              if (!isdigit(*s))
                  throw cRuntimeError("Syntax error in script: number of bytes expected");

              long numBytes = strtol(s, nullptr, 10);

              while (isdigit(*s))
                  s++;

              // add command
              EV_DEBUG << " add command (" << tSend << "s, " << "B)\n";
              commands.push_back(Command(tSend, numBytes));

              // skip delimiter
              while (isspace(*s))
                  s++;

              if (!*s)
                  break;

              if (*s != ';')
                  throw cRuntimeError("Syntax error in script: separator ';' missing");

              s++;

              while (isspace(*s))
                  s++;
          }
          EV_DEBUG << "parser finished\n";
      }

      void TcpWorkloadApp::finish()
      {
          EV << getFullPath() << ": received " << bytesRcvd << " bytes in " << packetsRcvd << " packets\n";
          recordScalar("bytesRcvd", bytesRcvd);
          recordScalar("bytesSent", bytesSent);
      }

      void TcpWorkloadApp::refreshDisplay() const
      {
          std::ostringstream os;
          os << TcpSocket::stateName(socket.getState()) << "\nsent: " << bytesSent << " bytes\nrcvd: " << bytesRcvd << " bytes";
          getDisplayString().setTagArg("t", 0, os.str().c_str());
      }

} /* namespace inet */





