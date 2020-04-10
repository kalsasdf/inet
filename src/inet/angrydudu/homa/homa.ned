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

package inet.angrydudu.homa;
//
// TODO documentation
//
//import inet.common.queue.IOutputQueue;

//
// Drop-tail queue, to be used in network interfaces.
// Conforms to the ~IOutputQueue interface.
//
simple homa
{
    parameters:
        int frameCapacity = default(10000000);
  	   	string crcMode @enum("declared", "computed") = default("declared");
        string interfaceTableModule;   // The path to the InterfaceTable module
        
        int sumpaths = default(1);
        bool activate = default(false);
        double linkspeed @unit(Gbps) = default (1Gbps);
 		double baseRTT = default(0.000005);
        
        // If unschedPrioResolutionMode is set to EXPLICIT,
        // then this string defines the priority cutoff points of unsched
        // bytes for the remaining message sizes. Example would be "100 1500 9000"
        string explicitUnschedPrioCutoff = default("");
        
        @display("i=block/downarrow,grey80;is=s;i2=status/down");
        @signal[queueLength](type=long);
        @statistic[queueLength](title="queue length"; record=max,timeavg,vector; interpolationmode=sample-hold);


    gates:
        input upperIn @labels(UdpHeader,IPv4ControlInfo/down,IPv6ControlInfo/down);// higher layer
        output upperOut @labels(UdpHeader,IPv4ControlInfo/up,IPv6ControlInfo/up);
        output lowerOut @labels(UdpHeader,IPv4ControlInfo/down,IPv6ControlInfo/down);
        input lowerIn @labels(UdpHeader,IPv4ControlInfo/up,IPv6ControlInfo/up);
}


