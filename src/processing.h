// file: src/image_processing.h

#ifndef __IMAGE_PROCESSING_H__
#define __IMAGE_PROCESSING_H__

#include <rtthread.h>

// ����һ���ṹ������ʾһ���������� (ROI - Region of Interest)
typedef struct {
    int x;
    int y;
    int width;
    int height;
} BoundingBox;

/**
 * @brief �ڸ�����RGB565ͼ���в���ƻ������
 *
 * @param image_data ָ��ԭʼRGB565ͼ�����ݵ�ָ��
 * @param width ͼ����
 * @param height ͼ��߶�
 * @param rois_out һ��BoundingBox���飬���ڴ洢�ҵ���ƻ������
 * @param max_rois rois_out������������
 * @param found_count_out һ��ָ�룬������ͨ��������ʵ���ҵ�����������
 * @return rt_err_t RT_EOK ��ʾ�ɹ�, ������ʾʧ��
 */
rt_err_t find_apple_regions(const rt_uint8_t* image_data, int width, int height,
                              BoundingBox* rois_out, int max_rois, int* found_count_out);

/**
 * @brief ��ԭʼͼ���вü������ų�һ��������
 *
 * @param src_image ԭʼRGB565ͼ������
 * @param src_width ԭʼͼ����
 * @param src_height ԭʼͼ��߶�
 * @param roi Ҫ�ü�������
 * @param dst_image �����Ŀ��ͼ�񻺳��� (�������Ѿ�������ڴ��)
 * @param dst_width Ŀ��ͼ���� (����ģ�������224)
 * @param dst_height Ŀ��ͼ��߶� (����ģ�������224)
 * @return rt_err_t RT_EOK ��ʾ�ɹ�
 */
rt_err_t crop_and_resize_image(const rt_uint8_t* src_image, int src_width, int src_height,
                                 const BoundingBox* roi, rt_uint8_t* dst_image,
                                 int dst_width, int dst_height);


#endif // __IMAGE_PROCESSING_H__
