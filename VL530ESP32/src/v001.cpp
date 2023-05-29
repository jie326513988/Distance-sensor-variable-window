//****** 引用库 ******
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include "SPI.h"

#include <U8g2lib.h> //OLED图形驱动库
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 18, /* data=*/ 23);

#if !(USING_DEFAULT_ARDUINO_LOOP_STACK_SIZE)
  uint16_t USER_CONFIG_ARDUINO_LOOP_STACK_SIZE = 16*1024;
#endif

#include <FS.h>
//#include "SD.h"
#include "SD_MMC.h"
#include "SdFat.h"
#include "sd_fat32_fs_wrapper.h"
SPIClass hspi(HSPI);
//#define SD_CONFIG SdSpiConfig(SD_CS, DEDICATED_SPI, SD_SCK_MHZ(8), &hspi)
//#define SD_CONFIG SdSpiConfig(SD_CS, SHARED_SPI, SD_SCK_MHZ(12), &hspi)
SdFs sd;
fs::FS sdFat = fs::FS(fs::FSImplPtr(new SdFat32FSImpl(sd)));

struct FSInfo
{
  size_t totalBytes; // 整个文件系统的大小
  size_t usedBytes;  // 已用大小
  size_t blockSize;  // 块大小
  int8_t fatType;    // 卡类型 16=FAT16 32=FAT32 -1=识别失败 0=无卡
};FSInfo info;

#include <Preferences.h>
Preferences eep;

#include <RX8010SJ.h>
#define RX8010_I2C_ADDR 0x32
const char * dayOfWeekStrings[] = {
	"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
};
RX8010SJ::Adapter adapter = RX8010SJ::Adapter(RX8010_I2C_ADDR);
RX8010SJ::DateTime defaultDateTime = RX8010SJ::DateTime();

#include <NTPClient.h>
#include <WiFiUdp.h>
//ntp时间设置
#define SECONDS_IN_DAY          86400 // 一天中的秒数
#define START_YEAR              1970  // 开始年份
#define TIME_ZONE               +8    // 默认时区
static uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const char* NTPadd[5] = {"ntp.ntsc.ac.cn",
                         "ntp1.aliyun.com",
                         "s2k.time.edu.cn",
                         "time1.cloud.tencent.com",
                         "cn.ntp.org.cn"
                        };
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTPadd[0], TIME_ZONE * 3600, 60000); //udp，服务器地址，时间偏移量，更新间隔

#include <ArduinoJson.h>
#include <HTTPUpdateServer.h>

WebServer server(80);
HTTPUpdateServer httpUpdater;
File fsUploadFile;      // 建立文件对象用于闪存文件上传
//SdFile fsUploadFile;      // 建立文件对象用于闪存文件上传
static bool fsOK;
String unsupportedFiles = "";
static const char TEXT_PLAIN[] PROGMEM = "text/plain";
static const char FS_INIT_ERROR[] PROGMEM = "FS INIT ERROR";
static const char FILE_NOT_FOUND[] PROGMEM = "FileNotFound";

//****** STA设置
//char sta_ssid[32] = {0};
//char sta_password[64] = {0};
IPAddress dns1(114, 114, 114, 114);
IPAddress dns2(114, 114, 115, 115);
//****** AP设置
const char *ap_ssid = "ESP32 E-Paper";
const char *ap_password = "333333333"; // 无密码则为开放式网络 9个3
IPAddress local_IP(192, 168, 3, 3);
IPAddress gateway(192, 168, 3, 3);
IPAddress subnet(255, 255, 255, 0);

//****** 可调参数 & 引脚配置 ******
const String version = "ESP32_EPD_V006"; // 程序版本
#define DBG_OUTPUT_PORT Serial
// 电池相关
#define BAT_LOW1 3.2 // 低压提示和永不唤醒电压
#define BAT_LOW2 3.1 // 不提示直接休眠的电压
#define BAT_PIN 34   // 读取电池电压引脚，不能超过1V
// 墨水屏接口
// V0.9
#define EPD_BUSY 13
#define EPD_RES 4
#define EPD_DC 16
#define EPD_CS 19
#define EPD_SCLK 23
#define EPD_SDI 18

