#include "application.h"
#include "button.h"
#include "codecs/box_audio_codec.h"
#include "config.h"
#include "wifi_board.h"
#include <wifi_station.h>

#include "power_save_timer.h"
#include "user_app.h"
#include <driver/i2c_master.h>
#include <esp_log.h>

#include "mcp_server.h"

#define TAG "esp-s3-PhotoPainter"

class waveshare_s3_ePaper_13_3E6 : public WifiBoard {
  private:
    i2c_master_bus_handle_t codec_i2c_bus_;
    Button                  boot_button_;
    PowerSaveTimer         *power_save_timer_;

    void InitializeCodecI2c() {
        ESP_ERROR_CHECK(i2c_master_get_bus_handle(0, &codec_i2c_bus_));
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });
    }

    void InitializeTools() {
        auto &mcp_server = McpServer::GetInstance();
        mcp_server.AddTool("self.disp.SwitchPictures", "刷新、显示、切换本地或 SD 卡中的图片，通过整数参数指定图片序号（如 “显示第 1 张图片”）,", PropertyList({Property("value", kPropertyTypeInteger, 1, sdcard_bmp_Quantity)}), [this](const PropertyList &properties) -> ReturnValue {
            if(ePaper_Refresh_Flag == 0x01) {
                return"图片正在切换中,请不要重复切换";
            } else if(ePaper_Refresh_Flag == 0x02) {
                return"正在AI生图并刷新中,请等待操作完成";
            } else {
                int value = properties["value"].value<int>();
                sdcard_doc_count = value;
                xEventGroupSetBits(epaper_groups, 0x02);        //  0000  0010
                return"切换中,刷新图片需要等待几十秒";
            }
        });

        mcp_server.AddTool("self.disp.getNumberimages", "获取 SD 卡中存储的图片文件总数，无输入参数，返回整数类型的图片数量", PropertyList(), [this](const PropertyList &) -> ReturnValue {
            xEventGroupSetBits(ai_IMG_Group, 0x02);       //Retrieve the images from the SD card
            if (xSemaphoreTake(ai_img_while_semap, pdMS_TO_TICKS(2000)) == pdTRUE) {
                return sdcard_bmp_Quantity;
            } else {
                return false;
            }
        });

        mcp_server.AddTool("self.disp.aiIMG", "这个是用户可以根据语音生成图片的(图片生成大概需要10-20s时间),比如：帮我生成一张动漫图片,直接生成就好，不要回复乱七八糟的东西", PropertyList(), [this](const PropertyList &) -> ReturnValue {
            if(ePaper_Refresh_Flag == 0x01) {
                return"图片正在切换中,请不要重复切换";
            } else if(ePaper_Refresh_Flag == 0x02) {
                return"正在AI生图并刷新中,请等待操作完成";
            } else {
                ESP_LOGI("MCP", "进入MCP aiIMG");
                xEventGroupSetBits(ai_IMG_Group, 0x01);         //Indicates that access to the 
                return"生成中，预计需要等待1~2分钟";
            }
        });

        mcp_server.AddTool("self.disp.imgloop", "进入轮询播放图片模式,循环sd卡里面的图片", PropertyList(), [this](const PropertyList &) -> ReturnValue {
            ESP_LOGI("MCP", "进入imgloop");
            ai_IMG_LoopTask_Flag = true;
            // xEventGroupSetBits(ai_IMG_LoopGroup, 0x01); 
            return true;
        });

        mcp_server.AddTool("self.disp.imgloopEit", "退出轮询播放图片模式,不在循环sd卡里面的图片", PropertyList(), [this](const PropertyList &) -> ReturnValue {
            ESP_LOGI("MCP", "进入imgloopEit");
            ai_IMG_LoopTask_Flag = false;
            // xEventGroupClearBits(ai_IMG_LoopGroup, 0x01); 
            return true;
        });

        mcp_server.AddTool("self.disp.imgsetTimerloop min", "设置轮询间隔时间,单位是分钟,如果用户说一分钟切换一张也调用这个", PropertyList({Property("timer", kPropertyTypeInteger, 1, 60)}), [this](const PropertyList &properties) -> ReturnValue {
            ESP_LOGI("MCP", "进入imgsetTimerloop");
            int value = properties["timer"].value<int>();
            ESP_LOGE("min timer", "%d", value);
            img_loopTimer = value * 60;
            ai_IMG_LoopTask_Flag = true;
            // xEventGroupSetBits(ai_IMG_LoopGroup, 0x01); 
            return true;
        });

        mcp_server.AddTool("self.disp.imgsetTimerloop h", "设置轮询间隔时间,单位是小时,如果用户说一小时切换一张也调用这个", PropertyList({Property("timer", kPropertyTypeInteger, 1, 240)}), [this](const PropertyList &properties) -> ReturnValue {
            ESP_LOGI("MCP", "进入imgsetTimerloop");
            int value = properties["timer"].value<int>();
            ESP_LOGE("h timer", "%d", value);
            img_loopTimer = value * 3600 ;
            ai_IMG_LoopTask_Flag = true;
            // xEventGroupSetBits(ai_IMG_LoopGroup, 0x01); 
            return true;
        });

        mcp_server.AddTool("self.disp.isSHTC3", "获取设备电池电量", PropertyList(), [this](const PropertyList &) -> ReturnValue {
            ESP_LOGI("MCP", "获取电池电量");
            char *str = Get_BatteryLevel();
            if(str) return str;
            else return NULL;
        });
    }

  public:
    waveshare_s3_ePaper_13_3E6() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeCodecI2c();
        User_xiaozhi_app_init();
        InitializeButtons();
        InitializeTools();
    }

    virtual AudioCodec *GetAudioCodec() override {
        static BoxAudioCodec audio_codec(
            codec_i2c_bus_,
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK,
            AUDIO_I2S_GPIO_BCLK,
            AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT,
            AUDIO_I2S_GPIO_DIN,
            AUDIO_CODEC_PA_PIN,
            AUDIO_CODEC_ES8311_ADDR,
            AUDIO_CODEC_ES7210_ADDR,
            AUDIO_INPUT_REFERENCE);
        return &audio_codec;
    }
};

DECLARE_BOARD(waveshare_s3_ePaper_13_3E6);