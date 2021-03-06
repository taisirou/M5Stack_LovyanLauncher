#ifndef _BINARYVIEWER_H_
#define _BINARYVIEWER_H_

#include <MenuCallBack.h>
#include <SD.h>
#include <esp_flash_partitions.h>

class BinaryViewer : public MenuCallBack
{
public:
  bool setup() 
  {
    for (int i = 0; i < DISPLAYLINES; ++i) {
      drawData(i, i * 16);
    }
    M5.Lcd.setCursor(224,30);
    M5.Lcd.setTextColor(0xFFFF, 0);
    M5.Lcd.printf("size:% 9d", dataSize);
    drawAddress();
    btnDrawer.setText(0, "Back/Up");
    btnDrawer.setText(2, "Down");
    return true;
  }

  bool loop()
  {
    int move = (cmd == M5TreeView::eCmd::NEXT && dataSize/16 > firstdisplayline+DISPLAYLINES ) ? 1
             : (cmd == M5TreeView::eCmd::PREV && 0 < firstdisplayline) ? -1 : 0;

    if (0 != move) {
      firstdisplayline += move;
      setupScrollArea(hHeader, hFooter);
      scroll(hHeader + (firstdisplayline % DISPLAYLINES) * 10);
      if (move == 1) {
        drawData(((firstdisplayline-1) % DISPLAYLINES), (firstdisplayline-1+ DISPLAYLINES) * 16);
      } else {
        drawData((firstdisplayline % DISPLAYLINES), firstdisplayline * 16);
      }
      drawAddress();
    } else {
      delay(10);
    }
    return true;
  }
  void close() {
    scroll(0);
  }

protected:
  int dataSize;
  const int DISPLAYLINES = 18;
  int firstdisplayline = 0;
  int hHeader = 40;
  int hFooter = 20;

  virtual int getData(int pos, uint8_t* buf) { return false; }
private:
  void setupScrollArea(uint16_t tfa, uint16_t bfa) {
    M5.Lcd.writecommand(ILI9341_VSCRDEF); // Vertical scroll definition
    M5.Lcd.writedata(tfa >> 8);           // Top Fixed Area line count
    M5.Lcd.writedata(tfa);
    M5.Lcd.writedata((TFT_WIDTH-tfa-bfa)>>8);  // Vertical Scrolling Area line count
    M5.Lcd.writedata(TFT_WIDTH-tfa-bfa);
    M5.Lcd.writedata(bfa >> 8);           // Bottom Fixed Area line count
    M5.Lcd.writedata(bfa);
  }
  void scroll(uint16_t vsp) {
    M5.Lcd.writecommand(ILI9341_VSCRSADD); // Vertical scrolling pointer
    M5.Lcd.writedata(vsp>>8);
    M5.Lcd.writedata(vsp);
  }
  bool drawAddress()
  {
    M5.Lcd.setCursor(0,30);
    M5.Lcd.setTextColor(0xFFE0, 0);
    M5.Lcd.printf("0x%08X", firstdisplayline * 16);
  }
  bool drawData(int line_y, int pos)
  {
    int y = line_y * 10 + hHeader;
    uint8_t buf[16] = {0};
    int len = getData(pos, buf);
    if (len < 16) {
      M5.Lcd.fillRect(len * 14, y, M5.Lcd.width(), 10, 0);
    }
    M5.Lcd.drawFastVLine(110, y, 10, 0x8410);
    M5.Lcd.drawFastVLine(222, y, 10, 0x8410);
    ++y;
    M5.Lcd.setTextFont(1);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(0xFFFF, 0);
    for (int x = 0; x < len; ++x) {
      M5.Lcd.setCursor(x * 14, y);
      M5.Lcd.printf("%02X", buf[x]);
      if (buf[x] > 0x20) {
        M5.Lcd.setCursor(224 + x * 6, y);
        M5.Lcd.write(buf[x]);
      } else {
        M5.Lcd.fillRect(224 + x * 6, y, 12, 8, 0);
      }
    }
    return true;
  }
};

class BinaryViewerFS : public BinaryViewer
{
  File file;
public:
  bool setup() 
  {
    MenuItemFS* mi = (MenuItemFS*)menuItem;
    if (mi->path == "") return false;
    FS& fs(mi->getFS());
    file = fs.open(mi->path);
    if (file.isDirectory()) {
      return false;
    }
    dataSize = file.size();
    M5.Lcd.setTextColor(0xFFFF);
    for (int i = 1; i < 16; ++i) {
      M5.Lcd.drawFastHLine(0, i, TFT_HEIGHT, (&fs == &SD) ? (i << 1) : (i << 6));
    }
    M5.Lcd.drawString(String((&fs == &SD) ? "SD" : "SPIFFS") + " FileBinaryViewer", 10, 0, 2);

    M5.Lcd.drawString(mi->path, 0, 20, 1);

    return BinaryViewer::setup();
  }

  void close() {
    BinaryViewer::close();
    file.close();
  }
  virtual int getData(int pos, uint8_t* buf) {
    if (!file.seek(pos)) return false;
    return file.read(buf, 16);
  }
};

class BinaryViewerFlash : public BinaryViewer
{
  esp_partition_t partition;
public:
  bool setup() 
  {
    switch (menuItem->tag) {
    case 0:
      partition.address = ESP_BOOTLOADER_OFFSET;
      partition.size = CONFIG_PARTITION_TABLE_OFFSET - ESP_BOOTLOADER_OFFSET;
      strcpy(partition.label, "2nd boot loader");
      partition.encrypted = false;
      break;
    case 1:
      partition.address = CONFIG_PARTITION_TABLE_OFFSET;
      partition.size = 4096;
      strcpy(partition.label, "partition table");
      partition.encrypted = false;
      break;

    default:
      const esp_partition_t* p
             (esp_partition_find_first( (esp_partition_type_t)(menuItem->tag >> 8)
                                      , (esp_partition_subtype_t)(menuItem->tag & 0xFF)
                                      , NULL)
             );
      if (p == NULL) return false;
      partition = *p;
    }

    dataSize = partition.size;

    M5.Lcd.setTextColor(0xFFFF);
    for (int i = 1; i < 16; ++i) {
      M5.Lcd.drawFastHLine(0, i, TFT_HEIGHT, (i << 12) | (i << 6));
    }
    M5.Lcd.drawString("FLASH BinaryViewer", 10, 0, 2);

    M5.Lcd.drawString("0x" + String(partition.address, HEX) + ":" + partition.label + (partition.encrypted ? " encrypted" : ""), 0, 20, 1);

    return BinaryViewer::setup();
  }

  virtual int getData(int pos, uint8_t* buf) {
    if (pos >= partition.size) return 0;
    if (!ESP.flashRead(partition.address + pos, (uint32_t*)buf, 16)) return 0;
    return 16;
  }
};
#endif
