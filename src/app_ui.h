

#ifndef __APP_UI_H_

#define __APP_UI_H_

#include <zephyr/kernel.h>
#include <zephyr/smf.h>

#include "heater_controller.h"


enum event {
	EVT_NEXT,
	EVT_UP,
	EVT_DOWN,
	EVT_OK,
	EVT_BACK,
	EVT_HOME,
	EVT_ENTER_PREVIEW,
	EVT_EMPTY,
};

enum ui_state{
    UI_MAIN,//主界面
	UI_PID_TUNING,// 测试用，调节pid
    UI_PREVIEW,   // 启动时提示界面，显示vbus电压
    UI_STATE_COUNT,
};

enum pid_adj{
    ADJ_KP,
    ADJ_KI,
    ADJ_KD,
};

struct req_value{
	/** First word is reserved for use by FIFO */
	void *fifo_reserved;
	/** Request value */
	int32_t val;
};

enum draw_mode{
    NORMAL,
    FULL_SCREEN,
};



struct app {
    struct smf_ctx ctx;
    struct controller *tip_ctrl;
    enum pid_adj p_adj;
    struct port0_data_t *pd_data;
    uint32_t last_time; //记录最后刷新时间  
                         
    //刷新队列，如果发生事件会发送刷新，保证及时响应按键之类
	struct k_fifo req_fifo;
	struct req_value req;
	struct k_mutex mutex;

    int32_t test;

};

void app_init(struct app *app);

void app_event_handler(struct app *app, enum event evt);

void app_draw(struct app *app);

#endif // __APP_UI_H_
