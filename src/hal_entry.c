#include <rtthread.h>
#include "camera_module.h"
#include "model_inference.h"
#include "push_rod_control.h"
#include "processing.h" // 1. 包含新的头文件

#include "sensor.h"
#include "board.h"
#include "drv_lcd.h"


#ifndef CAM_WIDTH
#define CAM_WIDTH   240
#endif
#ifndef CAM_HEIGHT
#define CAM_HEIGHT  240
#endif
#ifndef MODEL_INPUT_WIDTH
#define MODEL_INPUT_WIDTH 224
#endif
#ifndef MODEL_INPUT_HEIGHT
#define MODEL_INPUT_HEIGHT 224
#endif

// 定义图像缓冲区，放在SDRAM中
rt_uint8_t g_image_rgb565_sdram_buffer[CAM_WIDTH * CAM_HEIGHT * 2] BSP_PLACE_IN_SECTION(".sdram");
// 2. 为裁剪和缩放后的图像分配一个单独的缓冲区，用于送入模型
rt_uint8_t g_model_input_buffer[MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * 2] BSP_PLACE_IN_SECTION(".sdram");

#define THREAD_PRIORITY         25
#define THREAD_STACK_SIZE       4096 // 3. 图像处理需要更多栈空间，适当增加
#define THREAD_TIMESLICE        10

static void main_controller_thread_entry(void* parameter)
{
    // Initialize all modules
    if (camera_init() != RT_EOK)
    {
        rt_kprintf("Camera initialization failed!\n");
        return;
    }
    push_rod_init();

    rt_kprintf("Main controller started.\n");

    while (1)
    {
        // 1. Capture image
        rt_uint8_t* image_data = camera_capture_image(); // 假设这个函数会把图像数据放到某个全局或静态缓冲区
        if (image_data == RT_NULL)
        {
            rt_kprintf("Failed to capture image, retrying...\n");
            rt_thread_mdelay(1000); // Wait a bit before retrying
            continue;
        }


        BoundingBox apple_rois[2]; // 我们期望找到2个苹果（真实+镜像）
        int found_count = 0;

        // 2. Find apple regions in the captured image
        find_apple_regions(image_data, CAM_WIDTH, CAM_HEIGHT, apple_rois, 2, &found_count);

        rt_bool_t is_bad_apple = RT_FALSE;

        // 3. Check if we found exactly two apples
        if (found_count == 2) {
            rt_kprintf("Found 2 apple regions. Starting inference...\n");

            // 4. Loop through each found region and run inference
            for (int i = 0; i < 2; i++) {
                // Crop and resize the region to fit the model input
                crop_and_resize_image(image_data, CAM_WIDTH, CAM_HEIGHT,
                                      &apple_rois[i], g_model_input_buffer,
                                      MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT);

                // Run model inference on the prepared image
                // 假设 run_inference 函数现在接收处理后的图像数据
                int result = run_inference(g_model_input_buffer);

                rt_kprintf("  - Region %d inference result: %s\n", i + 1, (result == 1) ? "Bad" : "Good");

                // 5. Decision Fusion: "One-vote veto"
                if (result == 1) { // Bad apple
                    is_bad_apple = RT_TRUE;
                    break; // Optimization: no need to check the other one
                }
            }
        } else {
            // 如果没找到2个，可能是苹果还没完全进入视野，按好苹果处理，直接通过
            rt_kprintf("Warning: Found %d regions, expected 2. Passing as good.\n", found_count);
        }

        // 6. Control push rod based on the final decision
        if (is_bad_apple) {
            rt_kprintf("Final Decision: Bad apple detected! Extending push rod.\n");
            push_rod_extend();
            rt_thread_mdelay(500);
            push_rod_retract();
        } else {
            rt_kprintf("Final Decision: Good apple detected. No action needed.\n");
        }
        // ======================= 核心逻辑结束 =======================


        // Delay for next detection cycle
        rt_thread_mdelay(1000); // Adjust as needed
    }
}

// ... main_controller_init 和 hal_entry 函数保持不变 ...
// ... 只需要确保 rt_thread_create 时使用的栈大小足够 (例如 4096) ...
int main_controller_init(void)
{
    rt_thread_t tid;

    tid = rt_thread_create("main_ctrl",
                            main_controller_thread_entry,
                            RT_NULL,
                            THREAD_STACK_SIZE, // 使用更新后的栈大小
                            THREAD_PRIORITY,
                            THREAD_TIMESLICE);

    // ... 后续代码不变 ...
}

// Export the initialization function to the MSH command list
MSH_CMD_EXPORT(main_controller_init, Start the main controller);

void hal_entry(void)
{
    rt_kprintf("System booting, entering hal_entry...\n");
    // 在这里调用你的初始化函数，来启动你的应用线程
    main_controller_init();
}


