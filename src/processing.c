// file: src/image_processing.c

#include "processing.h"
#include <string.h> // for memset

// ����һЩ�ڲ�ʹ�õĳ���
#define GRAYSCALE_THRESHOLD 80  // ��ֵ����ֵ������ʵ�ʹ�����������
#define MIN_CONTOUR_AREA 2000 // ����С������С���������ƻ����ͼ���еĴ�С����

// �ڲ�������������
static void rgb565_to_grayscale(const rt_uint8_t* src, rt_uint8_t* dst, int width, int height);
static void threshold_image(const rt_uint8_t* src, rt_uint8_t* dst, int width, int height, rt_uint8_t threshold);
static void find_connected_components(const rt_uint8_t* binary_image, int width, int height, BoundingBox* rois_out, int max_rois, int* found_count_out);


rt_err_t find_apple_regions(const rt_uint8_t* image_data, int width, int height,
                              BoundingBox* rois_out, int max_rois, int* found_count_out)
{
    // Ϊ�м䴦����̷����ڴ�
    // ע�⣺����Դ���ŵ�MCU�ϣ����Կ���ʹ�þ�̬ȫ�ֻ�����������Ƶ��������ͷ�
    rt_uint8_t* gray_buffer = (rt_uint8_t*)rt_malloc(width * height);
    rt_uint8_t* binary_buffer = (rt_uint8_t*)rt_malloc(width * height);

    if (!gray_buffer || !binary_buffer) {
        rt_kprintf("Failed to allocate memory for image processing\n");
        if(gray_buffer) rt_free(gray_buffer);
        if(binary_buffer) rt_free(binary_buffer);
        return -RT_ENOMEM;
    }

    // 1. ��RGB565ͼ��ת��Ϊ�Ҷ�ͼ
    rgb565_to_grayscale(image_data, gray_buffer, width, height);

    // 2. ���Ҷ�ͼ��ֵ��
    threshold_image(gray_buffer, binary_buffer, width, height, GRAYSCALE_THRESHOLD);

    // 3. �ڶ�ֵͼ�в�����ͨ�򣨼����ǵ�ƻ����
    find_connected_components(binary_buffer, width, height, rois_out, max_rois, found_count_out);

    // �ͷ��ڴ�
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
            // ʹ������ڲ�ֵ������򵥡���죩
            int src_x = roi->x + (int)(x * x_ratio);
            int src_y = roi->y + (int)(y * y_ratio);

            // �߽���
            if (src_x >= 0 && src_x < src_width && src_y >= 0 && src_y < src_height) {
                dst_ptr[y * dst_width + x] = src_ptr[src_y * src_width + src_x];
            }
        }
    }
    return RT_EOK;
}

// ===================================================================
// ==================== �ڲ�ʵ�ֺ��� (Internal Functions) =============
// ===================================================================

static void rgb565_to_grayscale(const rt_uint8_t* src, rt_uint8_t* dst, int width, int height) {
    const rt_uint16_t* src_ptr = (const rt_uint16_t*)src;
    for (int i = 0; i < width * height; i++) {
        rt_uint16_t pixel = src_ptr[i];
        // ��RGB565����ȡR, G, B����
        rt_uint8_t r = (pixel >> 11) & 0x1F;
        rt_uint8_t g = (pixel >> 5) & 0x3F;
        rt_uint8_t b = (pixel) & 0x1F;
        // ת����8λ
        r = (r * 255) / 31;
        g = (g * 255) / 63;
        b = (b * 255) / 31;
        // Ӧ�ûҶ�ת����ʽ (����������)
        dst[i] = (r * 30 + g * 59 + b * 11) / 100;
    }
}

static void threshold_image(const rt_uint8_t* src, rt_uint8_t* dst, int width, int height, rt_uint8_t threshold) {
    for (int i = 0; i < width * height; i++) {
        dst[i] = (src[i] > threshold) ? 255 : 0;
    }
}


// һ���򻯵���ͨ��������������ڲ�������
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
                // �ҵ���һ������������
                if (*found_count_out >= max_rois) {
                    rt_free(visited);
                    return; // �洢�ռ�����
                }

                BoundingBox current_box = {x, y, 0, 0};
                int min_x = x, max_x = x, min_y = y, max_y = y;
                long area = 0;

                // ʹ��ջ��������������� (DFS) ��������������
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

                    // �����Χ4���ھ�
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

                // �������Ƿ��㹻��
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
