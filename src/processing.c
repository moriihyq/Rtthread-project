// file: src/image_processing.c

#include "processing.h"
#include <string.h> // for memset

// 定义一些内部使用的常量
#define GRAYSCALE_THRESHOLD 80  // 二值化阈值，根据实际光照条件调整
#define MIN_CONTOUR_AREA 2000 // 过滤小噪点的最小面积，根据苹果在图像中的大小调整

// 内部辅助函数声明
static void rgb565_to_grayscale(const rt_uint8_t* src, rt_uint8_t* dst, int width, int height);
static void threshold_image(const rt_uint8_t* src, rt_uint8_t* dst, int width, int height, rt_uint8_t threshold);
static void find_connected_components(const rt_uint8_t* binary_image, int width, int height, BoundingBox* rois_out, int max_rois, int* found_count_out);


rt_err_t find_apple_regions(const rt_uint8_t* image_data, int width, int height,
                              BoundingBox* rois_out, int max_rois, int* found_count_out)
{
    // 为中间处理过程分配内存
    // 注意：在资源紧张的MCU上，可以考虑使用静态全局缓冲区，避免频繁分配和释放
    rt_uint8_t* gray_buffer = (rt_uint8_t*)rt_malloc(width * height);
    rt_uint8_t* binary_buffer = (rt_uint8_t*)rt_malloc(width * height);

    if (!gray_buffer || !binary_buffer) {
        rt_kprintf("Failed to allocate memory for image processing\n");
        if(gray_buffer) rt_free(gray_buffer);
        if(binary_buffer) rt_free(binary_buffer);
        return -RT_ENOMEM;
    }

    // 1. 将RGB565图像转换为灰度图
    rgb565_to_grayscale(image_data, gray_buffer, width, height);

    // 2. 将灰度图二值化
    threshold_image(gray_buffer, binary_buffer, width, height, GRAYSCALE_THRESHOLD);

    // 3. 在二值图中查找连通域（即我们的苹果）
    find_connected_components(binary_buffer, width, height, rois_out, max_rois, found_count_out);

    // 释放内存
    rt_free(gray_buffer);
    rt_free(binary_buffer);

    return RT_EOK;
}

rt_err_t crop_and_resize_image(const rt_uint8_t* src_image, int src_width, int src_height,
                                 const BoundingBox* roi, rt_uint8_t* dst_image,
                                 int dst_width, int dst_height)
{
    const rt_uint16_t* src_ptr = (const rt_uint16_t*)src_image;
    rt_uint16_t* dst_ptr = (rt_uint16_t*)dst_image;

    float x_ratio = (float)roi->width / dst_width;
    float y_ratio = (float)roi->height / dst_height;

    for (int y = 0; y < dst_height; y++) {
        for (int x = 0; x < dst_width; x++) {
            // 使用最近邻插值法（最简单、最快）
            int src_x = roi->x + (int)(x * x_ratio);
            int src_y = roi->y + (int)(y * y_ratio);

            // 边界检查
            if (src_x >= 0 && src_x < src_width && src_y >= 0 && src_y < src_height) {
                dst_ptr[y * dst_width + x] = src_ptr[src_y * src_width + src_x];
            }
        }
    }
    return RT_EOK;
}

// ===================================================================
// ==================== 内部实现函数 (Internal Functions) =============
// ===================================================================

static void rgb565_to_grayscale(const rt_uint8_t* src, rt_uint8_t* dst, int width, int height) {
    const rt_uint16_t* src_ptr = (const rt_uint16_t*)src;
    for (int i = 0; i < width * height; i++) {
        rt_uint16_t pixel = src_ptr[i];
        // 从RGB565中提取R, G, B分量
        rt_uint8_t r = (pixel >> 11) & 0x1F;
        rt_uint8_t g = (pixel >> 5) & 0x3F;
        rt_uint8_t b = (pixel) & 0x1F;
        // 转换到8位
        r = (r * 255) / 31;
        g = (g * 255) / 63;
        b = (b * 255) / 31;
        // 应用灰度转换公式 (简化整数运算)
        dst[i] = (r * 30 + g * 59 + b * 11) / 100;
    }
}

static void threshold_image(const rt_uint8_t* src, rt_uint8_t* dst, int width, int height, rt_uint8_t threshold) {
    for (int i = 0; i < width * height; i++) {
        dst[i] = (src[i] > threshold) ? 255 : 0;
    }
}


// 一个简化的连通域分析函数，用于查找物体
static void find_connected_components(const rt_uint8_t* binary_image, int width, int height, BoundingBox* rois_out, int max_rois, int* found_count_out) {
    *found_count_out = 0;
    rt_uint8_t* visited = (rt_uint8_t*)rt_malloc(width * height);
    if (!visited) {
        rt_kprintf("Failed to allocate for visited buffer\n");
        return;
    }
    memset(visited, 0, width * height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (binary_image[y * width + x] == 255 && !visited[y * width + x]) {
                // 找到了一个新物体的起点
                if (*found_count_out >= max_rois) {
                    rt_free(visited);
                    return; // 存储空间已满
                }

                BoundingBox current_box = {x, y, 0, 0};
                int min_x = x, max_x = x, min_y = y, max_y = y;
                long area = 0;

                // 使用栈进行深度优先搜索 (DFS) 来查找整个物体
                int* stack = (int*)rt_malloc(width * height * sizeof(int));
                if (!stack) { rt_free(visited); return; }

                int stack_top = 0;
                stack[stack_top++] = y * width + x;
                visited[y * width + x] = 1;

                while (stack_top > 0) {
                    int p = stack[--stack_top];
                    int cx = p % width;
                    int cy = p / width;

                    area++;
                    if (cx < min_x) min_x = cx;
                    if (cx > max_x) max_x = cx;
                    if (cy < min_y) min_y = cy;
                    if (cy > max_y) max_y = cy;

                    // 检查周围4个邻居
                    int neighbors[] = { p - 1, p + 1, p - width, p + width };
                    for (int i = 0; i < 4; i++) {
                        int np = neighbors[i];
                        if (np >= 0 && np < width * height && binary_image[np] == 255 && !visited[np]) {
                            visited[np] = 1;
                            stack[stack_top++] = np;
                        }
                    }
                }

                rt_free(stack);

                // 检查面积是否足够大
                if (area > MIN_CONTOUR_AREA) {
                    current_box.x = min_x;
                    current_box.y = min_y;
                    current_box.width = max_x - min_x + 1;
                    current_box.height = max_y - min_y + 1;
                    rois_out[*found_count_out] = current_box;
                    (*found_count_out)++;
                }
            }
        }
    }
    rt_free(visited);
}
