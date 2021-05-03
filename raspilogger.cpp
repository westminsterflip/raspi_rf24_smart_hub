#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <algorithm>
#include <InfluxDB.h>
#include <InfluxDBFactory.h>
#include <iostream>
#include <json.hpp>

using namespace std;

//keep short names 2 char
struct AirQuality
{
    static const std::string fullName;
    static const std::string shortName;
    static const std::string flagName;
};
const std::string AirQuality::fullName = "Air_Quality";
const std::string AirQuality::shortName = "aq";
const std::string AirQuality::flagName = "qualflag";

struct Thermometer
{
    static const std::string fullName;
    static const std::string shortName;
    static const std::string flagName;
};
const std::string Thermometer::fullName = "Thermometer";
const std::string Thermometer::shortName = "tm";
const std::string Thermometer::flagName = "tempunit";

struct Vent
{
    static const std::string fullName;
    static const std::string shortName;
    static const std::string flagName;
};
const std::string Vent::fullName = "Vent";
const std::string Vent::shortName = "v_";
const std::string Vent::flagName = "istherm";

RF24 radio(22, 0);

RF24Network network(radio);
// Address of our node in Octal format
const uint16_t this_node = 00;

static inline std::string &rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         std::not1(std::ptr_fun<int, int>(std::isspace)))
                .base(),
            s.end());
    return s;
}

bool is_number(const std::string &s)
{
    return !s.empty() && std::find_if(s.begin(),
                                      s.end(), [](unsigned char c) { return (!std::isdigit(c) && c != '.'); }) == s.end();
}

std::string getFullName(std::string shortName)
{
    if (shortName == AirQuality::shortName)
    {
        return AirQuality::fullName;
    }
    else if (shortName == Thermometer::shortName)
    {
        return Thermometer::fullName;
    }
    else if (shortName == Vent::shortName)
    {
        return Vent::fullName;
    }
    else
    {
        return "";
    }
}

// std::string getShortName(std::string fullName)
// {
//     if (fullName == AirQuality::fullName)
//     {
//         return AirQuality::shortName;
//     }
//     else if (fullName == Thermometer::fullName)
//     {
//         return Thermometer::shortName;
//     }
//     else if (fullName == Vent::fullName)
//     {
//         return Vent::shortName;
//     }
//     else
//     {
//         return "";
//     }
// }

std::string getFlagName(std::string name)
{
    if (name.length() == 2)
    {
        if (name == AirQuality::shortName)
        {
            return AirQuality::flagName;
        }
        else if (name == Thermometer::shortName)
        {
            return Thermometer::flagName;
        }
        else if (name == Vent::shortName)
        {
            return Vent::flagName;
        }
        else
        {
            return "";
        }
    }
    else
    {
        if (name == AirQuality::fullName)
        {
            return AirQuality::flagName;
        }
        else if (name == Thermometer::fullName)
        {
            return Thermometer::flagName;
        }
        else if (name == Vent::fullName)
        {
            return Vent::flagName;
        }
        else
        {
            return "";
        }
    }
}

int main(int argc, char **argv)
{
    try
    {
        auto influxdb = influxdb::InfluxDBFactory::Get("http://localhost:8086?db=airquallog");
        bool rbegan = radio.begin();
        if (!rbegan)
        {
            printf("radio failed to begin\n");
            exit(1);
        }
        delay(5);
        network.begin(90, this_node);

        RF24NetworkHeader header;

        printf("Looking for data...\n");
        while (1)
        {
            network.update();
            while (network.available())
            {
                time_t cur_time = time(NULL);
                char data[7]; //data format {2 char type code}{max 3 digit info}{1 char flag}
                network.read(header, &data, sizeof(data));
                printf("%s ard%i %s\n", ctime(&cur_time), header.from_node, data);
                string dataString(data);
                string tmp = dataString.substr(2, 3);
                rtrim(tmp);
                int volt = -1;
                if (is_number(tmp))
                {
                    volt = stoi(tmp);
                    string_view cleanString{&data[5], 1};
                    string sourceType = dataString.substr(0, 2);
                    influxdb->write(influxdb::Point{getFullName(sourceType)}.addField("value", volt).addTag("source", to_string(header.from_node)).addTag(getFlagName(sourceType), cleanString));
                }
            }
        }
        //Could join thread here but will never be reached
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << "\n";
    }
}