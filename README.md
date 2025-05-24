# micro_ros_action_example

這個範例展示如何在 ESP32-S3 R8N16 上使用 micro-ROS Action。  
適合正在開發 micro-ROS 並希望整合 ROS 2 Action 功能的使用者參考。

---

## 環境準備

1. 安裝 PlatformIO for VSCode

請先安裝 PlatformIO IDE for VSCode

可以參考這篇中文教學來快速搭建 PlatformIO 開發環境：  
https://fishros.com/d2lros2/#/humble/chapt13/get_started/3.%E6%90%AD%E5%BB%BAPlateFormIO%E5%BC%80%E5%8F%91%E7%8E%AF%E5%A2%83

---

## 專案設定與編譯

2. 將本專案複製到本地端

```
git clone https://github.com/Rem3000/micro_ros_action_example.git
```
用 VSCode 開啟下載下來的資料夾。
儲存檔案或啟用自動儲存後會自動產生 .pio 資料夾。

![alt text](image.png)

3.	修改 colcon.meta 設定

開啟 .pio/micro_ros_platformio/metas/colcon.meta，
將原本的：
```
-DRMW_UXRCE_MAX_SERVICES=1
```
修改成：
```
-DRMW_UXRCE_MAX_SERVICES=3
```
![alt text](image-1.png)

#補充：一個 micro-ROS Action 至少會使用 3 個 ROS service，
預設值太小會導致 action 執行失敗或異常。

4.	刪除快取資料夾

請務必刪除以下資料夾：
```
.pio/micro_ros_platformio/libmicror
```
否則會因快取導致後續編譯失敗。

5.	編譯介面套件

在 VSCode Terminal 中切換到 extra_packages 資料夾並執行：
```
cd extra_packages
colcon build
```

![alt text](image-2.png)

6.	使用 PlatformIO 重新編譯

回到 VSCode，按下 Ctrl + Alt + B 進行重新編譯。


7.	將自定義介面複製到 ROS 2 Workspace
```
cp -r extra_packages/bot_interfaces ~/ros2_ws/src/
cd ~/ros2_ws
colcon build
```
## 執行測試

1.	啟動 micro-ROS Agent
```
ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyACM0
```
如果不知道怎麼啟動的話可以到
https://fishros.com/d2lros2/#/humble/chapt16/17.%E6%8B%93%E5%B1%95-%E6%BA%90%E7%A0%81%E7%BC%96%E8%AF%91Agent
2.	發送 Action 測試

在另一個終端機中執行：
```
ros2 action send_goal /move_distance bot_interfaces/action/MoveDistance "{goal: 20.0}" --feedback
```
你應該會看到 ESP32 回傳的 feedback，最後回傳結果。
##注意事項
	請務必將 RMW_UXRCE_MAX_SERVICES 設為 3，否則無法成功使用 Action。
	libmicror 資料夾刪除後才會強制重建 micro-ROS 套件。
	若遇到編譯錯誤，可嘗試整個刪除 .pio 後重新來一次。

![alt text](image-4.png)
