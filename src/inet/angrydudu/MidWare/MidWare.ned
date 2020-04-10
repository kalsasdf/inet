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

package inet.angrydudu.MidWare;
//
// TODO documentation
//
//import inet.common.queue.IOutputQueue;

//
// Drop-tail queue, to be used in network interfaces.
// Conforms to the ~IOutputQueue interface.
//
simple MidWare
{
    parameters:
        string crcMode @enum("declared", "computed") = default("declared");
        double linkspeed @unit(Gbps) = default(10Gbps);
        @display("i=block/table2,grey80;is=s;i2=status/down");
        @signal[BufferOccupancy](type=unsigned long);
        @statistic[BufferOccupancy](title="Buffer Occupancy (bytes)"; record=max,timeavg,vector; interpolationmode=sample-hold);
    gates:
        inout phys;
}