// 按键相关
#define re_lb_max 30 // 旋转编码器滤波次数
#define KEY_OK 25    // 确定键
//#define KEY_A 32   // 编码器A
//#define KEY_B 33   // 编码器B
#define KEY_A 33   // 编码器A
#define KEY_B 32   // 编码器B
#define KEY_BACK 0 // 返回键
// SD卡相关
#define SD_CS 26           // SD卡片选开关
#define SD_SCK 14          //
#define SD_MISO 2          //
#define SD_MOSI 15         //
#define SD_POW 5           // SD卡电源开关 低电平打开 高电平关闭
#define SPI_SPEED 20000000 // SD卡频率
#define SD_POW_ON 0        // SD卡电源开关，0开启，1关闭
#define SD_POW_OFF 1       // SD卡电源开关，0开启，1关闭
// 音频相关
#define AUDIO_POW 12
#define AUDIO_SCL 23
//#define AUDIO_SDA 13    // V0.8
#define AUDIO_SDA 18    // V0.3 V0.92 v0.93
#define AUDIO_SCLK 22
#define AUDIO_DSDIN 17
#define AUDIO_LRCK 21
#define AUDIO_POW_GF 27
#define AUDIO_PJDET 35

//I2C地址 0x44-sht30 0x18-es8311 0x32-rx8010

//****** 一些变量 ******
String language = "zh-Hans";  // 请求语言
String url_yiyan = "https://v1.hitokoto.cn/?encode=json&min_length=1&max_length=22";//一言获取地址

uint8_t wakeup_reason = 255; // 唤醒原因

bool displayUpdate = 1;      // 屏幕刷新状态 1-可以刷新 0-正在刷新
bool qpsx = 1;               // 全屏刷新
float bat_vcc = 0.0;         // 电池电压
size_t overTime = 0;         // 超时对比时间
size_t zdfy_sec = 0;         // 自动翻页对比时间
uint32_t getvcc_time = 0;    // 获取电池电压时间
bool displayRunModState = 0; // 显示模式选择界面的状态 1-显示 2-不显示
uint8_t pageUpdataCount = 0; // 控制全刷的计数
bool sdInitOk = 0;           // SD卡挂载成功状态 1-成功 0-失败
bool getWebWeather = 0;      // web是否需要获取天气 0-不需要 1-需要
// 硬件时钟芯片是否正常
#define szxp_ok bitRead(RTC_clock_code, 0) == 1 && bitRead(RTC_clock_code, 1) == 1

struct ntpDate //ntp获取日期的结构体
{
  uint16_t year, month, day;
};

bool RE_initState;    // 旋转编码器 初始状态
bool RE_currentState; // 旋转编码器 当前状态
uint32_t RE_time;     // 旋转编码器 滤波时间

RX8010SJ::DateTime dateTime;

// 声明外部文件
//#include "gb2312.c"
#include "FontBmp.h"
#include "Other.h"
#include "STA_AT_OTA.h" 
#include "CallHttps.h"
#include "key.h"
#include "Clock8010.h"
#include "NTPApi.h"
#include "SDinit.h"

#include "Adafruit_VL53L0X.h"
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
VL53L0X_RangingMeasurementData_t measure;
int Filter4();
int Filter();
uint16_t getJL(); // 获取传感器距离
int yyyy(uint16_t *arr, uint16_t len);

uint32_t time0 = 0;       // 运行一次的时间
uint16_t jl_y[128] = {0}; // 距离数组
uint8_t posMax = 0;       // 数组当前位置
uint16_t jl_min, jl_max;  // 距离最大值最小值
uint32_t zf = 0;          // 振幅
uint32_t time1 = 0;       // 记录振幅的对比时间

bool clockChipOK = 0; // 时钟芯片是否存在 0-不存在 1-存在

