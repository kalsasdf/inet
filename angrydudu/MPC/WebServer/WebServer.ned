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

package inet.examples.angrydudu.MPC.WebServer;
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

import inet.common.misc.ThruputMeter;
import inet.angrydudu.MPC.MpcHost.MpcHost;
import inet.angrydudu.xpass.l3Switch;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import ned.DatarateChannel;
import inet.common.misc.ThruputMeteringChannel;
import inet.node.inet.StandardHost;


//
// TODO auto-generated type
//
network webserver
{
    @display("bgb=6968.4146,3052.7407;bgl=4");
    types:
        channel leafchannel extends DatarateChannel
        {
            delay = 4us;
            datarate = 40Gbps;
        }
        channel Spinechannel extends DatarateChannel
        {
            delay = 4us;
            datarate = 100Gbps;
        }
    submodules:
        configurator: Ipv4NetworkConfigurator {
            @display("p=5534.8325,176.2981;is=s");
            config = xml("<config>"
                        + "<interface among='s1 leaf1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='s2 leaf1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='s3 leaf1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='s4 leaf1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='s5 leaf1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='s6 leaf1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='s7 leaf1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='s8 leaf1' address='192.168.1.x' netmask='255.255.255.x' />"
                        + "<interface among='s9 leaf1' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s10 leaf1' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s11 leaf2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s12 leaf2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s13 leaf2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s14 leaf2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s15 leaf2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s16 leaf2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s17 leaf2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s18 leaf2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s19 leaf2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s20 leaf2' address='192.168.2.x' netmask='255.255.255.x' />"
                        + "<interface among='s21 leaf3' address='192.168.3.x' netmask='255.255.255.x' />"
                        + "<interface among='s22 leaf3' address='192.168.3.x' netmask='255.255.255.x' />"
                        + "<interface among='s23 leaf3' address='192.168.3.x' netmask='255.255.255.x' />"
                        + "<interface among='s24 leaf3' address='192.168.3.x' netmask='255.255.255.x' />"
                        + "<interface among='s25 leaf3' address='192.168.3.x' netmask='255.255.255.x' />"
                        + "<interface among='s26 leaf3' address='192.168.3.x' netmask='255.255.255.x' />"
                        + "<interface among='s27 leaf3' address='192.168.3.x' netmask='255.255.255.x' />"
                        + "<interface among='s28 leaf3' address='192.168.3.x' netmask='255.255.255.x' />"
                        + "<interface among='s29 leaf3' address='192.168.3.x' netmask='255.255.255.x' />"
                        + "<interface among='s30 leaf3' address='192.168.3.x' netmask='255.255.255.x' />"
                        + "<interface among='s31 leaf4' address='192.168.4.x' netmask='255.255.255.x' />"
                        + "<interface among='s32 leaf4' address='192.168.4.x' netmask='255.255.255.x' />"
                        + "<interface among='s33 leaf4' address='192.168.4.x' netmask='255.255.255.x' />"
                        + "<interface among='s34 leaf4' address='192.168.4.x' netmask='255.255.255.x' />"
                        + "<interface among='s35 leaf4' address='192.168.4.x' netmask='255.255.255.x' />"
                        + "<interface among='s36 leaf4' address='192.168.4.x' netmask='255.255.255.x' />"
                        + "<interface among='s37 leaf4' address='192.168.4.x' netmask='255.255.255.x' />"
                        + "<interface among='s38 leaf4' address='192.168.4.x' netmask='255.255.255.x' />"
                        + "<interface among='s39 leaf4' address='192.168.4.x' netmask='255.255.255.x' />"
                        + "<interface among='s40 leaf4' address='192.168.4.x' netmask='255.255.255.x' />"
                        + "<interface among='s41 leaf5' address='192.168.5.x' netmask='255.255.255.x' />"
                        + "<interface among='s42 leaf5' address='192.168.5.x' netmask='255.255.255.x' />"
                        + "<interface among='s43 leaf5' address='192.168.5.x' netmask='255.255.255.x' />"
                        + "<interface among='s44 leaf5' address='192.168.5.x' netmask='255.255.255.x' />"
                        + "<interface among='s45 leaf5' address='192.168.5.x' netmask='255.255.255.x' />"
                        + "<interface among='s46 leaf5' address='192.168.5.x' netmask='255.255.255.x' />"
                        + "<interface among='s47 leaf5' address='192.168.5.x' netmask='255.255.255.x' />"
                        + "<interface among='s48 leaf5' address='192.168.5.x' netmask='255.255.255.x' />"
                        + "<interface among='s49 leaf5' address='192.168.5.x' netmask='255.255.255.x' />"
                        + "<interface among='s50 leaf5' address='192.168.5.x' netmask='255.255.255.x' />"
                        + "<interface among='s51 leaf6' address='192.168.6.x' netmask='255.255.255.x' />"
                        + "<interface among='s52 leaf6' address='192.168.6.x' netmask='255.255.255.x' />"
                        + "<interface among='s53 leaf6' address='192.168.6.x' netmask='255.255.255.x' />"
                        + "<interface among='s54 leaf6' address='192.168.6.x' netmask='255.255.255.x' />"
                        + "<interface among='s55 leaf6' address='192.168.6.x' netmask='255.255.255.x' />"
                        + "<interface among='s56 leaf6' address='192.168.6.x' netmask='255.255.255.x' />"
                        + "<interface among='s57 leaf6' address='192.168.6.x' netmask='255.255.255.x' />"
                        + "<interface among='s58 leaf6' address='192.168.6.x' netmask='255.255.255.x' />"
                        + "<interface among='s59 leaf6' address='192.168.6.x' netmask='255.255.255.x' />"
                        + "<interface among='s60 leaf6' address='192.168.6.x' netmask='255.255.255.x' />"
                        + "<interface among='s61 leaf7' address='192.168.7.x' netmask='255.255.255.x' />"
                        + "<interface among='s62 leaf7' address='192.168.7.x' netmask='255.255.255.x' />"
                        + "<interface among='s63 leaf7' address='192.168.7.x' netmask='255.255.255.x' />"
                        + "<interface among='s64 leaf7' address='192.168.7.x' netmask='255.255.255.x' />"
                        + "<interface among='s65 leaf7' address='192.168.7.x' netmask='255.255.255.x' />"
                        + "<interface among='s66 leaf7' address='192.168.7.x' netmask='255.255.255.x' />"
                        + "<interface among='s67 leaf7' address='192.168.7.x' netmask='255.255.255.x' />"
                        + "<interface among='s68 leaf7' address='192.168.7.x' netmask='255.255.255.x' />"
                        + "<interface among='s69 leaf7' address='192.168.7.x' netmask='255.255.255.x' />"
                        + "<interface among='s70 leaf7' address='192.168.7.x' netmask='255.255.255.x' />"
                        + "<interface among='s71 leaf8' address='192.168.8.x' netmask='255.255.255.x' />"
                        + "<interface among='s72 leaf8' address='192.168.8.x' netmask='255.255.255.x' />"
                        + "<interface among='s73 leaf8' address='192.168.8.x' netmask='255.255.255.x' />"
                        + "<interface among='s74 leaf8' address='192.168.8.x' netmask='255.255.255.x' />"
                        + "<interface among='s75 leaf8' address='192.168.8.x' netmask='255.255.255.x' />"
                        + "<interface among='s76 leaf8' address='192.168.8.x' netmask='255.255.255.x' />"
                        + "<interface among='s77 leaf8' address='192.168.8.x' netmask='255.255.255.x' />"
                        + "<interface among='s78 leaf8' address='192.168.8.x' netmask='255.255.255.x' />"
                        + "<interface among='s79 leaf8' address='192.168.8.x' netmask='255.255.255.x' />"
                        + "<interface among='s80 leaf8' address='192.168.8.x' netmask='255.255.255.x' />"
                        + "<interface among='s81 leaf9' address='192.168.9.x' netmask='255.255.255.x' />"
                        + "<interface among='s82 leaf9' address='192.168.9.x' netmask='255.255.255.x' />"
                        + "<interface among='s83 leaf9' address='192.168.9.x' netmask='255.255.255.x' />"
                        + "<interface among='s84 leaf9' address='192.168.9.x' netmask='255.255.255.x' />"
                        + "<interface among='s85 leaf9' address='192.168.9.x' netmask='255.255.255.x' />"
                        + "<interface among='s86 leaf9' address='192.168.9.x' netmask='255.255.255.x' />"
                        + "<interface among='s87 leaf9' address='192.168.9.x' netmask='255.255.255.x' />"
                        + "<interface among='s88 leaf9' address='192.168.9.x' netmask='255.255.255.x' />"
                        + "<interface among='s89 leaf9' address='192.168.9.x' netmask='255.255.255.x' />"
                        + "<interface among='s90 leaf9' address='192.168.9.x' netmask='255.255.255.x' />"
                        + "<interface among='s91 leaf10' address='192.168.10.x' netmask='255.255.255.x' />"
                        + "<interface among='s92 leaf10' address='192.168.10.x' netmask='255.255.255.x' />"
                        + "<interface among='s93 leaf10' address='192.168.10.x' netmask='255.255.255.x' />"
                        + "<interface among='s94 leaf10' address='192.168.10.x' netmask='255.255.255.x' />"
                        + "<interface among='s95 leaf10' address='192.168.10.x' netmask='255.255.255.x' />"
                        + "<interface among='s96 leaf10' address='192.168.10.x' netmask='255.255.255.x' />"
                        + "<interface among='s97 leaf10' address='192.168.10.x' netmask='255.255.255.x' />"
                        + "<interface among='s98 leaf10' address='192.168.10.x' netmask='255.255.255.x' />"
                        + "<interface among='s99 leaf10' address='192.168.10.x' netmask='255.255.255.x' />"
                        + "<interface among='s100 leaf10' address='192.168.10.x' netmask='255.255.255.x' />"
                        + "<interface among='s101 leaf11' address='192.168.11.x' netmask='255.255.255.x' />"
                        + "<interface among='s102 leaf11' address='192.168.11.x' netmask='255.255.255.x' />"
                        + "<interface among='s103 leaf11' address='192.168.11.x' netmask='255.255.255.x' />"
                        + "<interface among='s104 leaf11' address='192.168.11.x' netmask='255.255.255.x' />"
                        + "<interface among='s105 leaf11' address='192.168.11.x' netmask='255.255.255.x' />"
                        + "<interface among='s106 leaf11' address='192.168.11.x' netmask='255.255.255.x' />"
                        + "<interface among='s107 leaf11' address='192.168.11.x' netmask='255.255.255.x' />"
                        + "<interface among='s108 leaf11' address='192.168.11.x' netmask='255.255.255.x' />"
                        + "<interface among='s109 leaf11' address='192.168.11.x' netmask='255.255.255.x' />"
                        + "<interface among='s110 leaf11' address='192.168.11.x' netmask='255.255.255.x' />"
                        + "<interface among='s111 leaf12' address='192.168.12.x' netmask='255.255.255.x' />"
                        + "<interface among='s112 leaf12' address='192.168.12.x' netmask='255.255.255.x' />"
                        + "<interface among='s113 leaf12' address='192.168.12.x' netmask='255.255.255.x' />"
                        + "<interface among='s114 leaf12' address='192.168.12.x' netmask='255.255.255.x' />"
                        + "<interface among='s115 leaf12' address='192.168.12.x' netmask='255.255.255.x' />"
                        + "<interface among='s116 leaf12' address='192.168.12.x' netmask='255.255.255.x' />"
                        + "<interface among='s117 leaf12' address='192.168.12.x' netmask='255.255.255.x' />"
                        + "<interface among='s118 leaf12' address='192.168.12.x' netmask='255.255.255.x' />"
                        + "<interface among='s119 leaf12' address='192.168.12.x' netmask='255.255.255.x' />"
                        + "<interface among='s120 leaf12' address='192.168.12.x' netmask='255.255.255.x' />"
                        + "<interface among='s121 leaf13' address='192.168.13.x' netmask='255.255.255.x' />"
                        + "<interface among='s122 leaf13' address='192.168.13.x' netmask='255.255.255.x' />"
                        + "<interface among='s123 leaf13' address='192.168.13.x' netmask='255.255.255.x' />"
                        + "<interface among='s124 leaf13' address='192.168.13.x' netmask='255.255.255.x' />"
                        + "<interface among='s125 leaf13' address='192.168.13.x' netmask='255.255.255.x' />"
                        + "<interface among='s126 leaf13' address='192.168.13.x' netmask='255.255.255.x' />"
                        + "<interface among='s127 leaf13' address='192.168.13.x' netmask='255.255.255.x' />"
                        + "<interface among='s128 leaf13' address='192.168.13.x' netmask='255.255.255.x' />"
                        + "<interface among='s129 leaf13' address='192.168.13.x' netmask='255.255.255.x' />"
                        + "<interface among='s130 leaf13' address='192.168.13.x' netmask='255.255.255.x' />"
                        + "<interface among='s131 leaf14' address='192.168.14.x' netmask='255.255.255.x' />"
                        + "<interface among='s132 leaf14' address='192.168.14.x' netmask='255.255.255.x' />"
                        + "<interface among='s133 leaf14' address='192.168.14.x' netmask='255.255.255.x' />"
                        + "<interface among='s134 leaf14' address='192.168.14.x' netmask='255.255.255.x' />"
                        + "<interface among='s135 leaf14' address='192.168.14.x' netmask='255.255.255.x' />"
                        + "<interface among='s136 leaf14' address='192.168.14.x' netmask='255.255.255.x' />"
                        + "<interface among='s137 leaf14' address='192.168.14.x' netmask='255.255.255.x' />"
                        + "<interface among='s138 leaf14' address='192.168.14.x' netmask='255.255.255.x' />"
                        + "<interface among='s139 leaf14' address='192.168.14.x' netmask='255.255.255.x' />"
                        + "<interface among='s140 leaf14' address='192.168.14.x' netmask='255.255.255.x' />"
                        + "<interface among='s141 leaf15' address='192.168.15.x' netmask='255.255.255.x' />"
                        + "<interface among='s142 leaf15' address='192.168.15.x' netmask='255.255.255.x' />"
                        + "<interface among='s143 leaf15' address='192.168.15.x' netmask='255.255.255.x' />"
                        + "<interface among='s144 leaf15' address='192.168.15.x' netmask='255.255.255.x' />"
                        + "<interface among='s145 leaf15' address='192.168.15.x' netmask='255.255.255.x' />"
                        + "<interface among='s146 leaf15' address='192.168.15.x' netmask='255.255.255.x' />"
                        + "<interface among='s147 leaf15' address='192.168.15.x' netmask='255.255.255.x' />"
                        + "<interface among='s148 leaf15' address='192.168.15.x' netmask='255.255.255.x' />"
                        + "<interface among='s149 leaf15' address='192.168.15.x' netmask='255.255.255.x' />"
                        + "<interface among='s150 leaf15' address='192.168.15.x' netmask='255.255.255.x' />"
                        + "<interface among='s151 leaf16' address='192.168.16.x' netmask='255.255.255.x' />"
                        + "<interface among='s152 leaf16' address='192.168.16.x' netmask='255.255.255.x' />"
                        + "<interface among='s153 leaf16' address='192.168.16.x' netmask='255.255.255.x' />"
                        + "<interface among='s154 leaf16' address='192.168.16.x' netmask='255.255.255.x' />"
                        + "<interface among='s155 leaf16' address='192.168.16.x' netmask='255.255.255.x' />"
                        + "<interface among='s156 leaf16' address='192.168.16.x' netmask='255.255.255.x' />"
                        + "<interface among='s157 leaf16' address='192.168.16.x' netmask='255.255.255.x' />"
                        + "<interface among='s158 leaf16' address='192.168.16.x' netmask='255.255.255.x' />"
                        + "<interface among='s159 leaf16' address='192.168.16.x' netmask='255.255.255.x' />"
                        + "<interface among='s160 leaf16' address='192.168.16.x' netmask='255.255.255.x' />"
                        + "<interface among='s161 leaf17' address='192.168.17.x' netmask='255.255.255.x' />"
                        + "<interface among='s162 leaf17' address='192.168.17.x' netmask='255.255.255.x' />"
                        + "<interface among='s163 leaf17' address='192.168.17.x' netmask='255.255.255.x' />"
                        + "<interface among='s164 leaf17' address='192.168.17.x' netmask='255.255.255.x' />"
                        + "<interface among='s165 leaf17' address='192.168.17.x' netmask='255.255.255.x' />"
                        + "<interface among='s166 leaf17' address='192.168.17.x' netmask='255.255.255.x' />"
                        + "<interface among='s167 leaf17' address='192.168.17.x' netmask='255.255.255.x' />"
                        + "<interface among='s168 leaf17' address='192.168.17.x' netmask='255.255.255.x' />"
                        + "<interface among='s169 leaf17' address='192.168.17.x' netmask='255.255.255.x' />"
                        + "<interface among='s170 leaf17' address='192.168.17.x' netmask='255.255.255.x' />"
                        + "<interface among='s171 leaf18' address='192.168.18.x' netmask='255.255.255.x' />"
                        + "<interface among='s172 leaf18' address='192.168.18.x' netmask='255.255.255.x' />"
                        + "<interface among='s173 leaf18' address='192.168.18.x' netmask='255.255.255.x' />"
                        + "<interface among='s174 leaf18' address='192.168.18.x' netmask='255.255.255.x' />"
                        + "<interface among='s175 leaf18' address='192.168.18.x' netmask='255.255.255.x' />"
                        + "<interface among='s176 leaf18' address='192.168.18.x' netmask='255.255.255.x' />"
                        + "<interface among='s177 leaf18' address='192.168.18.x' netmask='255.255.255.x' />"
                        + "<interface among='s178 leaf18' address='192.168.18.x' netmask='255.255.255.x' />"
                        + "<interface among='s179 leaf18' address='192.168.18.x' netmask='255.255.255.x' />"
                        + "<interface among='s180 leaf18' address='192.168.18.x' netmask='255.255.255.x' />"
                        + "<interface among='s181 leaf19' address='192.168.19.x' netmask='255.255.255.x' />"
                        + "<interface among='s182 leaf19' address='192.168.19.x' netmask='255.255.255.x' />"
                        + "<interface among='s183 leaf19' address='192.168.19.x' netmask='255.255.255.x' />"
                        + "<interface among='s184 leaf19' address='192.168.19.x' netmask='255.255.255.x' />"
                        + "<interface among='s185 leaf19' address='192.168.19.x' netmask='255.255.255.x' />"
                        + "<interface among='s186 leaf19' address='192.168.19.x' netmask='255.255.255.x' />"
                        + "<interface among='s187 leaf19' address='192.168.19.x' netmask='255.255.255.x' />"
                        + "<interface among='s188 leaf19' address='192.168.19.x' netmask='255.255.255.x' />"
                        + "<interface among='s189 leaf19' address='192.168.19.x' netmask='255.255.255.x' />"
                        + "<interface among='s190 leaf19' address='192.168.19.x' netmask='255.255.255.x' />"
                        + "<interface among='s191 leaf20' address='192.168.20.x' netmask='255.255.255.x' />"
                        + "<interface among='s192 leaf20' address='192.168.20.x' netmask='255.255.255.x' />"
                        + "<interface among='s193 leaf20' address='192.168.20.x' netmask='255.255.255.x' />"
                        + "<interface among='s194 leaf20' address='192.168.20.x' netmask='255.255.255.x' />"
                        + "<interface among='s195 leaf20' address='192.168.20.x' netmask='255.255.255.x' />"
                        + "<interface among='s196 leaf20' address='192.168.20.x' netmask='255.255.255.x' />"
                        + "<interface among='s197 leaf20' address='192.168.20.x' netmask='255.255.255.x' />"
                        + "<interface among='s198 leaf20' address='192.168.20.x' netmask='255.255.255.x' />"
                        + "<interface among='s199 leaf20' address='192.168.20.x' netmask='255.255.255.x' />"
                        + "<interface among='s200 leaf20' address='192.168.20.x' netmask='255.255.255.x' />"
                        + "<interface among='s201 leaf21' address='192.168.21.x' netmask='255.255.255.x' />"
                        + "<interface among='s202 leaf21' address='192.168.21.x' netmask='255.255.255.x' />"
                        + "<interface among='s203 leaf21' address='192.168.21.x' netmask='255.255.255.x' />"
                        + "<interface among='s204 leaf21' address='192.168.21.x' netmask='255.255.255.x' />"
                        + "<interface among='s205 leaf21' address='192.168.21.x' netmask='255.255.255.x' />"
                        + "<interface among='s206 leaf21' address='192.168.21.x' netmask='255.255.255.x' />"
                        + "<interface among='s207 leaf21' address='192.168.21.x' netmask='255.255.255.x' />"
                        + "<interface among='s208 leaf21' address='192.168.21.x' netmask='255.255.255.x' />"
                        + "<interface among='s209 leaf21' address='192.168.21.x' netmask='255.255.255.x' />"
                        + "<interface among='s210 leaf21' address='192.168.21.x' netmask='255.255.255.x' />"
                        + "<interface among='s211 leaf22' address='192.168.22.x' netmask='255.255.255.x' />"
                        + "<interface among='s212 leaf22' address='192.168.22.x' netmask='255.255.255.x' />"
                        + "<interface among='s213 leaf22' address='192.168.22.x' netmask='255.255.255.x' />"
                        + "<interface among='s214 leaf22' address='192.168.22.x' netmask='255.255.255.x' />"
                        + "<interface among='s215 leaf22' address='192.168.22.x' netmask='255.255.255.x' />"
                        + "<interface among='s216 leaf22' address='192.168.22.x' netmask='255.255.255.x' />"
                        + "<interface among='s217 leaf22' address='192.168.22.x' netmask='255.255.255.x' />"
                        + "<interface among='s218 leaf22' address='192.168.22.x' netmask='255.255.255.x' />"
                        + "<interface among='s219 leaf22' address='192.168.22.x' netmask='255.255.255.x' />"
                        + "<interface among='s220 leaf22' address='192.168.22.x' netmask='255.255.255.x' />"
                        + "<interface among='s221 leaf23' address='192.168.23.x' netmask='255.255.255.x' />"
                        + "<interface among='s222 leaf23' address='192.168.23.x' netmask='255.255.255.x' />"
                        + "<interface among='s223 leaf23' address='192.168.23.x' netmask='255.255.255.x' />"
                        + "<interface among='s224 leaf23' address='192.168.23.x' netmask='255.255.255.x' />"
                        + "<interface among='s225 leaf23' address='192.168.23.x' netmask='255.255.255.x' />"
                        + "<interface among='s226 leaf23' address='192.168.23.x' netmask='255.255.255.x' />"
                        + "<interface among='s227 leaf23' address='192.168.23.x' netmask='255.255.255.x' />"
                        + "<interface among='s228 leaf23' address='192.168.23.x' netmask='255.255.255.x' />"
                        + "<interface among='s229 leaf23' address='192.168.23.x' netmask='255.255.255.x' />"
                        + "<interface among='s230 leaf23' address='192.168.23.x' netmask='255.255.255.x' />"
                        + "<interface among='s231 leaf24' address='192.168.24.x' netmask='255.255.255.x' />"
                        + "<interface among='s232 leaf24' address='192.168.24.x' netmask='255.255.255.x' />"
                        + "<interface among='s233 leaf24' address='192.168.24.x' netmask='255.255.255.x' />"
                        + "<interface among='s234 leaf24' address='192.168.24.x' netmask='255.255.255.x' />"
                        + "<interface among='s235 leaf24' address='192.168.24.x' netmask='255.255.255.x' />"
                        + "<interface among='s236 leaf24' address='192.168.24.x' netmask='255.255.255.x' />"
                        + "<interface among='s237 leaf24' address='192.168.24.x' netmask='255.255.255.x' />"
                        + "<interface among='s238 leaf24' address='192.168.24.x' netmask='255.255.255.x' />"
                        + "<interface among='s239 leaf24' address='192.168.24.x' netmask='255.255.255.x' />"
                        + "<interface among='s240 leaf24' address='192.168.24.x' netmask='255.255.255.x' />"
                        + "<interface among='Spine* leaf*' address='10.0.x.x' netmask='255.255.x.x' />"
                        + "<multicast-group hosts='leaf*' address='224.0.0.5 224.0.0.6' />"
                        + "<multicast-group hosts='Spine*' address='224.0.0.5 224.0.0.6' />"
                        + "</config>");
        }
        Spine1: l3Switch {
            @display("p=2340,1300");
            gates:
                ethg[24];
        }
        Spine2: l3Switch {
            @display("p=2780,1300");
            gates:
                ethg[24];
        }
        Spine3: l3Switch {
            @display("p=3220,1300");
            gates:
                ethg[24];
        }
        Spine4: l3Switch {
            @display("p=3660,1300");
            gates:
                ethg[24];
        }
        leaf1: l3Switch {
            @display("p=580,1700");
            gates:
                ethg[14];
        }
        leaf2: l3Switch {
            @display("p=1020,1700");
            gates:
                ethg[14];
        }
        leaf3: l3Switch {
            @display("p=1460,1700");
            gates:
                ethg[14];
        }
        leaf4: l3Switch {
            @display("p=1900,1700");
            gates:
                ethg[14];
        }
        leaf5: l3Switch {
            @display("p=2340,1700");
            gates:
                ethg[14];
        }
        leaf6: l3Switch {
            @display("p=2780,1700");
            gates:
                ethg[14];
        }
        leaf7: l3Switch {
            @display("p=3220,1700");
            gates:
                ethg[14];
        }
        leaf8: l3Switch {
            @display("p=3660,1700");
            gates:
                ethg[14];
        }
        leaf9: l3Switch {
            @display("p=4100,1700");
            gates:
                ethg[14];
        }
        leaf10: l3Switch {
            @display("p=4540,1700");
            gates:
                ethg[14];
        }
        leaf11: l3Switch {
            @display("p=4980,1700");
            gates:
                ethg[14];
        }
        leaf12: l3Switch {
            @display("p=5420,1700");
            gates:
                ethg[14];
        }
        leaf13: l3Switch {
            @display("p=580,900");
            gates:
                ethg[14];
        }
        leaf14: l3Switch {
            @display("p=1020,900");
            gates:
                ethg[14];
        }
        leaf15: l3Switch {
            @display("p=1460,900");
            gates:
                ethg[14];
        }
        leaf16: l3Switch {
            @display("p=1900,900");
            gates:
                ethg[14];
        }
        leaf17: l3Switch {
            @display("p=2340,900");
            gates:
                ethg[14];
        }
        leaf18: l3Switch {
            @display("p=2780,900");
            gates:
                ethg[14];
        }
        leaf19: l3Switch {
            @display("p=3220,900");
            gates:
                ethg[14];
        }
        leaf20: l3Switch {
            @display("p=3660,900");
            gates:
                ethg[14];
        }
        leaf21: l3Switch {
            @display("p=4100,900");
            gates:
                ethg[14];
        }
        leaf22: l3Switch {
            @display("p=4540,900");
            gates:
                ethg[14];
        }
        leaf23: l3Switch {
            @display("p=4980,900");
            gates:
                ethg[14];
        }
        leaf24: l3Switch {
            @display("p=5420,900");
            gates:
                ethg[14];
        }
        s1: MpcHost {
            @display("i=device/server;p=400,2000");
            gates:
                ethg[1];
        }
        s2: MpcHost {
            @display("i=device/server;p=440,2000");
            gates:
                ethg[1];
        }
        s3: MpcHost {
            @display("i=device/server;p=480,2000");
            gates:
                ethg[1];
        }
        s4: MpcHost {
            @display("i=device/server;p=520,2000");
            gates:
                ethg[1];
        }
        s5: MpcHost {
            @display("i=device/server;p=560,2000");
            gates:
                ethg[1];
        }
        s6: MpcHost {
            @display("i=device/server;p=600,2000");
            gates:
                ethg[1];
        }
        s7: MpcHost {
            @display("i=device/server;p=640,2000");
            gates:
                ethg[1];
        }
        s8: MpcHost {
            @display("i=device/server;p=680,2000");
            gates:
                ethg[1];
        }
        s9: MpcHost {
            @display("i=device/server;p=720,2000");
            gates:
                ethg[1];
        }
        s10: MpcHost {
            @display("i=device/server;p=760,2000");
            gates:
                ethg[1];
        }
        s11: MpcHost {
            @display("i=device/server;p=840,2000");
            gates:
                ethg[1];
        }
        s12: MpcHost {
            @display("i=device/server;p=880,2000");
            gates:
                ethg[1];
        }
        s13: MpcHost {
            @display("i=device/server;p=920,2000");
            gates:
                ethg[1];
        }
        s14: MpcHost {
            @display("i=device/server;p=960,2000");
            gates:
                ethg[1];
        }
        s15: MpcHost {
            @display("i=device/server;p=1000,2000");
            gates:
                ethg[1];
        }
        s16: MpcHost {
            @display("i=device/server;p=1040,2000");
            gates:
                ethg[1];
        }
        s17: MpcHost {
            @display("i=device/server;p=1080,2000");
            gates:
                ethg[1];
        }
        s18: MpcHost {
            @display("i=device/server;p=1120,2000");
            gates:
                ethg[1];
        }
        s19: MpcHost {
            @display("i=device/server;p=1160,2000");
            gates:
                ethg[1];
        }
        s20: MpcHost {
            @display("i=device/server;p=1200,2000");
            gates:
                ethg[1];
        }
        s21: MpcHost {
            @display("i=device/server;p=1280,2000");
            gates:
                ethg[1];
        }
        s22: MpcHost {
            @display("i=device/server;p=1320,2000");
            gates:
                ethg[1];
        }
        s23: MpcHost {
            @display("i=device/server;p=1360,2000");
            gates:
                ethg[1];
        }
        s24: MpcHost {
            @display("i=device/server;p=1400,2000");
            gates:
                ethg[1];
        }
        s25: MpcHost {
            @display("i=device/server;p=1440,2000");
            gates:
                ethg[1];
        }
        s26: MpcHost {
            @display("i=device/server;p=1480,2000");
            gates:
                ethg[1];
        }
        s27: MpcHost {
            @display("i=device/server;p=1520,2000");
            gates:
                ethg[1];
        }
        s28: MpcHost {
            @display("i=device/server;p=1560,2000");
            gates:
                ethg[1];
        }
        s29: MpcHost {
            @display("i=device/server;p=1600,2000");
            gates:
                ethg[1];
        }
        s30: MpcHost {
            @display("i=device/server;p=1640,2000");
            gates:
                ethg[1];
        }
        s31: MpcHost {
            @display("i=device/server;p=1720,2000");
            gates:
                ethg[1];
        }
        s32: MpcHost {
            @display("i=device/server;p=1760,2000");
            gates:
                ethg[1];
        }
        s33: MpcHost {
            @display("i=device/server;p=1800,2000");
            gates:
                ethg[1];
        }
        s34: MpcHost {
            @display("i=device/server;p=1840,2000");
            gates:
                ethg[1];
        }
        s35: MpcHost {
            @display("i=device/server;p=1880,2000");
            gates:
                ethg[1];
        }
        s36: MpcHost {
            @display("i=device/server;p=1920,2000");
            gates:
                ethg[1];
        }
        s37: MpcHost {
            @display("i=device/server;p=1960,2000");
            gates:
                ethg[1];
        }
        s38: MpcHost {
            @display("i=device/server;p=2000,2000");
            gates:
                ethg[1];
        }
        s39: MpcHost {
            @display("i=device/server;p=2040,2000");
            gates:
                ethg[1];
        }
        s40: MpcHost {
            @display("i=device/server;p=2080,2000");
            gates:
                ethg[1];
        }
        s41: MpcHost {
            @display("i=device/server;p=2160,2000");
            gates:
                ethg[1];
        }
        s42: MpcHost {
            @display("i=device/server;p=2200,2000");
            gates:
                ethg[1];
        }
        s43: MpcHost {
            @display("i=device/server;p=2240,2000");
            gates:
                ethg[1];
        }
        s44: MpcHost {
            @display("i=device/server;p=2280,2000");
            gates:
                ethg[1];
        }
        s45: MpcHost {
            @display("i=device/server;p=2320,2000");
            gates:
                ethg[1];
        }
        s46: MpcHost {
            @display("i=device/server;p=2360,2000");
            gates:
                ethg[1];
        }
        s47: MpcHost {
            @display("i=device/server;p=2400,2000");
            gates:
                ethg[1];
        }
        s48: MpcHost {
            @display("i=device/server;p=2440,2000");
            gates:
                ethg[1];
        }
        s49: MpcHost {
            @display("i=device/server;p=2480,2000");
            gates:
                ethg[1];
        }
        s50: MpcHost {
            @display("i=device/server;p=2520,2000");
            gates:
                ethg[1];
        }
        s51: MpcHost {
            @display("i=device/server;p=2600,2000");
            gates:
                ethg[1];
        }
        s52: MpcHost {
            @display("i=device/server;p=2640,2000");
            gates:
                ethg[1];
        }
        s53: MpcHost {
            @display("i=device/server;p=2680,2000");
            gates:
                ethg[1];
        }
        s54: MpcHost {
            @display("i=device/server;p=2720,2000");
            gates:
                ethg[1];
        }
        s55: MpcHost {
            @display("i=device/server;p=2760,2000");
            gates:
                ethg[1];
        }
        s56: MpcHost {
            @display("i=device/server;p=2800,2000");
            gates:
                ethg[1];
        }
        s57: MpcHost {
            @display("i=device/server;p=2840,2000");
            gates:
                ethg[1];
        }
        s58: MpcHost {
            @display("i=device/server;p=2880,2000");
            gates:
                ethg[1];
        }
        s59: MpcHost {
            @display("i=device/server;p=2920,2000");
            gates:
                ethg[1];
        }
        s60: MpcHost {
            @display("i=device/server;p=2960,2000");
            gates:
                ethg[1];
        }
        s61: MpcHost {
            @display("i=device/server;p=3040,2000");
            gates:
                ethg[1];
        }
        s62: MpcHost {
            @display("i=device/server;p=3080,2000");
            gates:
                ethg[1];
        }
        s63: MpcHost {
            @display("i=device/server;p=3120,2000");
            gates:
                ethg[1];
        }
        s64: MpcHost {
            @display("i=device/server;p=3160,2000");
            gates:
                ethg[1];
        }
        s65: MpcHost {
            @display("i=device/server;p=3200,2000");
            gates:
                ethg[1];
        }
        s66: MpcHost {
            @display("i=device/server;p=3240,2000");
            gates:
                ethg[1];
        }
        s67: MpcHost {
            @display("i=device/server;p=3280,2000");
            gates:
                ethg[1];
        }
        s68: MpcHost {
            @display("i=device/server;p=3320,2000");
            gates:
                ethg[1];
        }
        s69: MpcHost {
            @display("i=device/server;p=3360,2000");
            gates:
                ethg[1];
        }
        s70: MpcHost {
            @display("i=device/server;p=3400,2000");
            gates:
                ethg[1];
        }
        s71: MpcHost {
            @display("i=device/server;p=3480,2000");
            gates:
                ethg[1];
        }
        s72: MpcHost {
            @display("i=device/server;p=3520,2000");
            gates:
                ethg[1];
        }
        s73: MpcHost {
            @display("i=device/server;p=3560,2000");
            gates:
                ethg[1];
        }
        s74: MpcHost {
            @display("i=device/server;p=3600,2000");
            gates:
                ethg[1];
        }
        s75: MpcHost {
            @display("i=device/server;p=3640,2000");
            gates:
                ethg[1];
        }
        s76: MpcHost {
            @display("i=device/server;p=3680,2000");
            gates:
                ethg[1];
        }
        s77: MpcHost {
            @display("i=device/server;p=3720,2000");
            gates:
                ethg[1];
        }
        s78: MpcHost {
            @display("i=device/server;p=3760,2000");
            gates:
                ethg[1];
        }
        s79: MpcHost {
            @display("i=device/server;p=3800,2000");
            gates:
                ethg[1];
        }
        s80: MpcHost {
            @display("i=device/server;p=3840,2000");
            gates:
                ethg[1];
        }
        s81: MpcHost {
            @display("i=device/server;p=3920,2000");
            gates:
                ethg[1];
        }
        s82: MpcHost {
            @display("i=device/server;p=3960,2000");
            gates:
                ethg[1];
        }
        s83: MpcHost {
            @display("i=device/server;p=4000,2000");
            gates:
                ethg[1];
        }
        s84: MpcHost {
            @display("i=device/server;p=4040,2000");
            gates:
                ethg[1];
        }
        s85: MpcHost {
            @display("i=device/server;p=4080,2000");
            gates:
                ethg[1];
        }
        s86: MpcHost {
            @display("i=device/server;p=4120,2000");
            gates:
                ethg[1];
        }
        s87: MpcHost {
            @display("i=device/server;p=4160,2000");
            gates:
                ethg[1];
        }
        s88: MpcHost {
            @display("i=device/server;p=4200,2000");
            gates:
                ethg[1];
        }
        s89: MpcHost {
            @display("i=device/server;p=4240,2000");
            gates:
                ethg[1];
        }
        s90: MpcHost {
            @display("i=device/server;p=4280,2000");
            gates:
                ethg[1];
        }
        s91: MpcHost {
            @display("i=device/server;p=4360,2000");
            gates:
                ethg[1];
        }
        s92: MpcHost {
            @display("i=device/server;p=4400,2000");
            gates:
                ethg[1];
        }
        s93: MpcHost {
            @display("i=device/server;p=4440,2000");
            gates:
                ethg[1];
        }
        s94: MpcHost {
            @display("i=device/server;p=4480,2000");
            gates:
                ethg[1];
        }
        s95: MpcHost {
            @display("i=device/server;p=4520,2000");
            gates:
                ethg[1];
        }
        s96: MpcHost {
            @display("i=device/server;p=4560,2000");
            gates:
                ethg[1];
        }
        s97: MpcHost {
            @display("i=device/server;p=4600,2000");
            gates:
                ethg[1];
        }
        s98: MpcHost {
            @display("i=device/server;p=4640,2000");
            gates:
                ethg[1];
        }
        s99: MpcHost {
            @display("i=device/server;p=4680,2000");
            gates:
                ethg[1];
        }
        s100: MpcHost {
            @display("i=device/server;p=4720,2000");
            gates:
                ethg[1];
        }
        s101: MpcHost {
            @display("i=device/server;p=4800,2000");
            gates:
                ethg[1];
        }
        s102: MpcHost {
            @display("i=device/server;p=4840,2000");
            gates:
                ethg[1];
        }
        s103: MpcHost {
            @display("i=device/server;p=4880,2000");
            gates:
                ethg[1];
        }
        s104: MpcHost {
            @display("i=device/server;p=4920,2000");
            gates:
                ethg[1];
        }
        s105: MpcHost {
            @display("i=device/server;p=4960,2000");
            gates:
                ethg[1];
        }
        s106: MpcHost {
            @display("i=device/server;p=5000,2000");
            gates:
                ethg[1];
        }
        s107: MpcHost {
            @display("i=device/server;p=5040,2000");
            gates:
                ethg[1];
        }
        s108: MpcHost {
            @display("i=device/server;p=5080,2000");
            gates:
                ethg[1];
        }
        s109: MpcHost {
            @display("i=device/server;p=5120,2000");
            gates:
                ethg[1];
        }
        s110: MpcHost {
            @display("i=device/server;p=5160,2000");
            gates:
                ethg[1];
        }
        s111: MpcHost {
            @display("i=device/server;p=5240,2000");
            gates:
                ethg[1];
        }
        s112: MpcHost {
            @display("i=device/server;p=5280,2000");
            gates:
                ethg[1];
        }
        s113: MpcHost {
            @display("i=device/server;p=5320,2000");
            gates:
                ethg[1];
        }
        s114: MpcHost {
            @display("i=device/server;p=5360,2000");
            gates:
                ethg[1];
        }
        s115: MpcHost {
            @display("i=device/server;p=5400,2000");
            gates:
                ethg[1];
        }
        s116: MpcHost {
            @display("i=device/server;p=5440,2000");
            gates:
                ethg[1];
        }
        s117: MpcHost {
            @display("i=device/server;p=5480,2000");
            gates:
                ethg[1];
        }
        s118: MpcHost {
            @display("i=device/server;p=5520,2000");
            gates:
                ethg[1];
        }
        s119: MpcHost {
            @display("i=device/server;p=5560,2000");
            gates:
                ethg[1];
        }
        s120: MpcHost {
            @display("i=device/server;p=5600,2000");
            gates:
                ethg[1];
        }
        s121: MpcHost {
            @display("i=device/server;p=400,600");
            gates:
                ethg[1];
        }
        s122: MpcHost {
            @display("i=device/server;p=440,600");
            gates:
                ethg[1];
        }
        s123: MpcHost {
            @display("i=device/server;p=480,600");
            gates:
                ethg[1];
        }
        s124: MpcHost {
            @display("i=device/server;p=520,600");
            gates:
                ethg[1];
        }
        s125: MpcHost {
            @display("i=device/server;p=560,600");
            gates:
                ethg[1];
        }
        s126: MpcHost {
            @display("i=device/server;p=600,600");
            gates:
                ethg[1];
        }
        s127: MpcHost {
            @display("i=device/server;p=640,600");
            gates:
                ethg[1];
        }
        s128: MpcHost {
            @display("i=device/server;p=680,600");
            gates:
                ethg[1];
        }
        s129: MpcHost {
            @display("i=device/server;p=720,600");
            gates:
                ethg[1];
        }
        s130: MpcHost {
            @display("i=device/server;p=760,600");
            gates:
                ethg[1];
        }
        s131: MpcHost {
            @display("i=device/server;p=840,600");
            gates:
                ethg[1];
        }
        s132: MpcHost {
            @display("i=device/server;p=880,600");
            gates:
                ethg[1];
        }
        s133: MpcHost {
            @display("i=device/server;p=920,600");
            gates:
                ethg[1];
        }
        s134: MpcHost {
            @display("i=device/server;p=960,600");
            gates:
                ethg[1];
        }
        s135: MpcHost {
            @display("i=device/server;p=1000,600");
            gates:
                ethg[1];
        }
        s136: MpcHost {
            @display("i=device/server;p=1040,600");
            gates:
                ethg[1];
        }
        s137: MpcHost {
            @display("i=device/server;p=1080,600");
            gates:
                ethg[1];
        }
        s138: MpcHost {
            @display("i=device/server;p=1120,600");
            gates:
                ethg[1];
        }
        s139: MpcHost {
            @display("i=device/server;p=1160,600");
            gates:
                ethg[1];
        }
        s140: MpcHost {
            @display("i=device/server;p=1200,600");
            gates:
                ethg[1];
        }
        s141: MpcHost {
            @display("i=device/server;p=1280,600");
            gates:
                ethg[1];
        }
        s142: MpcHost {
            @display("i=device/server;p=1320,600");
            gates:
                ethg[1];
        }
        s143: MpcHost {
            @display("i=device/server;p=1360,600");
            gates:
                ethg[1];
        }
        s144: MpcHost {
            @display("i=device/server;p=1400,600");
            gates:
                ethg[1];
        }
        s145: MpcHost {
            @display("i=device/server;p=1440,600");
            gates:
                ethg[1];
        }
        s146: MpcHost {
            @display("i=device/server;p=1480,600");
            gates:
                ethg[1];
        }
        s147: MpcHost {
            @display("i=device/server;p=1520,600");
            gates:
                ethg[1];
        }
        s148: MpcHost {
            @display("i=device/server;p=1560,600");
            gates:
                ethg[1];
        }
        s149: MpcHost {
            @display("i=device/server;p=1600,600");
            gates:
                ethg[1];
        }
        s150: MpcHost {
            @display("i=device/server;p=1640,600");
            gates:
                ethg[1];
        }
        s151: MpcHost {
            @display("i=device/server;p=1720,600");
            gates:
                ethg[1];
        }
        s152: MpcHost {
            @display("i=device/server;p=1760,600");
            gates:
                ethg[1];
        }
        s153: MpcHost {
            @display("i=device/server;p=1800,600");
            gates:
                ethg[1];
        }
        s154: MpcHost {
            @display("i=device/server;p=1840,600");
            gates:
                ethg[1];
        }
        s155: MpcHost {
            @display("i=device/server;p=1880,600");
            gates:
                ethg[1];
        }
        s156: MpcHost {
            @display("i=device/server;p=1920,600");
            gates:
                ethg[1];
        }
        s157: MpcHost {
            @display("i=device/server;p=1960,600");
            gates:
                ethg[1];
        }
        s158: MpcHost {
            @display("i=device/server;p=2000,600");
            gates:
                ethg[1];
        }
        s159: MpcHost {
            @display("i=device/server;p=2040,600");
            gates:
                ethg[1];
        }
        s160: MpcHost {
            @display("i=device/server;p=2080,600");
            gates:
                ethg[1];
        }
        s161: MpcHost {
            @display("i=device/server;p=2160,600");
            gates:
                ethg[1];
        }
        s162: MpcHost {
            @display("i=device/server;p=2200,600");
            gates:
                ethg[1];
        }
        s163: MpcHost {
            @display("i=device/server;p=2240,600");
            gates:
                ethg[1];
        }
        s164: MpcHost {
            @display("i=device/server;p=2280,600");
            gates:
                ethg[1];
        }
        s165: MpcHost {
            @display("i=device/server;p=2320,600");
            gates:
                ethg[1];
        }
        s166: MpcHost {
            @display("i=device/server;p=2360,600");
            gates:
                ethg[1];
        }
        s167: MpcHost {
            @display("i=device/server;p=2400,600");
            gates:
                ethg[1];
        }
        s168: MpcHost {
            @display("i=device/server;p=2440,600");
            gates:
                ethg[1];
        }
        s169: MpcHost {
            @display("i=device/server;p=2480,600");
            gates:
                ethg[1];
        }
        s170: MpcHost {
            @display("i=device/server;p=2520,600");
            gates:
                ethg[1];
        }
        s171: MpcHost {
            @display("i=device/server;p=2600,600");
            gates:
                ethg[1];
        }
        s172: MpcHost {
            @display("i=device/server;p=2640,600");
            gates:
                ethg[1];
        }
        s173: MpcHost {
            @display("i=device/server;p=2680,600");
            gates:
                ethg[1];
        }
        s174: MpcHost {
            @display("i=device/server;p=2720,600");
            gates:
                ethg[1];
        }
        s175: MpcHost {
            @display("i=device/server;p=2760,600");
            gates:
                ethg[1];
        }
        s176: MpcHost {
            @display("i=device/server;p=2800,600");
            gates:
                ethg[1];
        }
        s177: MpcHost {
            @display("i=device/server;p=2840,600");
            gates:
                ethg[1];
        }
        s178: MpcHost {
            @display("i=device/server;p=2880,600");
            gates:
                ethg[1];
        }
        s179: MpcHost {
            @display("i=device/server;p=2920,600");
            gates:
                ethg[1];
        }
        s180: MpcHost {
            @display("i=device/server;p=2960,600");
            gates:
                ethg[1];
        }
        s181: MpcHost {
            @display("i=device/server;p=3040,600");
            gates:
                ethg[1];
        }
        s182: MpcHost {
            @display("i=device/server;p=3080,600");
            gates:
                ethg[1];
        }
        s183: MpcHost {
            @display("i=device/server;p=3120,600");
            gates:
                ethg[1];
        }
        s184: MpcHost {
            @display("i=device/server;p=3160,600");
            gates:
                ethg[1];
        }
        s185: MpcHost {
            @display("i=device/server;p=3200,600");
            gates:
                ethg[1];
        }
        s186: MpcHost {
            @display("i=device/server;p=3240,600");
            gates:
                ethg[1];
        }
        s187: MpcHost {
            @display("i=device/server;p=3280,600");
            gates:
                ethg[1];
        }
        s188: MpcHost {
            @display("i=device/server;p=3320,600");
            gates:
                ethg[1];
        }
        s189: MpcHost {
            @display("i=device/server;p=3360,600");
            gates:
                ethg[1];
        }
        s190: MpcHost {
            @display("i=device/server;p=3400,600");
            gates:
                ethg[1];
        }
        s191: MpcHost {
            @display("i=device/server;p=3480,600");
            gates:
                ethg[1];
        }
        s192: MpcHost {
            @display("i=device/server;p=3520,600");
            gates:
                ethg[1];
        }
        s193: MpcHost {
            @display("i=device/server;p=3560,600");
            gates:
                ethg[1];
        }
        s194: MpcHost {
            @display("i=device/server;p=3600,600");
            gates:
                ethg[1];
        }
        s195: MpcHost {
            @display("i=device/server;p=3640,600");
            gates:
                ethg[1];
        }
        s196: MpcHost {
            @display("i=device/server;p=3680,600");
            gates:
                ethg[1];
        }
        s197: MpcHost {
            @display("i=device/server;p=3720,600");
            gates:
                ethg[1];
        }
        s198: MpcHost {
            @display("i=device/server;p=3760,600");
            gates:
                ethg[1];
        }
        s199: MpcHost {
            @display("i=device/server;p=3800,600");
            gates:
                ethg[1];
        }
        s200: MpcHost {
            @display("i=device/server;p=3840,600");
            gates:
                ethg[1];
        }
        s201: MpcHost {
            @display("i=device/server;p=3920,600");
            gates:
                ethg[1];
        }
        s202: MpcHost {
            @display("i=device/server;p=3960,600");
            gates:
                ethg[1];
        }
        s203: MpcHost {
            @display("i=device/server;p=4000,600");
            gates:
                ethg[1];
        }
        s204: MpcHost {
            @display("i=device/server;p=4040,600");
            gates:
                ethg[1];
        }
        s205: MpcHost {
            @display("i=device/server;p=4080,600");
            gates:
                ethg[1];
        }
        s206: MpcHost {
            @display("i=device/server;p=4120,600");
            gates:
                ethg[1];
        }
        s207: MpcHost {
            @display("i=device/server;p=4160,600");
            gates:
                ethg[1];
        }
        s208: MpcHost {
            @display("i=device/server;p=4200,600");
            gates:
                ethg[1];
        }
        s209: MpcHost {
            @display("i=device/server;p=4240,600");
            gates:
                ethg[1];
        }
        s210: MpcHost {
            @display("i=device/server;p=4280,600");
            gates:
                ethg[1];
        }
        s211: MpcHost {
            @display("i=device/server;p=4360,600");
            gates:
                ethg[1];
        }
        s212: MpcHost {
            @display("i=device/server;p=4400,600");
            gates:
                ethg[1];
        }
        s213: MpcHost {
            @display("i=device/server;p=4440,600");
            gates:
                ethg[1];
        }
        s214: MpcHost {
            @display("i=device/server;p=4480,600");
            gates:
                ethg[1];
        }
        s215: MpcHost {
            @display("i=device/server;p=4520,600");
            gates:
                ethg[1];
        }
        s216: MpcHost {
            @display("i=device/server;p=4560,600");
            gates:
                ethg[1];
        }
        s217: MpcHost {
            @display("i=device/server;p=4600,600");
            gates:
                ethg[1];
        }
        s218: MpcHost {
            @display("i=device/server;p=4640,600");
            gates:
                ethg[1];
        }
        s219: MpcHost {
            @display("i=device/server;p=4680,600");
            gates:
                ethg[1];
        }
        s220: MpcHost {
            @display("i=device/server;p=4720,600");
            gates:
                ethg[1];
        }
        s221: MpcHost {
            @display("i=device/server;p=4800,600");
            gates:
                ethg[1];
        }
        s222: MpcHost {
            @display("i=device/server;p=4840,600");
            gates:
                ethg[1];
        }
        s223: MpcHost {
            @display("i=device/server;p=4880,600");
            gates:
                ethg[1];
        }
        s224: MpcHost {
            @display("i=device/server;p=4920,600");
            gates:
                ethg[1];
        }
        s225: MpcHost {
            @display("i=device/server;p=4960,600");
            gates:
                ethg[1];
        }
        s226: MpcHost {
            @display("i=device/server;p=5000,600");
            gates:
                ethg[1];
        }
        s227: MpcHost {
            @display("i=device/server;p=5040,600");
            gates:
                ethg[1];
        }
        s228: MpcHost {
            @display("i=device/server;p=5080,600");
            gates:
                ethg[1];
        }
        s229: MpcHost {
            @display("i=device/server;p=5120,600");
            gates:
                ethg[1];
        }
        s230: MpcHost {
            @display("i=device/server;p=5160,600");
            gates:
                ethg[1];
        }
        s231: MpcHost {
            @display("i=device/server;p=5240,600");
            gates:
                ethg[1];
        }
        s232: MpcHost {
            @display("i=device/server;p=5280,600");
            gates:
                ethg[1];
        }
        s233: MpcHost {
            @display("i=device/server;p=5320,600");
            gates:
                ethg[1];
        }
        s234: MpcHost {
            @display("i=device/server;p=5360,600");
            gates:
                ethg[1];
        }
        s235: MpcHost {
            @display("i=device/server;p=5400,600");
            gates:
                ethg[1];
        }
        s236: MpcHost {
            @display("i=device/server;p=5440,600");
            gates:
                ethg[1];
        }
        s237: MpcHost {
            @display("i=device/server;p=5480,600");
            gates:
                ethg[1];
        }
        s238: MpcHost {
            @display("i=device/server;p=5520,600");
            gates:
                ethg[1];
        }
        s239: MpcHost {
            @display("i=device/server;p=5560,600");
            gates:
                ethg[1];
        }
        s240: MpcHost {
            @display("i=device/server;p=5600,600");
            gates:
                ethg[1];
        }
    connections:
        s1.ethg++ <--> leafchannel <--> leaf1.ethg++;
        s2.ethg++ <--> leafchannel <--> leaf1.ethg++;
        s3.ethg++ <--> leafchannel <--> leaf1.ethg++;
        s4.ethg++ <--> leafchannel <--> leaf1.ethg++;
        s5.ethg++ <--> leafchannel <--> leaf1.ethg++;
        s6.ethg++ <--> leafchannel <--> leaf1.ethg++;
        s7.ethg++ <--> leafchannel <--> leaf1.ethg++;
        s8.ethg++ <--> leafchannel <--> leaf1.ethg++;
        s9.ethg++ <--> leafchannel <--> leaf1.ethg++;
        s10.ethg++ <--> leafchannel <--> leaf1.ethg++;
        
        s11.ethg++ <--> leafchannel <--> leaf2.ethg++;
        s12.ethg++ <--> leafchannel <--> leaf2.ethg++;
        s13.ethg++ <--> leafchannel <--> leaf2.ethg++;
        s14.ethg++ <--> leafchannel <--> leaf2.ethg++;
        s15.ethg++ <--> leafchannel <--> leaf2.ethg++;
        s16.ethg++ <--> leafchannel <--> leaf2.ethg++;
        s17.ethg++ <--> leafchannel <--> leaf2.ethg++;
        s18.ethg++ <--> leafchannel <--> leaf2.ethg++;
        s19.ethg++ <--> leafchannel <--> leaf2.ethg++;
        s20.ethg++ <--> leafchannel <--> leaf2.ethg++;
        
        s21.ethg++ <--> leafchannel <--> leaf3.ethg++;
        s22.ethg++ <--> leafchannel <--> leaf3.ethg++;
        s23.ethg++ <--> leafchannel <--> leaf3.ethg++;
        s24.ethg++ <--> leafchannel <--> leaf3.ethg++;
        s25.ethg++ <--> leafchannel <--> leaf3.ethg++;
        s26.ethg++ <--> leafchannel <--> leaf3.ethg++;
        s27.ethg++ <--> leafchannel <--> leaf3.ethg++;
        s28.ethg++ <--> leafchannel <--> leaf3.ethg++;
        s29.ethg++ <--> leafchannel <--> leaf3.ethg++;
        s30.ethg++ <--> leafchannel <--> leaf3.ethg++;
        
        s31.ethg++ <--> leafchannel <--> leaf4.ethg++;
        s32.ethg++ <--> leafchannel <--> leaf4.ethg++;
        s33.ethg++ <--> leafchannel <--> leaf4.ethg++;
        s34.ethg++ <--> leafchannel <--> leaf4.ethg++;
        s35.ethg++ <--> leafchannel <--> leaf4.ethg++;
        s36.ethg++ <--> leafchannel <--> leaf4.ethg++;
        s37.ethg++ <--> leafchannel <--> leaf4.ethg++;
        s38.ethg++ <--> leafchannel <--> leaf4.ethg++;
        s39.ethg++ <--> leafchannel <--> leaf4.ethg++;
        s40.ethg++ <--> leafchannel <--> leaf4.ethg++;

        s41.ethg++ <--> leafchannel <--> leaf5.ethg++;
        s42.ethg++ <--> leafchannel <--> leaf5.ethg++;
        s43.ethg++ <--> leafchannel <--> leaf5.ethg++;
        s44.ethg++ <--> leafchannel <--> leaf5.ethg++;
        s45.ethg++ <--> leafchannel <--> leaf5.ethg++;
        s46.ethg++ <--> leafchannel <--> leaf5.ethg++;
        s47.ethg++ <--> leafchannel <--> leaf5.ethg++;
        s48.ethg++ <--> leafchannel <--> leaf5.ethg++;
        s49.ethg++ <--> leafchannel <--> leaf5.ethg++;
        s50.ethg++ <--> leafchannel <--> leaf5.ethg++;
        
        s51.ethg++ <--> leafchannel <--> leaf6.ethg++;
        s52.ethg++ <--> leafchannel <--> leaf6.ethg++;
        s53.ethg++ <--> leafchannel <--> leaf6.ethg++;
        s54.ethg++ <--> leafchannel <--> leaf6.ethg++;
        s55.ethg++ <--> leafchannel <--> leaf6.ethg++;
        s56.ethg++ <--> leafchannel <--> leaf6.ethg++;
        s57.ethg++ <--> leafchannel <--> leaf6.ethg++;
        s58.ethg++ <--> leafchannel <--> leaf6.ethg++;
        s59.ethg++ <--> leafchannel <--> leaf6.ethg++;
        s60.ethg++ <--> leafchannel <--> leaf6.ethg++;
        
        s61.ethg++ <--> leafchannel <--> leaf7.ethg++;
        s62.ethg++ <--> leafchannel <--> leaf7.ethg++;
        s63.ethg++ <--> leafchannel <--> leaf7.ethg++;
        s64.ethg++ <--> leafchannel <--> leaf7.ethg++;
        s65.ethg++ <--> leafchannel <--> leaf7.ethg++;
        s66.ethg++ <--> leafchannel <--> leaf7.ethg++;
        s67.ethg++ <--> leafchannel <--> leaf7.ethg++;
        s68.ethg++ <--> leafchannel <--> leaf7.ethg++;
        s69.ethg++ <--> leafchannel <--> leaf7.ethg++;
        s70.ethg++ <--> leafchannel <--> leaf7.ethg++;
        
        leaf8.ethg++ <--> leafchannel <--> s71.ethg++;
        leaf8.ethg++ <--> leafchannel <--> s72.ethg++;
        leaf8.ethg++ <--> leafchannel <--> s73.ethg++;
        leaf8.ethg++ <--> leafchannel <--> s74.ethg++;
        leaf8.ethg++ <--> leafchannel <--> s75.ethg++;
        leaf8.ethg++ <--> leafchannel <--> s76.ethg++;
        leaf8.ethg++ <--> leafchannel <--> s77.ethg++;
        leaf8.ethg++ <--> leafchannel <--> s78.ethg++;
        leaf8.ethg++ <--> leafchannel <--> s79.ethg++;
        leaf8.ethg++ <--> leafchannel <--> s80.ethg++;

        leaf9.ethg++ <--> leafchannel <--> s81.ethg++;
        leaf9.ethg++ <--> leafchannel <--> s82.ethg++;
        leaf9.ethg++ <--> leafchannel <--> s83.ethg++;
        leaf9.ethg++ <--> leafchannel <--> s84.ethg++;
        leaf9.ethg++ <--> leafchannel <--> s85.ethg++;
        leaf9.ethg++ <--> leafchannel <--> s86.ethg++;
        leaf9.ethg++ <--> leafchannel <--> s87.ethg++;
        leaf9.ethg++ <--> leafchannel <--> s88.ethg++;
        leaf9.ethg++ <--> leafchannel <--> s89.ethg++;
        leaf9.ethg++ <--> leafchannel <--> s90.ethg++;
        
        leaf10.ethg++ <--> leafchannel <--> s91.ethg++;
        leaf10.ethg++ <--> leafchannel <--> s92.ethg++;
        leaf10.ethg++ <--> leafchannel <--> s93.ethg++;
        leaf10.ethg++ <--> leafchannel <--> s94.ethg++;
        leaf10.ethg++ <--> leafchannel <--> s95.ethg++;
        leaf10.ethg++ <--> leafchannel <--> s96.ethg++;
        leaf10.ethg++ <--> leafchannel <--> s97.ethg++;
        leaf10.ethg++ <--> leafchannel <--> s98.ethg++;
        leaf10.ethg++ <--> leafchannel <--> s99.ethg++;
        leaf10.ethg++ <--> leafchannel <--> s100.ethg++;
        
        leaf11.ethg++ <--> leafchannel <--> s101.ethg++;
        leaf11.ethg++ <--> leafchannel <--> s102.ethg++;
        leaf11.ethg++ <--> leafchannel <--> s103.ethg++;
        leaf11.ethg++ <--> leafchannel <--> s104.ethg++;
        leaf11.ethg++ <--> leafchannel <--> s105.ethg++;
        leaf11.ethg++ <--> leafchannel <--> s106.ethg++;
        leaf11.ethg++ <--> leafchannel <--> s107.ethg++;
        leaf11.ethg++ <--> leafchannel <--> s108.ethg++;
        leaf11.ethg++ <--> leafchannel <--> s109.ethg++;
        leaf11.ethg++ <--> leafchannel <--> s110.ethg++;
        
        leaf12.ethg++ <--> leafchannel <--> s111.ethg++;
        leaf12.ethg++ <--> leafchannel <--> s112.ethg++;
        leaf12.ethg++ <--> leafchannel <--> s113.ethg++;
        leaf12.ethg++ <--> leafchannel <--> s114.ethg++;
        leaf12.ethg++ <--> leafchannel <--> s115.ethg++;
        leaf12.ethg++ <--> leafchannel <--> s116.ethg++;
        leaf12.ethg++ <--> leafchannel <--> s117.ethg++;
        leaf12.ethg++ <--> leafchannel <--> s118.ethg++;
        leaf12.ethg++ <--> leafchannel <--> s119.ethg++;
        leaf12.ethg++ <--> leafchannel <--> s120.ethg++;

        leaf13.ethg++ <--> leafchannel <--> s121.ethg++;
        leaf13.ethg++ <--> leafchannel <--> s122.ethg++;
        leaf13.ethg++ <--> leafchannel <--> s123.ethg++;
        leaf13.ethg++ <--> leafchannel <--> s124.ethg++;
        leaf13.ethg++ <--> leafchannel <--> s125.ethg++;
        leaf13.ethg++ <--> leafchannel <--> s126.ethg++;
        leaf13.ethg++ <--> leafchannel <--> s127.ethg++;
        leaf13.ethg++ <--> leafchannel <--> s128.ethg++;
        leaf13.ethg++ <--> leafchannel <--> s129.ethg++;
        leaf13.ethg++ <--> leafchannel <--> s130.ethg++;
        
        leaf14.ethg++ <--> leafchannel <--> s131.ethg++;
        leaf14.ethg++ <--> leafchannel <--> s132.ethg++;
        leaf14.ethg++ <--> leafchannel <--> s133.ethg++;
        leaf14.ethg++ <--> leafchannel <--> s134.ethg++;
        leaf14.ethg++ <--> leafchannel <--> s135.ethg++;
        leaf14.ethg++ <--> leafchannel <--> s136.ethg++;
        leaf14.ethg++ <--> leafchannel <--> s137.ethg++;
        leaf14.ethg++ <--> leafchannel <--> s138.ethg++;
        leaf14.ethg++ <--> leafchannel <--> s139.ethg++;
        leaf14.ethg++ <--> leafchannel <--> s140.ethg++;
        
        leaf15.ethg++ <--> leafchannel <--> s141.ethg++;
        leaf15.ethg++ <--> leafchannel <--> s142.ethg++;
        leaf15.ethg++ <--> leafchannel <--> s143.ethg++;
        leaf15.ethg++ <--> leafchannel <--> s144.ethg++;
        leaf15.ethg++ <--> leafchannel <--> s145.ethg++;
        leaf15.ethg++ <--> leafchannel <--> s146.ethg++;
        leaf15.ethg++ <--> leafchannel <--> s147.ethg++;
        leaf15.ethg++ <--> leafchannel <--> s148.ethg++;
        leaf15.ethg++ <--> leafchannel <--> s149.ethg++;
        leaf15.ethg++ <--> leafchannel <--> s150.ethg++;
        
        leaf16.ethg++ <--> leafchannel <--> s151.ethg++;
        leaf16.ethg++ <--> leafchannel <--> s152.ethg++;
        leaf16.ethg++ <--> leafchannel <--> s153.ethg++;
        leaf16.ethg++ <--> leafchannel <--> s154.ethg++;
        leaf16.ethg++ <--> leafchannel <--> s155.ethg++;
        leaf16.ethg++ <--> leafchannel <--> s156.ethg++;
        leaf16.ethg++ <--> leafchannel <--> s157.ethg++;
        leaf16.ethg++ <--> leafchannel <--> s158.ethg++;
        leaf16.ethg++ <--> leafchannel <--> s159.ethg++;
        leaf16.ethg++ <--> leafchannel <--> s160.ethg++;

        leaf17.ethg++ <--> leafchannel <--> s161.ethg++;
        leaf17.ethg++ <--> leafchannel <--> s162.ethg++;
        leaf17.ethg++ <--> leafchannel <--> s163.ethg++;
        leaf17.ethg++ <--> leafchannel <--> s164.ethg++;
        leaf17.ethg++ <--> leafchannel <--> s165.ethg++;
        leaf17.ethg++ <--> leafchannel <--> s166.ethg++;
        leaf17.ethg++ <--> leafchannel <--> s167.ethg++;
        leaf17.ethg++ <--> leafchannel <--> s168.ethg++;
        leaf17.ethg++ <--> leafchannel <--> s169.ethg++;
        leaf17.ethg++ <--> leafchannel <--> s170.ethg++;
        
        leaf18.ethg++ <--> leafchannel <--> s171.ethg++;
        leaf18.ethg++ <--> leafchannel <--> s172.ethg++;
        leaf18.ethg++ <--> leafchannel <--> s173.ethg++;
        leaf18.ethg++ <--> leafchannel <--> s174.ethg++;
        leaf18.ethg++ <--> leafchannel <--> s175.ethg++;
        leaf18.ethg++ <--> leafchannel <--> s176.ethg++;
        leaf18.ethg++ <--> leafchannel <--> s177.ethg++;
        leaf18.ethg++ <--> leafchannel <--> s178.ethg++;
        leaf18.ethg++ <--> leafchannel <--> s179.ethg++;
        leaf18.ethg++ <--> leafchannel <--> s180.ethg++;
        
        leaf19.ethg++ <--> leafchannel <--> s181.ethg++;
        leaf19.ethg++ <--> leafchannel <--> s182.ethg++;
        leaf19.ethg++ <--> leafchannel <--> s183.ethg++;
        leaf19.ethg++ <--> leafchannel <--> s184.ethg++;
        leaf19.ethg++ <--> leafchannel <--> s185.ethg++;
        leaf19.ethg++ <--> leafchannel <--> s186.ethg++;
        leaf19.ethg++ <--> leafchannel <--> s187.ethg++;
        leaf19.ethg++ <--> leafchannel <--> s188.ethg++;
        leaf19.ethg++ <--> leafchannel <--> s189.ethg++;
        leaf19.ethg++ <--> leafchannel <--> s190.ethg++;
        
        leaf20.ethg++ <--> leafchannel <--> s191.ethg++;
        leaf20.ethg++ <--> leafchannel <--> s192.ethg++;
        leaf20.ethg++ <--> leafchannel <--> s193.ethg++;
        leaf20.ethg++ <--> leafchannel <--> s194.ethg++;
        leaf20.ethg++ <--> leafchannel <--> s195.ethg++;
        leaf20.ethg++ <--> leafchannel <--> s196.ethg++;
        leaf20.ethg++ <--> leafchannel <--> s197.ethg++;
        leaf20.ethg++ <--> leafchannel <--> s198.ethg++;
        leaf20.ethg++ <--> leafchannel <--> s199.ethg++;
        leaf20.ethg++ <--> leafchannel <--> s200.ethg++;
        
        leaf21.ethg++ <--> leafchannel <--> s201.ethg++;
        leaf21.ethg++ <--> leafchannel <--> s202.ethg++;
        leaf21.ethg++ <--> leafchannel <--> s203.ethg++;
        leaf21.ethg++ <--> leafchannel <--> s204.ethg++;
        leaf21.ethg++ <--> leafchannel <--> s205.ethg++;
        leaf21.ethg++ <--> leafchannel <--> s206.ethg++;
        leaf21.ethg++ <--> leafchannel <--> s207.ethg++;
        leaf21.ethg++ <--> leafchannel <--> s208.ethg++;
        leaf21.ethg++ <--> leafchannel <--> s209.ethg++;
        leaf21.ethg++ <--> leafchannel <--> s210.ethg++;
        
        leaf22.ethg++ <--> leafchannel <--> s211.ethg++;
        leaf22.ethg++ <--> leafchannel <--> s212.ethg++;
        leaf22.ethg++ <--> leafchannel <--> s213.ethg++;
        leaf22.ethg++ <--> leafchannel <--> s214.ethg++;
        leaf22.ethg++ <--> leafchannel <--> s215.ethg++;
        leaf22.ethg++ <--> leafchannel <--> s216.ethg++;
        leaf22.ethg++ <--> leafchannel <--> s217.ethg++;
        leaf22.ethg++ <--> leafchannel <--> s218.ethg++;
        leaf22.ethg++ <--> leafchannel <--> s219.ethg++;
        leaf22.ethg++ <--> leafchannel <--> s220.ethg++;
        
        leaf23.ethg++ <--> leafchannel <--> s221.ethg++;
        leaf23.ethg++ <--> leafchannel <--> s222.ethg++;
        leaf23.ethg++ <--> leafchannel <--> s223.ethg++;
        leaf23.ethg++ <--> leafchannel <--> s224.ethg++;
        leaf23.ethg++ <--> leafchannel <--> s225.ethg++;
        leaf23.ethg++ <--> leafchannel <--> s226.ethg++;
        leaf23.ethg++ <--> leafchannel <--> s227.ethg++;
        leaf23.ethg++ <--> leafchannel <--> s228.ethg++;
        leaf23.ethg++ <--> leafchannel <--> s229.ethg++;
        leaf23.ethg++ <--> leafchannel <--> s230.ethg++;
        
        leaf24.ethg++ <--> leafchannel <--> s231.ethg++;
        leaf24.ethg++ <--> leafchannel <--> s232.ethg++;
        leaf24.ethg++ <--> leafchannel <--> s233.ethg++;
        leaf24.ethg++ <--> leafchannel <--> s234.ethg++;
        leaf24.ethg++ <--> leafchannel <--> s235.ethg++;
        leaf24.ethg++ <--> leafchannel <--> s236.ethg++;
        leaf24.ethg++ <--> leafchannel <--> s237.ethg++;
        leaf24.ethg++ <--> leafchannel <--> s238.ethg++;
        leaf24.ethg++ <--> leafchannel <--> s239.ethg++;
        leaf24.ethg++ <--> leafchannel <--> s240.ethg++;
           
        Spine1.ethg++ <--> Spinechannel <--> leaf1.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf2.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf3.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf4.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf5.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf6.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf7.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf8.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf9.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf10.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf11.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf12.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf13.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf14.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf15.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf16.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf17.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf18.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf19.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf20.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf21.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf22.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf23.ethg++;
        Spine1.ethg++ <--> Spinechannel <--> leaf24.ethg++;

        Spine2.ethg++ <--> Spinechannel <--> leaf1.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf2.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf3.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf4.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf5.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf6.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf7.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf8.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf9.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf10.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf11.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf12.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf13.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf14.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf15.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf16.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf17.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf18.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf19.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf20.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf21.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf22.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf23.ethg++;
        Spine2.ethg++ <--> Spinechannel <--> leaf24.ethg++;

        Spine3.ethg++ <--> Spinechannel <--> leaf1.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf2.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf3.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf4.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf5.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf6.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf7.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf8.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf9.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf10.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf11.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf12.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf13.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf14.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf15.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf16.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf17.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf18.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf19.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf20.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf21.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf22.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf23.ethg++;
        Spine3.ethg++ <--> Spinechannel <--> leaf24.ethg++;

        Spine4.ethg++ <--> Spinechannel <--> leaf1.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf2.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf3.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf4.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf5.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf6.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf7.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf8.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf9.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf10.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf11.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf12.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf13.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf14.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf15.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf16.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf17.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf18.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf19.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf20.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf21.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf22.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf23.ethg++;
        Spine4.ethg++ <--> Spinechannel <--> leaf24.ethg++;

}