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

package inet.angrydudu.IRN;
//
// TODO documentation
//
//import inet.common.queue.IOutputQueue;

//
// Drop-tail queue, to be used in network interfaces.
// Conforms to the ~IOutputQueue interface.
//
simple irnCore
{
    parameters:
        int cacheCapacity = default(1000);
        bool activate = default(false);
 		double linkspeed = default(1000000000);
        int BDPpcks = default(60);
        int BDP_N = default(3);
        string interfaceTableModule;   // The path to the InterfaceTable module
        string crcMode @enum("disabled", "declared", "computed") = default("declared");
        
        @display("i=block/downarrow,grey80;is=s;i2=status/down");
        @signal[queueLength](type=long);
        @statistic[queueLength](title="queue length"; record=max,timeavg,vector; interpolationmode=sample-hold);


    gates:
        input upperIn @labels(UdpHeader,Ipv4ControlInfo/down);
        input lowerIn @labels(UdpHeader,Ipv4ControlInfo/up,Ipv6ControlInfo/up);
        output upperOut @labels(UdpHeader,Ipv4ControlInfo/up);
        output lowerOut @labels(UdpHeader,Ipv4ControlInfo/down,Ipv6ControlInfo/down);
}