File fileSd;          // 记录波形的文件对象
String fileName;      // 记录波形的文件名称
bool sdWrite = 0;     // SD卡写入状态

uint16_t bf_czlj = 0;   // 所有波峰差值累积
int16_t bf_pos_old = 0; // 上一个波峰的位置
uint16_t bf_conut = 0;  // 波峰数量
bool bg_state = 1;      // 波谷状态
float pjzqjs = 0.0;     // 平均周期个数
float pjzqsj = 0.0;     // 平均周期时间

uint8_t bg_count = 0;   // 波谷计数
bool jl_time_state = 1;
uint32_t jl_time=0;

void setup()
{
  WiFi.mode(WIFI_OFF); //  设置工作模式

  pinMode(SD_POW, OUTPUT);

  // 设置按键为上拉模式
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);
  pinMode(KEY_OK, INPUT_PULLUP);
  pinMode(KEY_BACK, INPUT_PULLUP);
  RE_initState = digitalRead(KEY_A);

  // 文件系统初始化
  /*fsOK = LittleFS.begin(1);
  if (!fsOK)
  {
    Serial.println("LittleFS 启动失败");

    esp_sleep(0);
  }*/

  Serial.begin(115200);
  delay(1000);

  wakeup_reason = print_wakeup_reason();

  Wire.end();
  Wire.begin(18, 23);

  //* OLED初始化 */
  u8g2.begin();
  u8g2.setFont(chinese_gb2312); // 适合本项目的中文库
  u8g2.setContrast(50);    // 设置亮度
  u8g2.enableUTF8Print();  // 开启UTF8字符功能
  // u8g2.setFontMode(1);
  u8g2.setBusClock(800000);
  // 打开按键中断
  // setKeyInterrupt(1);

  if (!lox.begin())
  {
    u8g2.clearBuffer(); //
    u8g2.drawButtonUTF8(64, 20, U8G2_BTN_HCENTER | U8G2_BTN_BW2, 34, 2, 4, "无法启动VL53L0X");
    u8g2.sendBuffer();
    Serial.println(F("无法启动VL53L0X"));
    esp_sleep(0);
  }

  u8g2.clearBuffer(); //
  u8g2.drawButtonUTF8(64, 20, U8G2_BTN_HCENTER | U8G2_BTN_BW2, 34, 2, 4, "靠近传感器启动系统");
  u8g2.sendBuffer();

  for (uint8_t i = 0; i < 5; i++)
  {
    Filter4();
  }

  // 判断什么时候需要挂载SD卡
  if (sdBeginCheck2())
  {
    u8g2.clearBuffer(); //
    u8g2.drawButtonUTF8(64, 20, U8G2_BTN_HCENTER | U8G2_BTN_BW2, 34, 2, 4, "SD卡加载成功");
    u8g2.sendBuffer();
  }
  else 
  {
    u8g2.clearBuffer(); //
    u8g2.drawButtonUTF8(64, 20, U8G2_BTN_HCENTER | U8G2_BTN_BW2, 34, 2, 4, "SD卡加载失败");
    u8g2.sendBuffer();
  }
  delay(500);
  if (get_key(KEY_OK) == 2)
  {
    u8g2.clearBuffer();
    u8g2.setCursor(0, 14);
    u8g2.print("连接wifi：ACE 2");
    u8g2.sendBuffer();
    if (connectToWifi())
    {
      u8g2.setCursor(100, 14);
      u8g2.print("成功");
      u8g2.sendBuffer();
      if (clockChipExist()) // 检查时钟芯片是否存在
      {
        u8g2.setCursor(0, 28);
        u8g2.print("时钟芯片存在");
        u8g2.setCursor(0, 42);
        u8g2.print("NTP服务器连接");
        u8g2.sendBuffer();
        if (ntpConnect()) // 连接NTP服务器
        {
          u8g2.setCursor(0, 42);
          u8g2.print("NTP服务器连接成功");
          u8g2.sendBuffer();
          if (ntpToClockChip()) // 数据写入时钟芯片
          {
            u8g2.setCursor(0, 58);
            u8g2.print("写入时钟芯片成功");
            u8g2.sendBuffer();
          }
          else //写入时钟芯片失败
          {
            u8g2.setCursor(0, 58);
            u8g2.print("写入时钟芯片失败");
            u8g2.sendBuffer();
          }
        }
        else //NTP服务器连接失败
        {
          u8g2.setCursor(0, 42);
          u8g2.print("NTP服务器连接失败");
          u8g2.sendBuffer();
        }
      }
      else //时钟芯片不存在
      {
        u8g2.setCursor(0, 28);
        u8g2.print("时钟芯片不存在");
        u8g2.sendBuffer();
      }
    }
    else //wifi连接失败
    {
      u8g2.setCursor(100, 14);
      u8g2.print("失败");
      u8g2.sendBuffer();
    }
    delay(2000);
  }
  if (clockChipExist())
  {
    clockChipOK = 1;
    dateTime = adapter.readDateTime();
    String time1 = String(dateTime.year + 2000) + "/" + String(dateTime.month) + "/" + String(dateTime.dayOfMonth);
    String time2 = String(dateTime.hour) + ":" + String(dateTime.minute) + ":" + String(dateTime.second);

    // 创建记录文件
    fileName = String(dateTime.year + 2000) +"年"+ String(dateTime.month) +"月"+ String(dateTime.dayOfMonth) +"日"+
                      " " + String(dateTime.hour) + "时" + String(dateTime.minute) + "分" + String(dateTime.second) +"秒";
    fileName += ".csv";
    fileSd = sdFat.open(fileName, "a+");
    u8g2.clearBuffer();
    u8g2.drawButtonUTF8(65, 18, U8G2_BTN_HCENTER | U8G2_BTN_BW1, 34, 2, 4, time1.c_str());
    u8g2.drawButtonUTF8(65, 39, U8G2_BTN_HCENTER | U8G2_BTN_BW1, 34, 2, 4, time2.c_str());
    String ok = "文件创建失败";
    if (fileSd) 
    {
      sdWrite = 1;
      ok = "文件创建成功";
      String in = "实时距离(mm),振幅(mm),周期(ms),时间(ms)";
      fileSd.println(in);
      in = "distance(mm),amplitude(mm),period(ms),time(ms)";
      fileSd.println(in);
    }
    fileSd.close();

    u8g2.drawButtonUTF8(65, 60, U8G2_BTN_HCENTER | U8G2_BTN_BW1, 34, 2, 4, ok.c_str());
    u8g2.sendBuffer();

    //设置文件的创建日期
    if(sdWrite)
    {
      FsFile uf = sd.open(fileName, O_WRITE);
      uint16_t n = dateTime.year + 2000, y = dateTime.month, r = dateTime.dayOfMonth,
               s = dateTime.hour, f = dateTime.minute, m = dateTime.second;
      if(uf.timestamp(T_ACCESS, n, y, r, s, f, m))Serial.println("T_ACCESS");
      if(uf.timestamp(T_CREATE, n, y, r, s, f, m))Serial.println("T_CREATE");
      if(uf.timestamp(T_WRITE, n, y, r, s, f, m))Serial.println("T_WRITE");
      uf.close();
    }

  }
  delay(1500);
}

