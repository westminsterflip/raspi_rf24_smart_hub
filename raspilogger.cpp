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
#include <thread>
#include <math.h>
#include "OledI2C.h"
#include "OledFont8x8.h"
#include "OledGraphics.h"

using namespace std;

#define OLEDHEIGHT 64
//#define OLEDHEIGHT 32

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
const std::string Vent::flagName = "";

RF24 radio(22, 0);
uint8_t oledAddrs[] = {0x3C, 0x3D}; //I2C addresses for SSH1306

std::string titles[] = {"", ""};

std::string oledBus[] = {"", ""};

std::string oledMode[] = {"", ""};

int dataSourceNodes[2];

time_t last_update = time(NULL);

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

std::string getShortName(std::string fullName)
{
    if (fullName == AirQuality::fullName)
    {
        return AirQuality::shortName;
    }
    else if (fullName == Thermometer::fullName)
    {
        return Thermometer::shortName;
    }
    else if (fullName == Vent::fullName)
    {
        return Vent::shortName;
    }
    else
    {
        return "";
    }
}

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

void setOLEDMode(int oled, std::string bus, std::string title, std::string mode, int dataSourceNode)
{
    oledBus[oled] = bus;
    oledMode[oled] = mode;
    dataSourceNodes[oled] = dataSourceNode;
    if (title.length() <= 16)
    {
        titles[oled] = title;
    }
}

void updateOLEDs()
{
    auto influxdb = influxdb::InfluxDBFactory::Get("http://localhost:8086?db=airquallog");

    for (int i = 0; i < sizeof(oledAddrs) / sizeof(oledAddrs[0]); i++)
    {
        SSD1306::OledI2C oled{oledBus[i], oledAddrs[i]};
        oled.clear();
        drawString8x8(SSD1306::OledPoint{0, 0}, titles[i], SSD1306::PixelStyle::Set, oled);
        oled.displayUpdate();
    }

    while (1)
    {
        if (time(NULL) - last_update >= 15)
        {
            last_update = time(NULL);
            for (int i = 0; i < sizeof(oledAddrs) / sizeof(oledAddrs[0]); i++)
            {
                SSD1306::OledI2C oled{oledBus[i], oledAddrs[i]};
                oled.clear();
                float maxlogi = 0;
                std::vector<influxdb::Point> log = influxdb->query("SELECT max(mean) FROM (SELECT mean(value) FROM " + oledMode[i] + " WHERE (source = '" + to_string(dataSourceNodes[i]) + "') AND time >= now() -2h GROUP BY time(1m))");
                if (log.size() > 0)
                {
                    influxdb::Point maxlog = log.at(0);
                    string maxlogs = maxlog.getFields();
                    maxlogs = maxlogs.substr(maxlogs.find('=') + 1, maxlogs.find('f'));

                    if (is_number(maxlogs))
                        maxlogi = stof(maxlogs);
                }
                int graphsize = 127 - 8 * ceil(log10((maxlogi == 0) ? 1 : maxlogi));
                std::vector<influxdb::Point> pointset = influxdb->query("SELECT mean(value) FROM " + oledMode[i] + " WHERE (source = '" + to_string(dataSourceNodes[i]) + "') AND time >= now() -2h GROUP BY time(1m) ORDER BY time DESC LIMIT " + to_string(graphsize));
                drawString8x8(SSD1306::OledPoint{0, 0}, titles[i], SSD1306::PixelStyle::Set, oled);
                drawString8x8(SSD1306::OledPoint{0, 8}, to_string((int)ceil(maxlogi)), SSD1306::PixelStyle::Set, oled);
                drawString8x8(SSD1306::OledPoint{8 * floor(log10((maxlogi == 0) ? 1 : maxlogi)), OLEDHEIGHT - 8}, "0", SSD1306::PixelStyle::Set, oled);
                int oledpointx = 127;
                if (maxlogi == 0 || log.size() == 0)
                {
                    SSD1306::line(SSD1306::OledPoint(8, OLEDHEIGHT - 1), SSD1306::OledPoint(127, OLEDHEIGHT - 1), SSD1306::PixelStyle::Set, oled);
                }
                else
                {
                    for (auto point : pointset)
                    {
                        string points = point.getFields();
                        points = points.substr(points.find('=') + 1, points.find('f'));
                        float pointf = 0;
                        if (is_number(points))
                        {
                            pointf = stof(points);
                        }
                        int oledpointy = round(OLEDHEIGHT - 1 - (pointf / maxlogi * (OLEDHEIGHT - 10))); //top of highest line 1 pixel from bottom of title
                        SSD1306::line(SSD1306::OledPoint(oledpointx, OLEDHEIGHT - 1), SSD1306::OledPoint(oledpointx, oledpointy), SSD1306::PixelStyle::Set, oled);
                        oledpointx--;
                    }
                }
                oled.displayUpdate();
            }
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

        setOLEDMode(0, "/dev/i2c-1", "Print Air Qual:", AirQuality::fullName, 2);
        setOLEDMode(1, "/dev/i2c-1", "Room Air Qual:", AirQuality::fullName, 1);

        std::thread updateoleds(updateOLEDs);

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