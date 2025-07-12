// camera_module.h

#ifndef __CAMERA_MODULE_H__
#define __CAMERA_MODULE_H__

#include <rtthread.h>


#define CAM_WIDTH   240
#define CAM_HEIGHT  240

// º¯ÊýÉùÃ÷
int camera_init(void);
rt_uint8_t* camera_capture_image(void);
void camera_deinit(void);

#endif // __CAMERA_MODULE_H__

