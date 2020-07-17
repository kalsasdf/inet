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

package inet.angrydudu.ec4;
//
// TODO documentation
//
//import inet.common.queue.IOutputQueue;

//
// Drop-tail queue, to be used in network interfaces.
// Conforms to the ~IOutputQueue interface.
//
simple ec4
{
    parameters:
        int frameCapacity = default(10000000);
        int bits_timeout = default(100000);
        bool activate = default(false);
 		double linkspeed @unit(Gbps) = default(1Gbps);
  	   	string crcMode @enum("declared", "computed") = default("declared");
        string interfaceTableModule;   // The path to the InterfaceTable module
        bool enableMP = default(false);
        int sumpaths = default(1);
        bool useECN = default(false);
        bool ReOrder = default(false);
        bool ratecontrol = default(false);
        int mp_algorithm = default(0);
        bool randomspreading = default(false);
        bool useTokens = default(false);
        int percdtTokens = default(1476);
        double AlphaTimer_th @unit(us) = default(55us);
 		double RateTimer_th @unit(us) = default(55us);
 		int frSteps_th = default(5);
 		int ByteCounter_th @unit(B) = default(10000B);
 		double Rai @unit(Mbps) = default(40Mbps);
 		double Rhai @unit(Mbps) = default(400Mbps);
 		double gamma = default(1/256);
        
        @display("i=block/downarrow,grey80;is=s;i2=status/down");
        @signal[queueLength](type=long);
        @statistic[queueLength](title="queue length"; record=max,timeavg,vector; interpolationmode=sample-hold);


    gates:
        input upperIn @labels(TCPSegment,IPv4ControlInfo/down,IPv6ControlInfo/down);// higher layer
        output upperOut @labels(TCPSegment,IPv4ControlInfo/up,IPv6ControlInfo/up);
        output lowerOut @labels(TCPSegment,IPv4ControlInfo/down,IPv6ControlInfo/down);
        input lowerIn @labels(TCPSegment,IPv4ControlInfo/up,IPv6ControlInfo/up);
}


