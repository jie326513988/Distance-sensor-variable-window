// 声明外部字体常量
extern const uint8_t chinese_gb2312[254243] U8G2_FONT_SECTION("chinese_gb2312");


// main
void RotarEncoder();
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void listDir_SdFat(String dirname, uint8_t levels);

// Other.h
void esp_sleep(uint32_t minutes); // 系统休眠
uint8_t print_wakeup_reason();    // 获取休眠唤醒原因

String getEspChipId();
void setKeyInterrupt(bool en);
void i2c_init();                           // i2c初始化

// sdinit.h
bool sdBeginCheck();             // SD挂载检查
bool sdBeginCheck2();            // SD挂载检查
bool getSDInfo(FSInfo &info); // 获取SD卡容量信息
void sd_off();
uint64_t getTotalBytes();

// STA_AT_OTA.h
bool connectToWifi(); // 初始化STA配置
void initApSTA();     // 初始化AP配置
void initApSTANoBegin();
void initAp();
boolean scanNetworks(String currentSSID);

// CallHttps.h
String callHttps(String url);
String callHttp(String url);
// NTPApi.h
bool ntpConnect();                                        // ntp连接程序 0-失败 1-成功
bool ntpToClockChip();                                    // 将NTP数据写入时钟芯片内
void getNTPDate(uint32_t timeStamp, struct ntpDate *jgt); // 获取NTP的日期
bool isLeapYear(unsigned int year);                       // 判断是否闰年
String weekDigitalToChinese(int digital);                 // 星期数字1234567转中文

// key.h
uint8_t get_key(uint8_t key_pin);
bool key_lb(uint8_t key,uint16_t lb_max);
void keyRunMode();

//Clock8010.h
bool clockChipReadCheck();  // 读取时钟数据是否正常
bool clockChipPowerDown();  // 检查是否掉电了
bool clockChipExist();      // 检查时钟芯片是否存在
