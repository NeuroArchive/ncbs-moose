//moose
// This is an almost perfect match to the old GENESIS squid model
// output with default parameter values. It simulates, as the Squid
// demo does, a stimulus of 0.1 uA starting at time 5 msec and 
// lasting for 40 msec. There is a final 5 msec after the stimulus.
// Most of this file is setting up the parameters and the HH tables.
// Later I'll implement and extended version of the HHGate that knows
// the funny X_alpha and other terms that the GENESIS version uses.

int i

float VMIN = -0.1
float VMAX = 0.05
int NDIVS = 150
float v = VMIN
float dv = ( VMAX - VMIN ) / NDIVS
float SIMDT = 1e-5
float PLOTDT = 1e-4
float RUNTIME = 0.5
float EREST = -0.07
float VLEAK = EREST + 0.010613
float VK = EREST -0.012
float VNa = EREST + 0.115
float RM = 424.4e3
float RA = 7639.44e3
float GLEAK = 0.3e-3
float GK = 36e-3
float GNa = 120e-3
float CM = 0.007854e-6
float INJECT = 0.1e-6

GK = 0.282743e-3
GNa = 0.94248e-3

setfield /sli_shell isInteractive 0

function calc_Na_m_A( v )
	float v

	if ( { abs { EREST + 0.025 - v } } < 1e-6 )
		v = v + 1e-6
	end
	
	return { ( 0.1e6 * ( EREST + 0.025 - v ) ) / ( { exp { ( EREST + 0.025 - v )/ 0.01 } } - 1.0 ) }
end

function calc_Na_m_B( v )
	float v
	return { 4.0e3 * { exp { ( EREST - v ) / 0.018 } } }
end

function calc_Na_h_A( v )
	float v
	return { 70.0 * { exp { ( EREST - v ) / 0.020 } } }
end

function calc_Na_h_B( v )
	float v
	return { 1.0e3 / ( { exp { ( 0.030 + EREST - v )/ 0.01 } } + 1.0 ) }
end

function calc_K_n_A( v )
	float v
	if ( { abs { 0.01 + EREST - v } } < 1e-6 )
		v = v + 1e-6
	end
	return { ( 1.0e4 * ( 0.01 + EREST - v ) ) / ( { exp { ( 0.01 + EREST - v )/ 0.01 } } - 1.0 ) }
end

function calc_K_n_B( v )
	float v
	return { 0.125e3 * { exp { (EREST - v ) / 0.08 } } }
end

create Compartment /squid
setfield /squid Rm {RM}
setfield /squid Ra {RA}
setfield /squid Cm {CM}
setfield /squid Em {VLEAK}

create HHChannel /squid/Na
setfield /squid/Na Ek {VNa}
setfield /squid/Na Gbar {GNa}
setfield /squid/Na Xpower 3
setfield /squid/Na Ypower 1
create HHGate /squid/Na/m
setfield /squid/Na/m power 3
create HHGate /squid/Na/h
setfield /squid/Na/h power 1

create HHChannel /squid/K
setfield /squid/K Ek {VK}
setfield /squid/K Gbar {GK}
setfield /squid/K Xpower 4
create HHGate /squid/K/n
setfield /squid/K/n power 4

create CaConc /squid/CaConc
setfield /squid/CaConc B 1.0e3
setfield /squid/CaConc CaBasal 1.0e-7
setfield /squid/CaConc tau 10.0e-3

create Nernst /squid/CaPotl
setfield /squid/CaPotl valence 2
setfield /squid/CaPotl Cout 5e-4

addmsg /squid/channel /squid/Na/channel
addmsg /squid/channel /squid/K/channel

addmsg /squid/Na/IkOut /squid/CaConc/currentIn
addmsg /squid/CaConc/concOut /squid/CaPotl/CinIn

addmsg /squid/Na/xGate /squid/Na/m/gate
addmsg /squid/Na/yGate /squid/Na/h/gate
addmsg /squid/K/xGate /squid/K/n/gate

create Plot /Vm
addmsg /Vm/trigPlot /squid/Vm

create Plot /gNa
addmsg /gNa/trigPlot /squid/Na/Gk
create Plot /gK
addmsg /gK/trigPlot /squid/K/Gk
create Plot /NaConc
addmsg /NaConc/trigPlot /squid/CaConc/Ca
create Plot /CaPotl
addmsg /CaPotl/trigPlot /squid/CaPotl/E

setfield /squid/Na/m A.xmin {VMIN}
setfield /squid/Na/m B.xmin {VMIN}
setfield /squid/Na/h A.xmin {VMIN}
setfield /squid/Na/h B.xmin {VMIN}
setfield /squid/K/n A.xmin {VMIN}
setfield /squid/K/n B.xmin {VMIN}
setfield /squid/Na/m A.xmax {VMAX}
setfield /squid/Na/m B.xmax {VMAX}
setfield /squid/Na/h A.xmax {VMAX}
setfield /squid/Na/h B.xmax {VMAX}
setfield /squid/K/n A.xmax {VMAX}
setfield /squid/K/n B.xmax {VMAX}
setfield /squid/Na/m A.xdivs {NDIVS}
setfield /squid/Na/m B.xdivs {NDIVS}
setfield /squid/Na/h A.xdivs {NDIVS}
setfield /squid/Na/h B.xdivs {NDIVS}
setfield /squid/K/n A.xdivs {NDIVS}
setfield /squid/K/n B.xdivs {NDIVS}

v = VMIN
for ( i = 0 ; i <= NDIVS; i = i + 1 )
	setfield /squid/Na/m A.table[{i}] { calc_Na_m_A { v } }
	setfield /squid/Na/m B.table[{i}] { { calc_Na_m_A { v } } + { calc_Na_m_B { v } } }
	setfield /squid/Na/h A.table[{i}] { calc_Na_h_A { v } }
	setfield /squid/Na/h B.table[{i}] { { calc_Na_h_A { v } } +  { calc_Na_h_B { v } } }
	setfield /squid/K/n A.table[{i}] { calc_K_n_A { v } }
	setfield /squid/K/n B.table[{i}] { { calc_K_n_A { v } } + { calc_K_n_B { v } } }
//setfield /sli_shell isInteractive 1
	echo {v} { calc_K_n_B { v } }
setfield /sli_shell isInteractive 0
	v = v + dv
//	echo {v}
end

setclock 0 {SIMDT} 0
setclock 1 {PLOTDT} 0

useclock /squid,squid/CaConc 0
useclock /##[TYPE=Plot] 1

// Crazy hack, but the squid demo does it and we need to match.
setfield /squid initVm {EREST}
reset
setfield /squid Inject 0
step 0.005 -t
setfield /squid Inject {INJECT}
step 0.040 -t
setfield /squid Inject 0
step 0.005 -t
call /##[TYPE=Plot] printIn casquid.plot
setfield /sli_shell isInteractive 1
// quit
