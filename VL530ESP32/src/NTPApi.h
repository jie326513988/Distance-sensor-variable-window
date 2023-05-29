bool ntpConnect() //ntp连接程序 0-失败 1-成功
{
  uint8_t update_count = 1;
  timeClient.begin();
  while (timeClient.update() == 0 && update_count <= 4)
  {
    delay(1000);
    timeClient.setPoolServerName(NTPadd[update_count]);
    update_count++;
  }
  if (update_count > 4) return 0;
  return 1;
}

bool ntpToClockChip() //将NTP数据写入时钟芯片内
{
  if (!clockChipExist())  return 0;
  
  ntpDate date;
  getNTPDate(timeClient.getEpochTime(), &date); // 获取日期

  defaultDateTime.hour = timeClient.getHours();     // 时
  defaultDateTime.minute = timeClient.getMinutes(); // 分
  defaultDateTime.second = timeClient.getSeconds(); // 秒
  defaultDateTime.dayOfWeek = timeClient.getDay();  // 星期
  // eep_week = "周" + weekDigitalToChinese(timeClient.getDay());
  defaultDateTime.dayOfMonth = date.day; // 日
  defaultDateTime.month = date.month;    // 月
  defaultDateTime.year = date.year;      // 年
  adapter.writeDateTime(defaultDateTime);
  return 1;
}

void getNTPDate(uint32_t timeStamp, struct ntpDate *jgt) //获取NTP的日期
{
  unsigned int year = START_YEAR;
  while (1) {
    uint32_t seconds;
    if (isLeapYear(year)) seconds = SECONDS_IN_DAY * 366;
    else seconds = SECONDS_IN_DAY * 365;
    if (timeStamp >= seconds) {
      timeStamp -= seconds;
      year++;
    } else break;
  }

  unsigned int month = 0;
  while (1)
  {
    uint32_t seconds = SECONDS_IN_DAY * days_in_month[month];
    if (isLeapYear(year) && month == 1) seconds = SECONDS_IN_DAY * 29;
    if (timeStamp >= seconds) {
      timeStamp -= seconds;
      month++;
    } else break;
  }
  month++;

  unsigned int day = 1;
  while (1) {
    if (timeStamp >= SECONDS_IN_DAY) {
      timeStamp -= SECONDS_IN_DAY;
      day++;
    } else break;
  }

  jgt->year = year - 2000; // 年
  jgt->month = month;      // 月
  jgt->day = day;          // 日

  // unsigned int hour = timeStamp / 3600;
  // unsigned int minute = (timeStamp - (uint32_t)hour * 3600) / 60;
  // unsigned int second = (timeStamp - (uint32_t)hour * 3600) - minute * 60;

  // eep_year = String(year);   // 年
  // eep_month = String(month); // 月
  // eep_day = String(day);     // 日

  /*Serial.println("当前日期和时间:");

    if (day < 10) Serial.print("0");
    Serial.print(day);
    Serial.print("/");

    if (month < 10) Serial.print("0");
    Serial.print(month);
    Serial.print("/");

    Serial.println(year);

    if (hour < 10) Serial.print("0");
    Serial.print(hour);
    Serial.print(":");

    if (minute < 10) Serial.print("0");
    Serial.print(minute);
    Serial.print(":");

    if (second < 10) Serial.print("0");
    Serial.println(second);

    Serial.println();*/
}
boolean isLeapYear(unsigned int year) //判断是否闰年
{
  return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

String weekDigitalToChinese(int digital) //星期数字1234567转中文
{
  if (digital == 1)      return F("一");
  else if (digital == 2) return F("二");
  else if (digital == 3) return F("三");
  else if (digital == 4) return F("四");
  else if (digital == 5) return F("五");
  else if (digital == 6) return F("六");
  else if (digital == 0) return F("日");
  return F("error:digitalToChinese");
}