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

package inet.angrydudu.MCCM.MccmCore;
//
// TODO documentation
//
//import inet.common.queue.IOutputQueue;

//
// Drop-tail queue, to be used in network interfaces.
// Conforms to the ~IOutputQueue interface.
//
simple MccmCore
{
    parameters:
        int frameCapacity = default(200);
        int bits_timeout = default(400000);
        int TenantsNumber = default(1);
        bool activate = default(false);
 		double linkspeed = default(10^9);
  	   	string crcMode @enum("declared", "computed") = default("declared");
        string interfaceTableModule;   // The path to the InterfaceTable module
        bool enableMP = default(false);
        int sumpaths = default(1);
        bool useECN = default(false);
        bool ReOrder = default(false);
        bool ratecontrol = default(false);
        int mp_algorithm = default(0);
        bool randomspreading = default(false);
        
        @display("i=block/downarrow,grey80;is=s;i2=status/down");
        @signal[queueLength](type=long);
        @statistic[queueLength](title="queue length"; record=max,timeavg,vector; interpolationmode=sample-hold);


    gates:
        input upperIn @labels(TCPSegment,IPv4ControlInfo/down,IPv6ControlInfo/down);// higher layer
        output upperOut @labels(TCPSegment,IPv4ControlInfo/up,IPv6ControlInfo/up);
        output lowerOut @labels(TCPSegment,IPv4ControlInfo/down,IPv6ControlInfo/down);
        input lowerIn @labels(TCPSegment,IPv4ControlInfo/up,IPv6ControlInfo/up);
}


