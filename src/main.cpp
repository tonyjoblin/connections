#include "headers.hpp"

#include <iostream>
#include <string>
#include <tuple>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time.hpp>

using namespace std;
using namespace boost;

namespace pt = boost::property_tree;

time_t CalculateTime(const string& tripDate, const string& time)
{
    using namespace posix_time;
    ptime t = time_from_string(tripDate + " " + time);
    return to_time_t(t);
}

optional<string> GetAttribute(const pt::ptree& node, const string& attribute)
{
    return node.get_optional<string>("<xmlattr>." + attribute);
}

optional<time_t> GetPlannedArrivalTime(const pt::ptree& stop, const string& tripDate)
{
    optional<string> pta = GetAttribute(stop, "pta");
    if (pta == boost::none)
    {
        return boost::none;
    }
    return CalculateTime(tripDate, *pta);
}

optional<time_t> GetPlannedDepartureTime(const pt::ptree& stop, const string& tripDate)
{
    optional<string> ptd = GetAttribute(stop, "ptd");
    if (ptd == boost::none) {
        return boost::none;
    }
    return CalculateTime(tripDate, *ptd);
}

optional<time_t> GetPassingPointTime(const pt::ptree& stop, const string& tripDate)
{
    optional<string> passingTime = GetAttribute(stop, "wtp");
    if (passingTime == boost::none) {
        return boost::none;
    }
    return CalculateTime(tripDate, *passingTime);
}

time_t AdjustTime(time_t time, const time_t lastTime)
{
    const time_t tenHours = 10 * 60 * 60;
    const time_t twentyFourHours = 24 * 60 * 60;
    if (time + tenHours < lastTime) {
        time += twentyFourHours;
    }
    return time;
}

void WriteConnection(
    ostream& out,
    time_t  departing,
    const   string& origin, 
    time_t  arriving, 
    const   string& destination,
    const   string& rid,
    const   string& uid,
    const   string& toc
    )
{
    const char s = '|';
    out 
        << departing << s
        << origin << s
        << arriving << s 
        << destination << s 
        << rid << s 
        << uid << s
        << toc << endl;
}

string GetStopTiploc(const pt::ptree& stop)
{
    return *GetAttribute(stop, "ftl");
}

// TODO stops that only have board
// TODO stops that only have alight
// TODO split/join trips

enum StatesType { LookingForArrival, LookingForDeparture };

struct ConnectionState
{
public:
    
    StatesType  state;
    
    string lastDepartureTiploc;
    time_t departureTime;
    time_t       lastTime = 0;
    
    string tripRid;
    string tripDate;
    string tripUid;
    string toc;
}

ConnectionState HandlePassingPoint(const pt::ptree& passingPoint, const ConnectionState& state)
{
    optional<time_t> passingTime = GetPassingPointTime(passingPoint, tripDate);
    if (passingTime == boost::none) {
        return state;
    }
    
    ConnectionState newState(state);
    newState.lastTime = AdjustTime(*passingTime, state.lastTime);
    return newState;
}

ConnectionState HandleDepartureTime(const pt::ptree& passingPoint, const ConnectionState& state)
{
    optional<time_t> ptd = GetPlannedDepartureTime(stop, tripDate);
    if (ptd == boost::none) {
        return state;
    }
    ConnectionState newState(state);
    newState.lastDepartureTiploc = GetStopTiploc(stop);
    departureTime = AdjustTime(*ptd, lastTime);
    lastTime = departureTime;
    state = LookingForArrival;

}

ConnectionState HandleArrivalTime(const pt::ptree& stop, const ConnectionState& state)
{
    optional<time_t> pta = GetPlannedArrivalTime(stop, state.tripDate);
    if (pta == boost::none) {
        return state;
    }
    time_t arrivingAt = AdjustTime(*pta, state.lastTime);
    const string& arrivalTiploc = GetStopTiploc(stop);
    
    WriteConnection(
        out,
        state.departureTime,
        state.lastDepartureTiploc,
        arrivingAt,
        arrivalTiploc,
        state.tripRid,
        state.tripUid,
        state.toc
        );
    
    ConnectionState newState(state);
    state.state = LookingForDeparture;
    state.lastTime = arrivingAt;
    return newState;
}

void ProcessJourney(const pt::ptree& journey, ostream& out)
{
    ConnectionState state;
    state.state = LookingForDeparture;
    state.lastTime = 0;
    state.tripRid = *GetAttribute(journey, "rid");
    state.tripDate = *GetAttribute(journey, "ssd");
    state.tripUid = *GetAttribute(journey, "uid");
    state.toc = *GetAttribute(journey, "toc");
    
    for(const pt::ptree::value_type& child: journey){
    
        const string& key = child.first;
        const pt::ptree& stop = child.second;
        
        if (key == "PP") {
            state = HandlePassingPoint(stop, state);
        }
        else if (key == "OR" || key == "IP" || key == "DT") {
        
            if (state == LookingForArrival) {
                state = HandleArrivalTime(stop, state);
            }

            if (state == LookingForDeparture) {
            }
        }
    }
}

void ProcessPushPortTimetable(const pt::ptree& timetable, ostream& out)
{
    for(const pt::ptree::value_type& child: timetable){
        const string key = child.first;
        if (key == "Journey"){
                ProcessJourney(child.second, out);
        }
    }
}

void ProcessTimetableFile(const string& timetableFileName, ostream& out)
{
    pt::ptree document;
    pt::xml_parser::read_xml(timetableFileName, document);
    const pt::ptree& timetable = document.get_child("PportTimetable");
    ProcessPushPortTimetable(timetable, out);
}

int main(int argc, char* argv[])
{
    if (argc != 2){
        cerr << "Timetable file not specified." << endl;
        return 1;
    }

    const string timetableFileName = argv[1];
    ProcessTimetableFile(timetableFileName, cout);

    return 0;
}
