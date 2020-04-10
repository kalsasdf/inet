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
import inet.homa.transport.ITransportScheme;

//
// Defines the skeleton of a minimal near optimal priority based transport
// scheme. The metrics of optimality is minimizing the average flow completion
// time for the message in the datacenter.
//
simple PseudoIdealPriorityTransport like ITransportScheme
{
    parameters:
        xml transportConfig = default(xml("<param/>"));
        int localPort = default(-1);
        int destPort;
        int nicLinkSpeed @unit(Gbps) = default(10Gbps);

        //@signal[msgsLeftToSend](type=long);
        //@signal[bytesLeftToSend](type=long);
        //@statistic[msgsLeftToSend](title="num msgs left to send"; source=msgsLeftToSend; record=stats, histogram; interpolationmode=none);
        //@statistic[bytesLeftToSend](title="num total bytes left to send"; source=bytesLeftToSend; record=stats, histogram; interpolationmode=none);

        @signal[msgsLeftToSend](type=long);
        @signal[stabilitySignal](type=unsigned long);
        @signal[bytesLeftToSend](type=long);
        @signal[totalOutstandingBytes](type=long);
        @signal[totalOutstandingBytes](type=long);
        @singal[homaPktPrio*Signal](type=HomaPkt);
        @signal[rxActiveTime](type=simtime_t);
        @signal[higherRxSelfWasteTime](type=simtime_t);
        @signal[lowerRxSelfWasteTime](type=simtime_t);
        @signal[rxActiveBytes](type=long);
        @signal[oversubscriptionTime](type=simtime_t);
        @signal[oversubscriptionBytes](type=long);
        @signal[sxActiveTime](type=simtime_t);
        @signal[sxActiveBytes](type=long);
        @signal[sxSchedPktDelay](type=simtime_t);
        @signal[sxUnschedPktDelay](type=simtime_t);
        @signal[activeSchedMesgs];

        @statistic[msgsLeftToSend](title="num msgs left to send"; source=msgsLeftToSend; record=stats, histogram; interpolationmode=none);
        @statistic[bytesLeftToSend](title="num total bytes left to send"; source=bytesLeftToSend; record=stats, histogram; interpolationmode=none);
        @statistic[bytesNeedGrant](title="num total sched bytes left to send"; source=bytesNeedGrant; record=stats, histogram; interpolationmode=none);
        @statistic[totalOutstandingBytes](title="total bytes in flight"; source=totalOutstandingBytes; record=stats, histogram; interpolationmode=none);
        @statistic[outstandingGrantBytes](title="num total bytes left to send"; source=outstandingGrantBytes; record=stats, histogram; interpolationmode=none);
        @statisticTemplate[pktPrioStats](title="Per pkt priority statistics"; record=stats(homaMsgSize), stats(homaPktBytes), stats(homaUnschedPktBytes), stats(homaGrantPktBytes); interpolationmode=none);
        @statistic[rxActiveTime](title="Time periods receiver expects to get bytes"; source=rxActiveTime; record=stats, histogram; interpolationmode=none);
        @statistic[rxActiveBytes](title="Bytes received during each active period"; source=rxActiveBytes; record=stats, histogram; interpolationmode=none);
        @statistic[oversubscriptionTime](title="Time periods receiver has more senders than it grants"; source=oversubscriptionTime; record=stats, histogram; interpolationmode=none);
        @statistic[oversubscriptionBytes](title="Bytes received during each oversubscription time"; source=oversubscriptionBytes; record=stats, histogram; interpolationmode=none);
        @statistic[sxActiveTime](title="Time periods sender has bytes to send"; source=sxActiveTime; record=stats, histogram; interpolationmode=none);
        @statistic[sxActiveBytes](title="Bytes sent during each active period"; source=sxActiveBytes; record=stats, histogram; interpolationmode=none);
        @statistic[sxSchedPktDelay](title="Sender delays for sched pkts"; source=sxSchedPktDelay; record=stats, histogram; interpolationmode=none);
        @statistic[sxUnschedPktDelay](title="Sender delays for unsched pkts"; source=sxUnschedPktDelay; record=stats, histogram; interpolationmode=none);

    gates:
        input upperIn[] @labels(AppMessage/down);
        input lowerIn @labels(UDPControlInfo/up);
        output upperOut[] @labels(AppMessage/up);
        output lowerOut @labels(UDPControlInfo/down);
}
