bool sdBeginCheck() // SD挂载检查
{
  pinMode(SD_POW, OUTPUT);
  digitalWrite(SD_POW, SD_POW_ON);
  hspi.end();
  SD_MMC.end();
  /*pinMode(SD_CS, OUTPUT);
  pinMode(SD_SCK, OUTPUT);
  pinMode(SD_MISO, OUTPUT);
  pinMode(SD_MOSI, OUTPUT);
  digitalWrite(SD_CS, 0);
  digitalWrite(SD_SCK, 0);
  digitalWrite(SD_MISO, 0);
  digitalWrite(SD_MOSI, 0);*/
  if (SD_MMC.begin("/cdcard", true, 800 * 1000)) //
  {
    Serial.println("SDMMC Ok");
    sdInitOk = 1;
    return 1;
  }
  else
  {
    Serial.println("SDMMC Fail");
    sdInitOk = 0;
    return 0;
  }
}

bool sdBeginCheck2() // SD挂载检查
{
  if (sdInitOk) return 1;

  digitalWrite(SD_POW, SD_POW_ON);
  hspi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS); // SCK,MISO,MOSI,SS
  if (!sd.begin(SdSpiConfig(SD_CS, DEDICATED_SPI, SD_SCK_MHZ(10), &hspi)))
  {
    // sd.initErrorHalt(&Serial); //打印错误并停止
    sdInitOk = 0;
    Serial.println("SdFat Fail");
    return 0;
  }
  else
  {
    sdInitOk = 1;
    Serial.println("SdFat Ok");
    return 1;
  }
}

void sd_off()
{
  sd.end();
  hspi.end();
  delay(30);
  digitalWrite(SD_POW, SD_POW_OFF);
  sdInitOk = 0;
}
bool getSDInfo(FSInfo& info)
{
  /*Serial.println("正在扫描FAT，请稍候:");
  if (sd.fatType() <= 32)
  {
    Serial.print("卷为FAT");Serial.println(sd.fatType());
  }
  else
  {
     Serial.println("卷为exFAT:");
  }*/
 
  /*Serial.print("每簇的扇区数:"); Serial.println(sd.sectorsPerCluster());
  Serial.print("fat起始扇区:"); Serial.println(sd.fatStartSector());
  Serial.print("数据起始扇区:"); Serial.println(sd.dataStartSector());
  Serial.print("集群计数:"); Serial.println(sd.clusterCount());
  Serial.print("自由群集计数:"); Serial.println(freeClusterCount);
  Serial.println();*/

  info.fatType = sd.fatType();
  if (info.fatType == -1) return 0;
  uint64_t freeClusterCount = sd.freeClusterCount();           // 统计自由簇计数
  info.blockSize = sd.sectorsPerCluster();                     // 统计每个集群的扇区数（块大小）
  info.totalBytes = 5.12 * sd.clusterCount() * info.blockSize; // 总大小
  info.usedBytes = info.totalBytes-(info.blockSize * freeClusterCount * 5.12); // 所有文件占用的大小 = 总大小 -（自由簇计数*块大小）
 
  Serial.print("总大小:"); Serial.println(info.totalBytes);
  Serial.print("自由簇计数:"); Serial.println(freeClusterCount);
  Serial.print("每个集群的扇区数:"); Serial.println(info.blockSize);
  Serial.print("所有文件占用的大小:"); Serial.println(info.usedBytes);
  return 1;
}

uint64_t getTotalBytes() //获取内存卡容量
{
  uint64_t TotalBytes = 5.12 * sd.clusterCount() * sd.sectorsPerCluster(); // 总大小;
  return TotalBytes;
}
