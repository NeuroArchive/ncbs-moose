#ifndef _pymoose_RandomSpike_cpp
#define _pymoose_RandomSpike_cpp
#include "RandomSpike.h"
using namespace pymoose;
const std::string RandomSpike::className = "RandomSpike";
RandomSpike::RandomSpike(Id id):PyMooseBase(id){}
RandomSpike::RandomSpike(std::string path):PyMooseBase(className, path){}
RandomSpike::RandomSpike(std::string name, Id parentId):PyMooseBase(className, name, parentId){}
RandomSpike::RandomSpike(std::string name, PyMooseBase& parent):PyMooseBase(className, name, parent){}
RandomSpike::RandomSpike(const RandomSpike& src, std::string objectName,  PyMooseBase& parent):PyMooseBase(src, objectName, parent){}

RandomSpike::RandomSpike(const RandomSpike& src, std::string objectName, Id& parent):PyMooseBase(src, objectName, parent){}
RandomSpike::RandomSpike(const RandomSpike& src, std::string path):PyMooseBase(src, path)
{
}

RandomSpike::RandomSpike(const Id& src, string name, Id& parent):PyMooseBase(src, name, parent)
{
}
RandomSpike::~RandomSpike(){}
const std::string& RandomSpike::getType(){ return className; }
double RandomSpike::__get_minAmp() const
{
    double minAmp;
    get < double > (id_(), "minAmp",minAmp);
    return minAmp;
}
void RandomSpike::__set_minAmp( double minAmp )
{
    set < double > (id_(), "minAmp", minAmp);
}
double RandomSpike::__get_maxAmp() const
{
    double maxAmp;
    get < double > (id_(), "maxAmp",maxAmp);
    return maxAmp;
}
void RandomSpike::__set_maxAmp( double maxAmp )
{
    set < double > (id_(), "maxAmp", maxAmp);
}
double RandomSpike::__get_rate() const
{
    double rate;
    get < double > (id_(), "rate",rate);
    return rate;
}
void RandomSpike::__set_rate( double rate )
{
    set < double > (id_(), "rate", rate);
}
double RandomSpike::__get_resetValue() const
{
    double resetValue;
    get < double > (id_(), "resetValue",resetValue);
    return resetValue;
}
void RandomSpike::__set_resetValue( double resetValue )
{
    set < double > (id_(), "resetValue", resetValue);
}
double RandomSpike::__get_state() const
{
    double state;
    get < double > (id_(), "state",state);
    return state;
}
void RandomSpike::__set_state( double state )
{
    set < double > (id_(), "state", state);
}
double RandomSpike::__get_absRefract() const
{
    double absRefract;
    get < double > (id_(), "absRefract",absRefract);
    return absRefract;
}
void RandomSpike::__set_absRefract( double absRefract )
{
    set < double > (id_(), "absRefract", absRefract);
}
double RandomSpike::__get_lastEvent() const
{
    double lastEvent;
    get < double > (id_(), "lastEvent",lastEvent);
    return lastEvent;
}
int RandomSpike::__get_reset() const
{
    int reset;
    get < int > (id_(), "reset",reset);
    return reset;
}
void RandomSpike::__set_reset( int reset )
{
    set < int > (id_(), "reset", reset);
}
#endif
