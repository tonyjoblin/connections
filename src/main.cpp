#include "headers.hpp"

#include <iostream>
#include <string>
#include <tuple>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;

namespace pt = boost::property_tree;

int ConverTimeStrToTimeInt(const string& timeStr)
{
        vector<string> hhmm;
        split(hhmm, timeStr, [](char c) { return c ==':'; });
        return atoi(hhmm[0].c_str()) * 60 + atoi(hhmm[1].c_str());
}

optional<string> GetAttribute(const pt::ptree& node, const string& attribute)
{
    return node.get_optional<string>("<xmlattr>." + attribute);
}

optional<int> GetPlannedArrivalTime(const pt::ptree& stop)
{
    optional<string> pta = GetAttribute(stop, "pta");
    if (pta == boost::none)
    {
        return boost::none;
    }
    return ConverTimeStrToTimeInt(*pta);
}

optional<int> GetPlannedDepartureTime(const pt::ptree& stop)
{
    optional<string> ptd = GetAttribute(stop, "ptd");
    if (ptd == boost::none) {
        return boost::none;
    }
    return ConverTimeStrToTimeInt(*ptd);
}

void AdjustTime(int& time, int& dayOffSet, int& lastTime)
{
    time += dayOffSet;
    if (time + 600 < lastTime) {
            dayOffSet += 1440;
            time += dayOffSet;
    }
    lastTime = time;
}

void WriteConnection(
    ostream& out,
    int     departing,
    const   string& origin, 
    int     arriving, 
    const   string& destination,
    const   string& rid
    )
{
    const char s = '|';
    out << departing << s << origin << s << arriving << s << destination << s << rid << endl;
}

string GetStopTiploc(const pt::ptree& stop)
{
    return *GetAttribute(stop, "ftl");
}

// TODO date on trips
// TODO adjust times for stops past midnight
// TODO stops that only have board
// TODO stops that only have alight
// TODO split/join trips

void ProcessJourney(const pt::ptree& journey, ostream& out)
{
    enum StatesType { LookingForArrival, LookingForDeparture };
    StatesType state = LookingForDeparture;
    
    string lastDepartureTiploc;
    int    departureTime;
    int    lastTime = 0;
    
    const string& tripRid = journey.get<string>("<xmlattr>.rid");
    
    for(const pt::ptree::value_type& child: journey){
    
        const string& key = child.first;
        const pt::ptree& stop = child.second;
        
        if (key == "OR" || key == "IP" || key == "DT") {
        
            if (state == LookingForArrival) {
                optional<int> pta = GetPlannedArrivalTime(stop);
                if (pta != boost::none) {
                    const string& arrivalTiploc = GetStopTiploc(stop);
                    WriteConnection(out, departureTime, lastDepartureTiploc, *pta, arrivalTiploc, tripRid);
                    state = LookingForDeparture;
                }
            }

            if (state == LookingForDeparture) {
                optional<int> ptd = GetPlannedDepartureTime(stop);
                if (ptd != boost::none) {
                    lastDepartureTiploc = GetStopTiploc(stop);
                    departureTime = *ptd;
                    state = LookingForArrival;
                }
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
