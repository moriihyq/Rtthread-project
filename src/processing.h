// file: src/image_processing.h

#ifndef __IMAGE_PROCESSING_H__
#define __IMAGE_PROCESSING_H__

#include <rtthread.h>

// 定义一个结构体来表示一个矩形区域 (ROI - Region of Interest)
typedef struct {
    int x;
    int y;
    int width;
    int height;
} BoundingBox;

/**
 * @brief 在给定的RGB565图像中查找苹果区域
 *
 * @param image_data 指向原始RGB565图像数据的指针
 * @param width 图像宽度
 * @param height 图像高度
 * @param rois_out 一个BoundingBox数组，用于存储找到的苹果区域
 * @param max_rois rois_out数组的最大容量
 * @param found_count_out 一个指针，函数将通过它返回实际找到的区域数量
 * @return rt_err_t RT_EOK 表示成功, 其他表示失败
 */
rt_err_t find_apple_regions(const rt_uint8_t* image_data, int width, int height,
                              BoundingBox* rois_out, int max_rois, int* found_count_out);

/**
 * @brief 从原始图像中裁剪并缩放出一个子区域
 *
 * @param src_image 原始RGB565图像数据
 * @param src_width 原始图像宽度
 * @param src_height 原始图像高度
 * @param roi 要裁剪的区域
 * @param dst_image 输出的目标图像缓冲区 (必须是已经分配好内存的)
 * @param dst_width 目标图像宽度 (例如模型输入的224)
 * @param dst_height 目标图像高度 (例如模型输入的224)
 * @return rt_err_t RT_EOK 表示成功
 */
rt_err_t crop_and_resize_image(const rt_uint8_t* src_image, int src_width, int src_height,
                                 const BoundingBox* roi, rt_uint8_t* dst_image,
                                 int dst_width, int dst_height);


#endif // __IMAGE_PROCESSING_H__