void loop()
{
  time0 = millis();

  // jl_y[posMax] = Filter4(); // 平均滤波
  // jl_y[posMax] = getJL();  //获取原始数据

  jl_y[posMax] = Filter(); // 动态调参一阶滤波

  //Serial.println(jl_y[posMax]);

  uint32_t jl_sd = jl_y[posMax];
  if (sdWrite && clockChipOK)
  {
    fileSd = sdFat.open(fileName, "a+");
    if (fileSd)
    {
      if (jl_time_state)
      {
        jl_time_state = 0;
        jl_time = millis();
      }
      String in = String(jl_sd) + "," + String(zf) + "," +
                  String(pjzqsj) + "," + String(millis() - jl_time);
      fileSd.println(in);
    }
    fileSd.close();
  }

  /*
  Serial.print("jl_y[" + String(posMax) + "]");
  Serial.println(jl_y[posMax]);
  */

  if (posMax < 127) posMax++;
  else
  {
    // 数组往前移动
    for (uint8_t i = 0; i < 127; i++)
     jl_y[i] = jl_y[i + 1];
  }
  
  // 寻找最小值、最大值
  jl_min = 10000; jl_max = 10;
  uint8_t i_Max = posMax;
  if (posMax == 127) i_Max = posMax + 1;
  for (uint8_t i = 0; i < i_Max; i++)
  {
    if (jl_y[i] <= jl_min) jl_min = jl_y[i];
    if (jl_y[i] > jl_max)  jl_max = jl_y[i];
    if (jl_max > 1000)     jl_max = 1000;
  }
  zf = jl_max - jl_min; //计算振幅

  /*Serial.print("jl_min:");Serial.println(jl_min);
  Serial.print("jl_max:");Serial.println(jl_max);
  Serial.println();*/

  //****** 显示到屏幕
  u8g2.clearBuffer(); // u8g2.sendBuffer();

  bf_czlj = 0;    // 所有波峰差值累积
  bf_pos_old = 0; // 上一个波峰的位置
  bf_conut = 0;   // 波峰数量
  bg_state = 1;   // 波谷状态 0找不到波谷，1-找到波谷
  //Serial.println();
  uint8_t ii = posMax - 1;
  if (posMax == 127) ii = posMax + 1;
  for (uint8_t i = 0; i < ii; i++)
  {
    if (i <= 126)
    {
     uint16_t x0, x1;
     x0 = map(jl_y[i], jl_min, jl_max, 63, 0);
     x1 = map(jl_y[i + 1], jl_min, jl_max, 63, 0);
     u8g2.drawLine(i, x0, i + 1, x1);
    }
    //******  寻找有几个波峰 波谷
    if (posMax == 127 && i < posMax - 2) // 长度大于一定值才好计算
    {
     // 最高点等于小于上一个点，最高点大于下一个点和大于下下一个点
     if (jl_y[i + 1] >= jl_y[i] && jl_y[i + 1] > jl_y[i + 2] && jl_y[i + 1] > jl_y[i + 3])
     {
        if (bg_state) // 上次找到波谷，这次波峰开始计算
        {
          bg_state = 0;
          int32_t cz = i - bf_pos_old; // 累积波峰差值
          if (bf_pos_old != 0)
          {
            bf_conut++; // 波峰数量+1
            bf_czlj += cz;
            //Serial.print("相邻波峰差值:");Serial.println(cz);
          }
          bf_pos_old = i; // 记录下位置
        }
     }
     else if (jl_y[i] <= jl_y[i - 1] && jl_y[i] <= jl_y[i - 2] && jl_y[i] < jl_y[i + 1])
     {
        bg_state = 1; // 标记找到波谷
     }
     if (bf_czlj > 0)
     {
        pjzqjs = float(bf_czlj) / float(bf_conut);   // 平均周期个数
        pjzqsj = pjzqjs * float((millis() - time0)); // 平均周期时间
     }
    }
    //******  寻找有几个波峰 波谷 END
  }

  u8g2.setCursor(54, 63);
  u8g2.print(jl_sd); // 即时距离

  u8g2.setCursor(0, 63);
  u8g2.print(jl_min); // 最小距离

  u8g2.setCursor(0, 14);
  u8g2.print(jl_max); // 最大距离

  u8g2.setCursor(54, 14);
  u8g2.print(zf); // 距离差值

  u8g2.setCursor(110, 14);
  u8g2.print(millis() - time0); // 测量耗时

  // u8g2.setCursor(100, 30);
  // u8g2.print(bf);

  // uint16_t zhouqi = yyyy(jl_y, posMax + 1);
  u8g2.setCursor(100, 63);
  u8g2.print(pjzqsj,0);

  u8g2.sendBuffer();
  
  //Serial.println(millis() - time0);
}

