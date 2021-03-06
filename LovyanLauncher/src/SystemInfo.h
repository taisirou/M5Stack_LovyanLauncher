#ifndef _SYSTEMINFO_H_
#define _SYSTEMINFO_H_

#include <MenuCallBack.h>
#include <M5PLUSEncoder.h>
#include <M5JoyStick.h>
#include <nvs.h>

class SystemInfo : public MenuCallBack
{
public:
  bool setup()
  {
    btnDrawer.setText(2, "Page");
    page = 0;
    drawPage(page);
    return true;
  }

  bool loop()
  {
    if (cmd == M5TreeView::eCmd::NEXT || cmd == M5TreeView::eCmd::PREV) {
      page = ++page % pageCount;
      drawPage(page);
    }
    delay(10);
    return true;
  }

private:
  int page;
  const int pageCount = 2;
  void header(const char* title) {
    int16_t y = M5.Lcd.getCursorY();
    y = y ? y + 10:0;
    for (int i = 1; i < 16; ++i) {
      M5.Lcd.drawFastHLine(0, y + i, TFT_HEIGHT, i << 1);
    }
    M5.Lcd.drawString(title, 10, y, 2);
    M5.Lcd.setCursor(0, y + 14);
  }
  void print(const char* title) {
    int16_t y = M5.Lcd.getCursorY() + 3;
    M5.Lcd.drawRightString(title, 110, y, 1);
    M5.Lcd.drawString(":", 110, y, 1);
    M5.Lcd.setCursor(120, y);
  }
  template <typename T>
  void print(const char* title, const char* data, T str) {
    print(title);
    M5.Lcd.printf(data, str);
    M5.Lcd.setCursor(0, M5.Lcd.getCursorY() + 8);
  }
  void drawPage(int page) {
    M5.Lcd.fillRect(0, 0, M5.Lcd.width(), M5.Lcd.height() - 14, 0);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(0xffff);
    M5.Lcd.setCursor(0,0);
    M5.Lcd.setTextFont(1);
    switch (page) {
    case 0:
      header("System Information");
      uint64_t chipid;
      chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
      esp_chip_info_t chip_info;
      esp_chip_info(&chip_info);
      print("ESP32 Chip ID",   "%04X",  (uint16_t)(chipid>>32));//print High 2 bytes
      print("Chip Revision",   "%d",    ESP.getChipRevision());
      print("Number of Core",  "%d",    chip_info.cores);
      print("CPU Frequency",   "%3d MHz", ESP.getCpuFreqMHz());  
      print("Flash Frequency", "%3d MHz", ESP.getFlashChipSpeed() / 1000000);
      print("Flash Chip Size", "%d Byte", ESP.getFlashChipSize());
      print("ESP-IDF version", "%s",    esp_get_idf_version());

      header("Mac Address");
      uint8_t mac[6];
      esp_base_mac_addr_get(mac);            print("Base Mac Address");printMac(mac);
      esp_efuse_mac_get_default(mac);        print("Default");printMac(mac);
      esp_read_mac(mac, ESP_MAC_WIFI_STA);   print("Wi-Fi Station");printMac(mac);
      esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);print("Wi-Fi Soft AP");printMac(mac);
      esp_read_mac(mac, ESP_MAC_BT);         print("Bluetooth");printMac(mac);
      esp_read_mac(mac, ESP_MAC_ETH);        print("Ethernet");printMac(mac);
      break;

    case 1:
      header("Memory");
      print("Total Heap Size"  , "%7d Byte", ESP.getHeapSize());
      print("Total PSRAM Size" , "%7d Byte", ESP.getPsramSize());
      print("Free Heap Size"   , "%7d Byte", esp_get_free_heap_size());
      print("Minimum Free Heap", "%7d Byte", esp_get_minimum_free_heap_size());

      header("NVS (Preferences)");
      nvs_stats_t nvs_stats;
      nvs_get_stats(NULL, &nvs_stats);
      print("Used Entries"   ,"%4d", nvs_stats.used_entries   );
      print("Free Entries"   ,"%4d", nvs_stats.free_entries   );
      print("Total Entries"  ,"%4d", nvs_stats.total_entries  );
      print("Namespace Count","%4d", nvs_stats.namespace_count);

      header("WiFi");
      print("IP Addr"); M5.Lcd.println(WiFi.localIP().toString());
      print("Subnet");  M5.Lcd.println(WiFi.subnetMask().toString());
      print("SSID");    M5.Lcd.println(WiFi.SSID());
      break;
    }
  }
  void printMac(uint8_t* mac) {
    M5.Lcd.printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }
};

#endif
