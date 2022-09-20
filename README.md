# ac_wifi
一个基于esp8266的wifi智能插座, 10A/110-250V, 持续工作10A，峰值电流可以达到160A， 
另有15A/240A的型号， 2种型号， 程序是一样的。  
固态继电器开关(2.2千瓦,支持感性负载,容性负载,阻性负载),  
电压, 电流, 有功功率, 功率因数, 累计电量, web方式控制开关,  
带一个小喇叭， 可以发音3个8度,   
带一个24位真彩色led发光管, 可以设置16777216种颜色.  
可以上传数据到apache2/nginx服务器, 

设置wifi上网的方式：  
https://github.com/EspressifApp/EsptouchForAndroid/releases 下载配置工具EspTouch， 使用v1协议，   
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
 
实时数据api: 
http://192.168.x.x/api.php  
返回json数据: 
{"KWH":190.35963929,"V":240.03,"I":0.98,"W":114.35,"PF":0.48} 

web界面: 
![image](https://github.com/lshw/ac_wifi/blob/master/doc/web.jpg)  

有很多大厂在做这个产品，为啥还要再做一个呢？ 
主要是大厂的产品，都走自己的云端， 不方便把数据用到自己的程序和项目去。  
 
刘世伟的地摊: https://shop316166779.taobao.com  
智能插座: https://item.taobao.com/item.htm?&id=669106171369  
欢迎收藏