int Filter4()
{
  int new_value = 0;
  for (uint8_t i = 0; i < 3; i++)
    new_value += getJL();
  return new_value / 3;
}

// 动态调参一阶滤波算法
#define Threshold_1 5 // 阈值1用于一阶带参滤波器，AD大于此值时，计数增加，越大越稳定也越慢
#define Threshold_2 3 // 阈值2用于一阶带参滤波器，计数值大于此值时，减少参数，增强滤波跟随
float K = 0.0;        // 滤波系数
uint16_t num_x = 0;    // 滤波计数器
bool flag = 0;        // 本次数据变化方向
float NEW_DATA = 10;
float OLD_DATA = 0;
int Filter()
{
  // 判断变化方向是否一致 flag=1为一致，flag=0为不一致
  if (NEW_DATA > OLD_DATA)
  {
    NEW_DATA = getJL();
    if (NEW_DATA > OLD_DATA) flag = 1;
    else flag = 0;
  }
  else if (NEW_DATA < OLD_DATA)
  {
    NEW_DATA = getJL();
    if (NEW_DATA < OLD_DATA) flag = 1;
    else flag = 0;
  }
  else
  {
    NEW_DATA = getJL();
    flag = 0;
  }

  if (flag) // 变化方向一致时执行
  {
    num_x++;
    if (abs(NEW_DATA - OLD_DATA) > Threshold_1) // 当变化大于Threshold_1的时候，进行计数器num快速增加，以达到快速增大K值，提高跟随性
    {
      num_x++;
    }
    if (num_x > Threshold_2) // 计数阈值设置，当递增或递减速度达到一定速率时，增大K值
    {
      K = K + 0.25; // 0.2为K的增长值，看实际需要修改
      num_x = 0;
    }
  }
  else
  {
    num_x = 0;
    K = 0.05; // 变化稳定时K值，看实际修改
  }
  OLD_DATA = ((1.0 - K) * OLD_DATA) + (K * NEW_DATA);
  if (OLD_DATA > 65000)
  {
    K = 0.0;   // 滤波系数  
    OLD_DATA = NEW_DATA;
  }
  return OLD_DATA;
}

