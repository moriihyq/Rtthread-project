// camera_module.c

#include "camera_module.h"
#include "sensor.h"

// ������ hal_entry.c �ж����ȫ��ͼ�񻺳���
extern rt_uint8_t g_image_rgb565_sdram_buffer[];

// ��Ҫ���� extern sensor; sensor.h


// ��ʼ������ͷ
int camera_init(void)
{
    // ���� sensor ��ܵĳ�ʼ�������������ʼ��ȫ�ֵ� sensor ����
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

// ���պ���
rt_uint8_t* camera_capture_image(void)
{
    rt_kprintf("Capturing image...\n");

    // ֱ��ʹ��ϵͳ�ṩ��ȫ�ֱ��� sensor����ȡ���ַ
    // sensor_snapshot ��������һ�� sensor_t* ���͵�ָ��
    int error_code = sensor_snapshot(&sensor, g_image_rgb565_sdram_buffer, 0);

    if (error_code != 0)
    {
        rt_kprintf("Failed to capture image (snapshot error code: %d).\n", error_code);
        return RT_NULL;
    }

    rt_kprintf("Image captured successfully.\n");
    return g_image_rgb565_sdram_buffer;
}

// ����ʼ��/�ر�����ͷ
void camera_deinit(void)
{
    // ����Ϊ��
    rt_kprintf("Camera device deinitialized (no action needed).\n");
}
