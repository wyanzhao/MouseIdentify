#pragma once

#define IOCTL_INDEX             0x800
#define IOCEL_HANDLE            0x801
#define IOCTL_MOUFILTR_GET_MOUSE_ATTRIBUTES CTL_CODE( FILE_DEVICE_MOUSE,   \
                                                        IOCTL_INDEX,    \
                                                        METHOD_BUFFERED,    \
                                                        FILE_READ_DATA)

#define IOCTL_MOUFILTR_GET_EVENT_HANDLE CTL_CODE( FILE_DEVICE_MOUSE,   \
                                                        IOCEL_HANDLE,    \
                                                        METHOD_BUFFERED,    \
                                                        FILE_READ_DATA)