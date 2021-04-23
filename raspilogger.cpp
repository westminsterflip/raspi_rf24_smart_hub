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
#include "OledFont8x8.h"
#include "OledI2C.h"
#include <thread>
#include <math.h>
#include "OledGraphics.h"

using namespace std;

RF24 radio(22,0);
uint8_t oledAddr1 = 0x3C; //Printer internal
uint8_t oledAddr2 = 0x3D; //External

time_t last_update = time(NULL);

RF24Network network(radio);
// Address of our node in Octal format
const uint16_t this_node = 00;

//auto influxdb1 = influxdb::InfluxDBFactory::Get("http://localhost:8086?db=airquallog");

static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

bool is_number(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(),
        s.end(), [](unsigned char c) { return (!std::isdigit(c)&&c!='.'); }) == s.end();
}

void updateoleds(){
    auto influxdb = influxdb::InfluxDBFactory::Get("http://localhost:8086?db=airquallog");

    SSD1306::OledI2C oled1{"/dev/i2c-1", oledAddr1};
    SSD1306::OledI2C oled2{"/dev/i2c-1", oledAddr2};
    oled1.clear();
    oled2.clear();
    drawString8x8(SSD1306::OledPoint{0,0},"Print Air Qual:",SSD1306::PixelStyle::Set,oled1);
    drawString8x8(SSD1306::OledPoint{0,0},"Room Air Qual:",SSD1306::PixelStyle::Set,oled2);
    oled1.displayUpdate();
    oled2.displayUpdate();

    while(1){
        if(time(NULL)-last_update >= 15){
            last_update = time(NULL);
            oled1.clear();
            oled2.clear();

            influxdb::Point maxlog1 = influxdb->query("SELECT max(mean) FROM (SELECT mean(value) FROM Air_Quality WHERE (source = '2') AND time >= now() -2h GROUP BY time(1m))").at(0);
            influxdb::Point maxlog2 = influxdb->query("SELECT max(mean) FROM (SELECT mean(value) FROM Air_Quality WHERE (source = '1') AND time >= now() -2h GROUP BY time(1m))").at(0);
            string maxlogs1 = maxlog1.getFields();
            string maxlogs2 = maxlog2.getFields();
            maxlogs1=maxlogs1.substr(maxlogs1.find('=')+1,maxlogs1.find('f'));
            maxlogs2=maxlogs2.substr(maxlogs2.find('=')+1,maxlogs2.find('f'));
            float maxlogi1 = 0;
            float maxlogi2 = 0;
            //drawString8x8(SSD1306::OledPoint{0,16},maxlog1.getFields(),SSD1306::PixelStyle::Set,oled1);
            //drawString8x8(SSD1306::OledPoint{0,16},maxlog2.getFields(),SSD1306::PixelStyle::Set,oled2);
            if(is_number(maxlogs1)){
                maxlogi1 = stof(maxlogs1);
            }
            if(is_number(maxlogs2)){
                maxlogi2 = stof(maxlogs2);
            }
            int graphsize1 = 127-8*ceil(log10((maxlogi1==0)?1:maxlogi1));
            int graphsize2 = 127-8*ceil(log10((maxlogi2==0)?1:maxlogi2));
            std::vector<influxdb::Point> points1 = influxdb->query("SELECT mean(value) FROM Air_Quality WHERE (source = '2') AND time >= now() -2h GROUP BY time(1m) ORDER BY time DESC LIMIT "+to_string(graphsize1));
            std::vector<influxdb::Point> points2 = influxdb->query("SELECT mean(value) FROM Air_Quality WHERE (source = '1') AND time >= now() -2h GROUP BY time(1m) ORDER BY time DESC LIMIT "+to_string(graphsize2));
            drawString8x8(SSD1306::OledPoint{0,0},"Print Air Qual:",SSD1306::PixelStyle::Set,oled1);
            drawString8x8(SSD1306::OledPoint{0,0},"Room Air Qual:",SSD1306::PixelStyle::Set,oled2);
            drawString8x8(SSD1306::OledPoint{0,8},to_string((int)ceil(maxlogi1)),SSD1306::PixelStyle::Set,oled1);
            drawString8x8(SSD1306::OledPoint{0,8},to_string((int)ceil(maxlogi2)),SSD1306::PixelStyle::Set,oled2);
            drawString8x8(SSD1306::OledPoint{8*floor(log10((maxlogi1==0)?1:maxlogi1)),56},"0",SSD1306::PixelStyle::Set,oled1);
            drawString8x8(SSD1306::OledPoint{8*floor(log10((maxlogi2==0)?1:maxlogi2)),56},"0",SSD1306::PixelStyle::Set,oled2);

            int oledpointx = 127;
            if(maxlogi1 == 0){
                SSD1306::line(SSD1306::OledPoint(oledpointx,0),SSD1306::OledPoint(127,0),SSD1306::PixelStyle::Set,oled1);
            }else{
                for(auto point : points1){
                    string points = point.getFields();
                    points = points.substr(points.find('=')+1,points.find('f'));
                    float pointf = 0;
                    if(is_number(points)){
                        pointf = stof(points);
                    }
                    int oledpointy = round(63-(pointf/maxlogi1*54));
                    SSD1306::line(SSD1306::OledPoint(oledpointx,63),SSD1306::OledPoint(oledpointx,oledpointy),SSD1306::PixelStyle::Set,oled1);
                    oledpointx--;
                }
            }
            oledpointx = 127;
            if(maxlogi2 == 0){
                SSD1306::line(SSD1306::OledPoint(oledpointx,0),SSD1306::OledPoint(127,0),SSD1306::PixelStyle::Set,oled2);
            }else{
                for(influxdb::Point point : points2){
                    string points = point.getFields();
                    points = points.substr(points.find('=')+1,points.find('f'));
                    float pointf = 0;
                    if(is_number(points)){
                        pointf = stof(points);
                    }
                    int oledpointy = round(63-(pointf/maxlogi2*54));
                    SSD1306::line(SSD1306::OledPoint(oledpointx,63),SSD1306::OledPoint(oledpointx,oledpointy),SSD1306::PixelStyle::Set,oled2);
                    oledpointx--;
                }
            }
            oled1.displayUpdate();
            oled2.displayUpdate();
        }
    }
}

