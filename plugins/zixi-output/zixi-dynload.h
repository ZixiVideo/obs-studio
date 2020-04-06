#ifndef __ZIXI_DYNLOAD_H__
#define __ZIXI_DYNLOAD_H__

#include "include/zixi_feeder_interface.h"

typedef int (*zixi_configure_logging_func)(ZIXI_LOG_LEVELS log_level, ZIXI_LOG_FUNC log_func, void *user_data);
typedef int (*zixi_open_stream_func)(zixi_stream_config parameters, encoder_control_info* enc_ctrl, void **out_stream_handle);
typedef int (*zixi_open_stream_with_rtmp_func)(zixi_stream_config parameters, encoder_control_info* enc_ctrl, zixi_rtmp_out_config* rtmp_out, void **out_stream_handle);
typedef int (*zixi_close_stream_func)(void *stream_handle);
typedef int (*zixi_set_automatic_ips_func)(void* stream_handle);
typedef int (*zixi_get_stats_func)(void *stream_handle, ZIXI_CONNECTION_STATS* conn_stats, ZIXI_NETWORK_STATS *net_stats, ZIXI_ERROR_CORRECTION_STATS *error_correction_stats);
typedef int (*zixi_version_func)(int* major, int* minor, int* minor_minor, int* build);
typedef int (*zixi_send_elementary_frame_func)(void *stream_handle, char *frame_buffer, int buffer_length, bool video, uint64_t pts, uint64_t dts);

struct ZixiFeederFunctions {
	zixi_configure_logging_func zixi_configure_logging;
	zixi_open_stream_func zixi_open_stream;
	zixi_open_stream_with_rtmp_func zixi_open_stream_with_rtmp;
	zixi_close_stream_func zixi_close_stream;
	zixi_set_automatic_ips_func zixi_set_automatic_ips;
	zixi_get_stats_func zixi_get_stats;
	zixi_version_func zixi_version;
	zixi_send_elementary_frame_func zixi_send_elementary_frame;
};

int create_zixi_feeder_functions(struct ZixiFeederFunctions * functions, void * dll);
#endif // __ZIXI_DYNLOAD_H__
