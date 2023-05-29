bool clockChipReadCheck() // 读取时钟数据是否正常
{
    i2c_init();
    dateTime = adapter.readDateTime();
    if (dateTime.year < 20 || dateTime.year > 99)
        return 0;
    else if (dateTime.month <= 0 || dateTime.month > 12)
        return 0;
    else if (dateTime.dayOfMonth <= 0 || dateTime.dayOfMonth > 31)
        return 0;
    else if (dateTime.hour < 0 || dateTime.hour > 24)
        return 0;
    else if (dateTime.minute < 0 || dateTime.minute > 60)
        return 0;
    else if (dateTime.second < 0 || dateTime.second > 60)
        return 0;
    else
        return 1;
    return 0;
}
bool clockChipPowerDown() // 检查是否掉电了
{
    i2c_init();
    if (adapter.initAdapter()) // 此函数检查VLF-bit是否为 1  当读到的 VLF-bit 为 1 的时候，必须对 RTC 进行初始化。
        return 1;
    return 0;
}
bool clockChipExist() // 检查时钟芯片是否存在
{
    //i2c_init();
    Wire.beginTransmission(RX8010_I2C_ADDR); // 从指定的地址开始向I2C从设备进行传输
    if (Wire.endTransmission() == 0) return 1;
    return 0;
    // Wire.endTransmission()返回结果：
    // 0: 成功
    // 1: 数据量超过传送缓存容纳限制
    // 2: 传送地址时收到 NACK
    // 3: 传送数据时收到 NACK
    // 4: 其它错误
}
