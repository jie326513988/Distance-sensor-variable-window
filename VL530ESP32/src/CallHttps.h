//https请求
String callHttps(String url)
{
  String payload;
  //Serial.print("requesting URL: ");
  //Serial.println(url);
  //std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure(); //不检验
  HTTPClient https;
  https.setReuse(3);
  yield();
  if (https.begin(*client, url))
  {
    int httpsCode = https.GET();
    if (httpsCode > 0)  //判断有无返回值
    {
      if (httpsCode == 200 || httpsCode == 304 || httpsCode == 403 || httpsCode == 404 || httpsCode == 500) //判断请求是正确
      {
        payload = https.getString();
        //Serial.println(" ");
        //Serial.println("https.GET");
        //Serial.println(payload);
        //Serial.println(" ");
        return payload;
      }
      else
      {
        //Serial.print("请求错误："); Serial.println(httpsCode); Serial.println(" ");
        //char* httpsCode_c = new char[8];
        //itoa(httpsCode, httpsCode_c, 10); //int转char*
        payload = "{\"status_code\":\"" + String("请求错误:") + String(httpsCode) + "\"}";  //将错误值转换成json格式
        return payload;
      }
    }
    else
    {
      //Serial.println(" "); Serial.print("GET请求错误："); Serial.println(httpsCode);
      //Serial.printf("[HTTPS] GET... 失败, 错误: %s\n", https.errorToString(httpsCode).c_str());
      payload = "{\"status_code\":\"" + String(https.errorToString(httpsCode).c_str()) + "\"}";  //将错误值转换成json格式
      //Serial.println(payload);
      return payload;
    }
    https.end();
  }
  else
  {
    //Serial.printf("[HTTPS] 无法连接服务器\n");
    payload = "{\"status_code\":\"" + String("无法连接服务器") + "\"}";  //将错误值转换成json格式
    return payload;
  }
}

String callHttp(String url)
{
  String payload; //返回值
  //Serial.print("requesting URL: ");
  //Serial.println(url);
  WiFiClient client;
  HTTPClient http;
  http.setReuse(2);
  if (http.begin(client, url))
  {
    int httpCode = http.GET();
    if (httpCode > 0)  //判断有无返回值
    {
      if (httpCode == 200 || httpCode == 304 || httpCode == 403 || httpCode == 404 || httpCode == 500) //判断请求是正确
      {
        payload = http.getString();
        /*Serial.println(" ");
        Serial.println("http.GET");
        Serial.println(payload);
        Serial.println(" ");*/
        return payload;
      }
      else
      {
        payload = "{\"status_code\":\"" + String("请求错误:") + String(httpCode) + "\"}";  //将错误值转换成json格式
        return payload;
      }
    }
    else
    {
      payload = "{\"status_code\":\"" + String("httpCode错误:") + String(http.errorToString(httpCode).c_str()) + "\"}"; //将错误值转换成json格式
      return payload;
    }
    http.end();
  }
  else
  {
    Serial.printf("[HTTP] 无法连接服务器\n");
    payload = "{\"status_code\":\"" + String("无法连接服务器") + "\"}";  //将错误值转换成json格式
    return payload;
  }
}