int main(int argc, char** argv){
    try{
        //SSD1306::OledI2C oled1{"/dev/i2c-1", oledAddr1};
        //SSD1306::OledI2C oled2{"/dev/i2c-1", oledAddr2};
        //drawString8x8(SSD1306::OledPoint{0,0},"Printer Air Qual:",SSD1306::PixelStyle::Set,oled1);
        //drawString8x8(SSD1306::OledPoint{0,0},"Room Air Qual:",SSD1306::PixelStyle::Set,oled2);
        //oled1.displayUpdate();
        //oled2.displayUpdate();

        auto influxdb = influxdb::InfluxDBFactory::Get("http://localhost:8086?db=airquallog");
        bool rbegan = radio.begin();
        if(!rbegan){
            printf("radio failed to begin\n");
            exit(1);
        }
        delay(5);
        network.begin(90, this_node);

	std::thread updateOLEDs (updateoleds);

        RF24NetworkHeader header;

        printf("Looking for data...\n");
        while(1){
            network.update();
            while(network.available()){
                time_t cur_time = time(NULL);
                char data[5];
                network.read(header,&data,sizeof(data));
                printf("%s ard%i %s\n",ctime(&cur_time),header.from_node,data);
                string dataString(data);
                string tmp = dataString.substr(0,3);
                rtrim(tmp);
                int volt = -1;
                if(is_number(tmp)){
                    volt = stoi(tmp);
                    string_view cleanString{&data[3],1};
                    influxdb->write(influxdb::Point{"Air_Quality"}.addField("value",volt).addTag("source", to_string(header.from_node)).addTag("qualflag",cleanString));
                }
            }
        }
	//Could join thread here but will never be reached
    }catch (std::exception& e){
        std::cerr << e.what() << "\n";
    }
}