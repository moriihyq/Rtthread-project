// camera_module.c

#include "camera_module.h"
#include "sensor.h"

// 引用在 hal_entry.c 中定义的全局图像缓冲区
extern rt_uint8_t g_image_rgb565_sdram_buffer[];

// 不要声明 extern sensor; sensor.h


// 初始化摄像头
int camera_init(void)
{
    // 调用 sensor 框架的初始化函数，它会初始化全局的 sensor 变量
    if (sensor_init() != RT_EOK)
    {
        rt_kprintf("Sensor init failed!\n");
        return -RT_ERROR;
    }

    sensor_reset();
    sensor_set_pixformat(PIXFORMAT_RGB565);
    sensor_set_framesize(FRAMESIZE_QVGA);

    rt_kprintf("Camera device initialized successfully.\n");
    return RT_EOK;
}

// 拍照函数
rt_uint8_t* camera_capture_image(void)
{
    rt_kprintf("Capturing image...\n");

    // 直接使用系统提供的全局变量 sensor，并取其地址
    // sensor_snapshot 函数期望一个 sensor_t* 类型的指针
    int error_code = sensor_snapshot(&sensor, g_image_rgb565_sdram_buffer, 0);

    if (error_code != 0)
    {
        rt_kprintf("Failed to capture image (snapshot error code: %d).\n", error_code);
        return RT_NULL;
    }

    rt_kprintf("Image captured successfully.\n");
    return g_image_rgb565_sdram_buffer;
}

// 反初始化/关闭摄像头
void camera_deinit(void)
{
    // 保持为空
    rt_kprintf("Camera device deinitialized (no action needed).\n");
}