uint16_t getJL() //获取传感器距离
{
  res:
    lox.rangingTest(&measure, 0); // 传入“true”以获得调试数据打印输出！
    while (measure.RangeStatus == 4) lox.rangingTest(&measure, 0);
    if (measure.RangeMilliMeter > 1000 || measure.RangeMilliMeter < 10)
      goto res;
    return measure.RangeMilliMeter;
}

void RotarEncoder() // 旋转中断
{
  if (millis() - RE_time < 10) return;
  RE_time = millis();

  RE_currentState = key_lb(KEY_A, re_lb_max);


  RE_initState = RE_currentState;
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Serial.printf("正在列出目录: %s\n", dirname);
  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("无法打开目录");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println("不是目录");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  目录 : ");
      Serial.print(file.name());
      time_t t = file.getLastWrite();
      struct tm *tmstruct = localtime(&t);
      Serial.printf("  最后一次写入: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
      if (levels)
      {
        listDir(fs, file.path(), levels - 1);
      }
    }
    else
    {
      Serial.print("  文件: ");
      Serial.print(file.name());
      Serial.print("  大小: ");
      Serial.print(file.size());
      time_t t = file.getLastWrite();
      struct tm *tmstruct = localtime(&t);
      Serial.printf("  最后一次写入: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    }
    file = root.openNextFile();
  }
}

void listDir_SdFat(String dirname, uint8_t levels)
{
  Serial.printf("正在列出目录: %s\n", dirname);
  FsFile root = sd.open(dirname, O_READ);
  if (!root)
  {
    Serial.println("无法打开目录");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println("不是目录");
    return;
  }

  FsFile file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      char file_name_c[128];
      file.getName(file_name_c, 128);
      //String file_name = '/' + String(file_name_c);   
      String file_name = String(file_name_c); 
      Serial.print("  目录 : ");
      Serial.print(file_name);
      uint16_t tm_year = 0, tm_data = 0;
      file.getCreateDateTime(&tm_year,&tm_data); //获取文件的创建日期和时间
      Serial.print(" tm_year:");Serial.print(tm_year); Serial.print("  tm_data:");Serial.print(tm_data);
      Serial.println();
      //struct tm *tmstruct = localtime(&t);
      //Serial.printf("  最后一次写入: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
      if (levels)
      {
        listDir_SdFat(file_name, levels - 1);
      }
    }
    else
    {
      char file_name[128];
      file.getName(file_name, 128);
      Serial.print("  文件: ");
      Serial.print(file_name);
      Serial.print("  大小: ");
      Serial.print(file.size());
      uint16_t tm_year = 0, tm_data = 0;
      file.getCreateDateTime(&tm_year,&tm_data); //获取文件的创建日期和时间
      Serial.print(" tm_year:");Serial.print(tm_year); Serial.print("  tm_data:");Serial.print(tm_data);
      Serial.println();
      //struct tm *tmstruct = localtime(&t);
      //Serial.printf("  最后一次写入: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    }
    file = root.openNextFile();
  }
}

