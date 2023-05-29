#define key_lb_max 15            // 按键滤波次数
// 按键滤波 0-未按下 1-短按 2-长按
uint8_t get_key(uint8_t key_pin) 
{
  uint32_t ca_time = 0;          // 长按的对比时间
  if (digitalRead(key_pin) == 0) // 按键按下为低电平
  {
    // 按键滤波
    uint8_t key_lb_count = 0; // 按键滤波计数
    boolean key_lb_state = 0; // 按键滤波状态 0-未稳定 1-稳定
    while (digitalRead(key_pin) == 0 && key_lb_state == 0)
    {
      key_lb_count++;
      if (key_lb_count >= key_lb_max)
        key_lb_state = 1;
    }
    
    if (key_lb_state) //按键稳定
    {
      ca_time = millis();
      while (digitalRead(key_pin) == 0) // 长短按判断
      {
        // 持续一定时间判断长按
        if (millis() - ca_time > 300)
          return 2;
        delay(1);
      }
      // 持续时间未到规定判断短按
      return 1;
    }
    else //按键未稳定
      return 0;
  }
  else
    return 0;
}

//旋转编码器滤波 参数：key按键 ，lv_max滤波次数
//连续定值算法，连续lv_max次一样的值才判断按键有效
bool key_lb(uint8_t key, uint16_t lb_max)
{
  uint8_t count = 0;
  bool state; //当前值
  bool state_old = digitalRead(key);//旧值
  while (count < lb_max) //连续多少次
  {
    state = digitalRead(key);
    if (digitalRead(key) == state_old) //值一样，累积
      count++;
    else //值不一样，重新开始
    {
      state_old = state;
      count = 0;
    }
  }
  return state;
}