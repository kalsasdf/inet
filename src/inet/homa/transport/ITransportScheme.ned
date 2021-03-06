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

package inet.homa.transport;

//
// My transport protocol interface interfaced to application layer on top and
// UDP protocol at the bottom.
//
moduleinterface ITransportScheme
{
    parameters:
        @display("i=block/transport");
        xml transportConfig;
        int nicLinkSpeed @unit(Gbps);
    gates:
        input upperIn[] @labels(AppMessage/down);
        input lowerIn @labels(UDPControlInfo/up);
        output upperOut[] @labels(AppMessage/up);
        output lowerOut @labels(UDPControlInfo/down);
}

simple TransportSchemeNone like ITransportScheme
{
    parameters:
        @display("i=old/x_noentry");
        @class("UnimplementedModule");
        xml transportConfig;
        int nicLinkSpeed @unit(Gbps);
    gates:
        input upperIn[] @labels(AppMessage/down);
        input lowerIn @labels(UDPControlInfo/up);
        output upperOut[] @labels(AppMessage/up);
        output lowerOut @labels(UDPControlInfo/down);
}