// r 以只读方式操作文件，读位置在文件的开始位置，文件不存在返回空对象；
// r+ 以可读可写方式打开文件，读写位置在文件的开始位置，文件不存在返回空对象；
// w 截取文件长度到0或者创建新文件，只能写操作，写位置在文件的开始位置；
// w+ 截取文件长度到0或者创建新文件，可读可写操作，写位置在文件的开始位置；
// a 在文件末尾追加内容或者文件不存在就创建新文件，追加位置在当前文件的末尾，只能写操作；
// a+ 在文件末尾追加内容或者文件不存在就创建新文件，追加位置在当前文件的末尾，可读写操作；
// 如果模式值是 SeekSet，则从文件开头移动指定的偏移量。
// 如果模式值是 SeekCur，则从目前的文件位置移动指定的偏移量。
// 如果模式值是 SeekEnd，则从文件结尾处移动指定的偏移量。

int yyyy(uint16_t *arr, uint16_t len)
{
  //长度太小，不足以计算
  if (len < 12) return 0;
  uint16_t bf_czlj = 0;   // 所有波峰差值累积
  int16_t bf_pos_old = 0; // 上一个波峰的位置
  uint16_t bf_conut = 0;  // 波峰数量
  bool bg_state = 1;      // 波谷状态
  // 寻找有几个波峰 波谷
  for (uint8_t i = 0; i < len - 3; i++)
  {
    // 最高点等于小于上一个点，最高点大于下一个点和大于下下一个点
    if (arr[i + 1] >= arr[i] && arr[i + 1] > arr[i + 2] && arr[i + 1] > arr[i + 3])
    {
      if (bg_state) // 上次找到波谷，这次波峰开始计算
      {
        bg_state = 0;
        int32_t cz = i - bf_pos_old; // 累积波峰差值
        if (bf_pos_old != 0)
        {
          bf_conut++; // 波峰数量+1
          bf_czlj += cz;
          // Serial.print("相邻波峰差值:");Serial.println(cz);
        }
        bf_pos_old = i; // 记录下位置
      }
    }
    else if (arr[i] <= arr[i - 1] && arr[i] <= arr[i - 2] && arr[i] < arr[i + 1])
    {
      bg_state = 1;   // 标记找到波谷
    }
  }

  if (bf_czlj == 0) return 0;

  float pjzqjs = float(bf_czlj)  / float(bf_conut); //平均周期个数
  float pjzqsj = pjzqjs * float((millis() - time0)) ; //平均周期时间
  /*Serial.print("相邻波峰差值累积:"); Serial.println(bf_czlj);
  Serial.print("波峰数量:"); Serial.println(bf_conut);
  Serial.print("平均周期个数:"); Serial.println(pjzqjs);
  Serial.print("平均周期时间:"); Serial.println(pjzqsj);*/
  return pjzqsj;
}