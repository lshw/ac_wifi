# ac_wifi
一个基于esp8266的wifi智能插座, 10A/110-250V,  
固态继电器开关(2200w感性负载,容性负载,阻性负载),  
电压, 电流, 有功功率, 功率因数, 累计电量, web方式控制开关,  
带一个小喇叭， 可以发音3个8度,   
带一个24位真彩色led发光管, 可以设置16777216种颜色.  
可以上传数据到apache2/nginx服务器, 

设置wifi上网的方式：  
https://github.com/EspressifApp/EsptouchForAndroid/releases 下载配置工具esptool V2.0版本， 使用v1协议，   
按住智能插座的按键5秒，led开始闪烁, 进入smartconfig配网模式   
手机要连接2.4G的AP, 因为智能插座, 不支持5G  
控制开关:  
http://192.168.x.x/save.php?switch=on  
http://192.168.x.x/save.php?switch=off  
  
控制led(RRGGBB)颜色:  
http://192.168.x.x/save.php?LED=000F00  
http://192.168.x.x/save.php?LED=0F0000    
  
播放简谱，控制音量:  
http://192.168.x.x/save.php?play=abcdefg1234567ABCDEFG&vol=50  
  

