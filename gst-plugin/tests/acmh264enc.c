/* GStreamer
 *
 * unit test for acmh264enc
 *
 * Copyright (C) 2013 Atmark Techno, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <gst/check/gstcheck.h>

#include "utest_util.h"

#define USE_STRIDE_PROP					0


/* For ease of programming we use globals to keep refs for our floating
 * src and sink pads we create; otherwise we always have to do get_pad,
 * get_peer, and then remove references in every test function */
static GstPad *mysrcpad, *mysinkpad;

#define VIDEO_CAPS_STRING "video/x-raw, " \
	"format = (string) NV12, " \
	"width = (int) 320, " \
	"height = (int) 240, " \
	"framerate = (fraction) 30/1"

#define MP4_CAPS_STRING "video/x-h264, " \
	"width = (int) 320, " \
	"height = (int) 240, " \
	"framerate = (fraction) 30/1, " \
	"stream-format = (string) avc, " \
	"alignment = (string) au"

/* output */
static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
	GST_PAD_SINK,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS (MP4_CAPS_STRING));

/* input */
static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
	GST_PAD_SRC,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS (VIDEO_CAPS_STRING));

/* number of the buffers which push */
#define PUSH_BUFFERS	100

/* chain function of sink pad */
static GstPadChainFunction g_sink_base_chain = NULL;

/* output data file path */
static char g_output_data_file_path[PATH_MAX];


/* setup */
static GstElement *
setup_acmh264enc ()
{
	GstElement *acmh264enc;
	
	g_print ("setup_acmh264enc\n");
	acmh264enc = gst_check_setup_element ("acmh264enc");
	g_print ("pass : gst_check_setup_element()\n");
	mysrcpad = gst_check_setup_src_pad (acmh264enc, &srctemplate);
	g_print ("pass : gst_check_setup_src_pad()\n");
	mysinkpad = gst_check_setup_sink_pad (acmh264enc, &sinktemplate);
	g_print ("pass : gst_check_setup_sink_pad()\n");

	gst_pad_set_active (mysrcpad, TRUE);
	gst_pad_set_active (mysinkpad, TRUE);
	
	return acmh264enc;
}

/* cleanup */
static void
cleanup_acmh264enc (GstElement * acmh264enc)
{
	g_print ("cleanup_acmh264enc\n");
	gst_element_set_state (acmh264enc, GST_STATE_NULL);
	g_print ("pass : gst_element_set_state()\n");

	gst_pad_set_active (mysrcpad, FALSE);
	g_print ("pass : gst_pad_set_active(mysrcpad)\n");
	gst_pad_set_active (mysinkpad, FALSE);
	g_print ("pass : gst_pad_set_active(mysinkpad)\n");
	gst_check_teardown_src_pad (acmh264enc);
	g_print ("pass : gst_check_teardown_src_pad()\n");
	gst_check_teardown_sink_pad (acmh264enc);
	g_print ("pass : gst_check_teardown_sink_pad()\n");
	gst_check_teardown_element (acmh264enc);
	g_print ("pass : gst_check_teardown_element()\n");
}

/* property set / get */
GST_START_TEST (test_properties)
{
	GstElement *acmh264enc;
	gchar *device;
#if USE_STRIDE_PROP
	gint stride;
#endif
	gint32 bit_rate;
	gint32 max_frame_size;
	gint rate_control_mode;
	gint max_GOP_length;
	gint B_pic_mode;
	gint x_offfset;
	gint y_offset;

	/* setup */
	acmh264enc = setup_acmh264enc ();
	
	/* set and check properties */
	g_object_set (acmh264enc,
				  "device", 				"/dev/video1",
#if USE_STRIDE_PROP
				  "stride", 				1024,
#endif
				  "bitrate", 				16000,
				  "max-frame-size",			2,
				  "rate-control-mode", 		0,
				  "max-gop-length",			0,
				  "b-pic-mode",				0,
				  "x-offset",				160,
				  "y-offset",				160,
				  NULL);
	g_object_get (acmh264enc,
				  "device", 				&device,
#if USE_STRIDE_PROP
				  "stride", 				&stride,
#endif
				  "bitrate", 				&bit_rate,
				  "max-frame-size", 		&max_frame_size,
				  "rate-control-mode", 		&rate_control_mode,
				  "max-gop-length", 		&max_GOP_length,
				  "b-pic-mode", 			&B_pic_mode,
				  "x-offset", 				&x_offfset,
				  "y-offset", 				&y_offset,
				  NULL);
	fail_unless (g_str_equal (device, "/dev/video1"));
#if USE_STRIDE_PROP
	fail_unless_equals_int (stride, 1024);
#endif
	fail_unless_equals_int (bit_rate, 16000);
	fail_unless_equals_int (max_frame_size, 2);
	fail_unless_equals_int (rate_control_mode, 0);
	fail_unless_equals_int (max_GOP_length, 0);
	fail_unless_equals_int (B_pic_mode, 0);
	fail_unless_equals_int (x_offfset, 160);
	fail_unless_equals_int (y_offset, 160);
	g_free (device);
	device = NULL;

	/* new properties */
	g_object_set (acmh264enc,
				  "device", 				"/dev/video2",
#if USE_STRIDE_PROP
				  "stride", 				1920,
#endif
				  "bitrate", 				40000000,
				  "max-frame-size",			5,
				  "rate-control-mode", 		2,
				  "max-gop-length",			120,
				  "b-pic-mode",				3,
				  "x-offset",				0,
				  "y-offset",				0,
				  NULL);
	g_object_get (acmh264enc,
				  "device", 				&device,
#if USE_STRIDE_PROP
				  "stride", 				&stride,
#endif
				  "bitrate", 				&bit_rate,
				  "max-frame-size", 		&max_frame_size,
				  "rate-control-mode", 		&rate_control_mode,
				  "max-gop-length", 		&max_GOP_length,
				  "b-pic-mode", 			&B_pic_mode,
				  "x-offset", 				&x_offfset,
				  "y-offset", 				&y_offset,
				  NULL);
	fail_unless (g_str_equal (device, "/dev/video2"));
#if USE_STRIDE_PROP
	fail_unless_equals_int (stride, 1920);
#endif
	fail_unless_equals_int (bit_rate, 40000000);
	fail_unless_equals_int (max_frame_size, 5);
	fail_unless_equals_int (rate_control_mode, 2);
	fail_unless_equals_int (max_GOP_length, 120);
	fail_unless_equals_int (B_pic_mode, 3);
	fail_unless_equals_int (x_offfset, 0);
	fail_unless_equals_int (y_offset, 0);
	g_free (device);
	device = NULL;

	/* cleanup	*/
	cleanup_acmh264enc (acmh264enc);
}
GST_END_TEST;

/* Combination of a property */
static void
check_property_combination(gint max_GOP_length, gint B_pic_mode, GstFlowReturn result)
{
	GstElement *acmh264enc;
	GstBuffer *buffer;
	GstCaps *caps;
	GstBuffer *outbuffer;
	gint i;
	
	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			max_GOP_length,
				  "b-pic-mode",				B_pic_mode,
				  NULL);
	gst_element_set_state (acmh264enc, GST_STATE_PLAYING);
	
	/* make caps */
	caps = gst_caps_from_string (VIDEO_CAPS_STRING);	
	fail_unless (gst_pad_set_caps (mysrcpad, caps));

	/* pre encode max is 3 */
	for (i = 0; i < 4; i++) {
		/* create buffer */
		fail_unless ((buffer = create_video_buffer (caps)) != NULL);
		
		/* push buffer */
		fail_unless (gst_pad_push (mysrcpad, buffer) == result);
	}
	
	/* release encoded data */
	if (g_list_length (buffers) > 0) {
		g_print ("num_buffers : %d\n", g_list_length (buffers));
		outbuffer = GST_BUFFER (buffers->data);
		buffers = g_list_remove (buffers, outbuffer);
		
		ASSERT_BUFFER_REFCOUNT (outbuffer, "outbuffer", 1);
		gst_buffer_unref (outbuffer);
		outbuffer = NULL;
	}
	
	/* cleanup */
	gst_caps_unref (caps);
	cleanup_acmh264enc (acmh264enc);
}

GST_START_TEST (test_property_combination)
{
	/* max_GOP_length:0, B_pic_mode:0 の場合を除いて、
	 * max_GOP_length は、B_pic_mode より大きくなくてはならない
	 */
	check_property_combination(0, 0, GST_FLOW_OK);
	check_property_combination(0, 1, GST_FLOW_NOT_NEGOTIATED);
	check_property_combination(0, 2, GST_FLOW_NOT_NEGOTIATED);
	check_property_combination(0, 3, GST_FLOW_NOT_NEGOTIATED);

	check_property_combination(1, 0, GST_FLOW_OK);
	check_property_combination(1, 1, GST_FLOW_NOT_NEGOTIATED);
	check_property_combination(1, 2, GST_FLOW_NOT_NEGOTIATED);
	check_property_combination(1, 3, GST_FLOW_NOT_NEGOTIATED);

	check_property_combination(2, 0, GST_FLOW_OK);
	check_property_combination(2, 1, GST_FLOW_OK);
	check_property_combination(2, 2, GST_FLOW_NOT_NEGOTIATED);
	check_property_combination(2, 3, GST_FLOW_NOT_NEGOTIATED);

	check_property_combination(3, 0, GST_FLOW_OK);
	check_property_combination(3, 1, GST_FLOW_OK);
	check_property_combination(3, 2, GST_FLOW_OK);
	check_property_combination(3, 3, GST_FLOW_NOT_NEGOTIATED);

	check_property_combination(15, 0, GST_FLOW_OK);
	check_property_combination(15, 1, GST_FLOW_OK);
	check_property_combination(15, 2, GST_FLOW_OK);
	check_property_combination(15, 3, GST_FLOW_OK);
}
GST_END_TEST;

/* check caps	*/
GST_START_TEST (test_check_caps)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;
    GstCaps *outcaps;
	GstBuffer *inbuffer, *outbuffer;
	int i, num_buffers;

	/* setup */
	acmh264enc = setup_acmh264enc ();
	/* all I picture */
	g_object_set (acmh264enc,
				  "max-gop-length",			1,
				  "b-pic-mode",				0,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");

	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);

	/* corresponds to I420 buffer for the size mentioned in the caps */
	/* YUV : 320 width x 240 height */
	inbuffer = gst_buffer_new_and_alloc (320 * 240 * 3 / 2);
	/* makes valgrind's memcheck happier */
	gst_buffer_memset (inbuffer, 0, 0, -1);
	GST_BUFFER_TIMESTAMP (inbuffer) = 0;
	ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);
	fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
	
	/* send eos to have all flushed if needed */
	fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()) == TRUE);

	num_buffers = g_list_length (buffers);
	g_print ("num_buffers : %d\n", num_buffers);
	fail_unless (num_buffers == 1);

	/* check the out caps */
	outcaps = gst_pad_get_current_caps (mysinkpad);
	g_print ("outcaps : %p\n", outcaps);
	{
		GstStructure *s;
		const GValue *sf, *cd;
		const gchar *stream_format;
		
		fail_unless (outcaps != NULL);
		
		GST_INFO ("outcaps %" GST_PTR_FORMAT "\n", outcaps);
		s = gst_caps_get_structure (outcaps, 0);
		fail_unless (s != NULL);
		fail_if (!gst_structure_has_name (s, "video/x-h264"));
		sf = gst_structure_get_value (s, "stream-format");
		fail_unless (sf != NULL);
		fail_unless (G_VALUE_HOLDS_STRING (sf));
		stream_format = g_value_get_string (sf);
		fail_unless (stream_format != NULL);
		if (strcmp (stream_format, "avc") == 0) {
			GstMapInfo map;
			GstBuffer *buf;
			
			cd = gst_structure_get_value (s, "codec_data");
			fail_unless (cd != NULL);
			fail_unless (GST_VALUE_HOLDS_BUFFER (cd));
			buf = gst_value_get_buffer (cd);
			fail_unless (buf != NULL);
			gst_buffer_map (buf, &map, GST_MAP_READ);
			fail_unless_equals_int (map.data[0], 1);
			/* RMA001_マルチメディアミドル_機能仕様書 : 映像エンコード仕様
			 * 画像サイズや、Bピクチャの有無によってプロファイルが自動的に変わる
			 */
			fail_unless (map.data[1] == 0x42);

			gst_buffer_unmap (buf, &map);
		}
		else {
			fail_if (TRUE, "unexpected stream-format in caps: %s", stream_format);
		}
	}

	/* clean up buffers */
	for (i = 0; i < num_buffers; ++i) {
		outbuffer = GST_BUFFER (buffers->data);
		fail_if (outbuffer == NULL);

		buffers = g_list_remove (buffers, outbuffer);

		ASSERT_BUFFER_REFCOUNT (outbuffer, "outbuffer", 1);
		gst_buffer_unref (outbuffer);
		outbuffer = NULL;
	}

	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	gst_caps_unref (srccaps);
// don't need    gst_caps_unref (outcaps);
	if (buffers) {
		g_list_free (buffers);
		buffers = NULL;
	}
}
GST_END_TEST;

// TODO : offset property , 画像サイズチェック

/* H264 encode	*/
static GstFlowReturn
test_encode_sink_chain(GstPad * pad, GstObject * parent, GstBuffer * buf)
{
	size_t size;
	void *p;
	char file[PATH_MAX];
	static gint nOutputBuffers = 0;
	GstBuffer *outbuffer;
	GstMapInfo map;
	GstFlowReturn ret;

	ret = g_sink_base_chain(pad, parent, buf);
	
	/* check outputed buffer */
	if (g_list_length (buffers) > 0) {
		++nOutputBuffers;
		
		outbuffer = GST_BUFFER (buffers->data);
		fail_if (outbuffer == NULL);
		fail_unless (GST_IS_BUFFER (outbuffer));

		sprintf(file, g_output_data_file_path, nOutputBuffers);
		g_print("%s\n", file);

		get_data(file, &size, &p);

		fail_unless (gst_buffer_get_size (outbuffer) == size);
		gst_buffer_map (outbuffer, &map, GST_MAP_READ);
		fail_unless (0 == memcmp(p, map.data, size));
		gst_buffer_unmap (outbuffer, &map);

		fail_unless (0 == munmap(p, size));

		buffers = g_list_remove (buffers, outbuffer);
		ASSERT_BUFFER_REFCOUNT (outbuffer, "outbuffer", 1);

		gst_buffer_unref (outbuffer);
		outbuffer = NULL;
	}
	
	return ret;
}

static void
input_buffers(int num_bufs, char* data_path)
{
	size_t size;
	void *p;
	char file[PATH_MAX];
	GstBuffer *inbuffer;
	gint nInputBuffers = 0;
	
	while (TRUE) {
		if (++nInputBuffers < num_bufs) {
			sprintf(file, data_path, nInputBuffers);
			g_print("%s\n", file);

			get_data(file, &size, &p);

			inbuffer = gst_buffer_new_and_alloc (size);
			gst_buffer_fill (inbuffer, 0, p, size);

			fail_unless (0 == munmap(p, size));

			GST_BUFFER_TIMESTAMP (inbuffer) = 0;
			ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

			fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);

//			gst_buffer_unref (inbuffer);
		}

		if (nInputBuffers == num_bufs) {
			/* push EOS event */
			fail_unless (gst_pad_push_event (mysrcpad,
				gst_event_new_eos ()) == TRUE);

			break;
		}
	}
}

GST_START_TEST (test_encode_prop01)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;

	/* encode test 1
	 * bitrate = 
	 * max-frame-size =
	 * rate-control-mode = 
	 ** max-gop-length = 0
	 ** b-pic-mode = 0
	 */

	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			0,
				  "b-pic-mode",				0,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");

	g_sink_base_chain = GST_PAD_CHAINFUNC (mysinkpad);
	gst_pad_set_chain_function (mysinkpad,
		GST_DEBUG_FUNCPTR (test_encode_sink_chain));

	strcpy(g_output_data_file_path, "data/h264_enc/propset01/h264_%03d.data");

	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);

	/* input buffers */
	input_buffers(PUSH_BUFFERS, "data/h264_enc/input01/rgb_%03d.data");

	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	g_list_free (buffers);
	buffers = NULL;
	gst_caps_unref (srccaps);
}
GST_END_TEST;

#if 0	/* max-gop-length must be greater than b-pic-mode */
GST_START_TEST (test_encode_prop02)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;
	
	/* encode test 2
	 * bitrate =
	 * max-frame-size =
	 * rate-control-mode =
	 ** max-gop-length = 0
	 ** b-pic-mode = 1
	 */
	
	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			0,
				  "b-pic-mode",				1,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");
	
	g_sink_base_chain = GST_PAD_CHAINFUNC (mysinkpad);
	gst_pad_set_chain_function (mysinkpad,
								GST_DEBUG_FUNCPTR (test_encode_sink_chain));
	
	strcpy(g_output_data_file_path, "data/h264_enc/propset02/h264_%03d.data");
	
	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);
	
	/* input buffers */
	input_buffers(PUSH_BUFFERS, "data/h264_enc/input01/rgb_%03d.data");
	
	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	g_list_free (buffers);
	buffers = NULL;
	gst_caps_unref (srccaps);
}
GST_END_TEST;
#endif

#if 0	/* max-gop-length must be greater than b-pic-mode */
GST_START_TEST (test_encode_prop03)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;
	
	/* encode test 3
	 * bitrate =
	 * max-frame-size =
	 * rate-control-mode =
	 ** max-gop-length = 0
	 ** b-pic-mode = 2
	 */
	
	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			0,
				  "b-pic-mode",				2,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");
	
	g_sink_base_chain = GST_PAD_CHAINFUNC (mysinkpad);
	gst_pad_set_chain_function (mysinkpad,
								GST_DEBUG_FUNCPTR (test_encode_sink_chain));
	
	strcpy(g_output_data_file_path, "data/h264_enc/propset03/h264_%03d.data");
	
	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);
	
	/* input buffers */
	input_buffers(PUSH_BUFFERS, "data/h264_enc/input01/rgb_%03d.data");
	
	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	g_list_free (buffers);
	buffers = NULL;
	gst_caps_unref (srccaps);
}
GST_END_TEST;
#endif

#if 0	/* max-gop-length must be greater than b-pic-mode */
GST_START_TEST (test_encode_prop04)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;
	
	/* encode test 4
	 * bitrate =
	 * max-frame-size =
	 * rate-control-mode =
	 ** max-gop-length = 0
	 ** b-pic-mode = 3
	 */
	
	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			0,
				  "b-pic-mode",				3,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");
	
	g_sink_base_chain = GST_PAD_CHAINFUNC (mysinkpad);
	gst_pad_set_chain_function (mysinkpad,
								GST_DEBUG_FUNCPTR (test_encode_sink_chain));
	
	strcpy(g_output_data_file_path, "data/h264_enc/propset04/h264_%03d.data");
	
	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);
	
	/* input buffers */
	input_buffers(PUSH_BUFFERS, "data/h264_enc/input01/rgb_%03d.data");
	
	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	g_list_free (buffers);
	buffers = NULL;
	gst_caps_unref (srccaps);
}
GST_END_TEST;
#endif

GST_START_TEST (test_encode_prop11)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;
	
	/* encode test 11
	 * bitrate =
	 * max-frame-size =
	 * rate-control-mode =
	 ** max-gop-length = 1
	 ** b-pic-mode = 0
	 */
	
	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			1,
				  "b-pic-mode",				0,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");
	
	g_sink_base_chain = GST_PAD_CHAINFUNC (mysinkpad);
	gst_pad_set_chain_function (mysinkpad,
								GST_DEBUG_FUNCPTR (test_encode_sink_chain));
	
	strcpy(g_output_data_file_path, "data/h264_enc/propset11/h264_%03d.data");
	
	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);
	
	/* input buffers */
	input_buffers(PUSH_BUFFERS, "data/h264_enc/input01/rgb_%03d.data");
	
	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	g_list_free (buffers);
	buffers = NULL;
	gst_caps_unref (srccaps);
}
GST_END_TEST;

#if 0	/* max-gop-length must be greater than b-pic-mode */
GST_START_TEST (test_encode_prop12)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;
	
	/* encode test 12
	 * bitrate =
	 * max-frame-size =
	 * rate-control-mode =
	 ** max-gop-length = 1
	 ** b-pic-mode = 1
	 */
	
	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			1,
				  "b-pic-mode",				1,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");
	
	g_sink_base_chain = GST_PAD_CHAINFUNC (mysinkpad);
	gst_pad_set_chain_function (mysinkpad,
								GST_DEBUG_FUNCPTR (test_encode_sink_chain));
	
	strcpy(g_output_data_file_path, "data/h264_enc/propset12/h264_%03d.data");
	
	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);
	
	/* input buffers */
	input_buffers(PUSH_BUFFERS, "data/h264_enc/input01/rgb_%03d.data");
	
	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	g_list_free (buffers);
	buffers = NULL;
	gst_caps_unref (srccaps);
}
GST_END_TEST;
#endif

#if 0	/* max-gop-length must be greater than b-pic-mode */
GST_START_TEST (test_encode_prop13)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;
	
	/* encode test 13
	 * bitrate =
	 * max-frame-size =
	 * rate-control-mode =
	 ** max-gop-length = 1
	 ** b-pic-mode = 2
	 */
	
	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			1,
				  "b-pic-mode",				2,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");
	
	g_sink_base_chain = GST_PAD_CHAINFUNC (mysinkpad);
	gst_pad_set_chain_function (mysinkpad,
								GST_DEBUG_FUNCPTR (test_encode_sink_chain));
	
	strcpy(g_output_data_file_path, "data/h264_enc/propset13/h264_%03d.data");
	
	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);
	
	/* input buffers */
	input_buffers(PUSH_BUFFERS, "data/h264_enc/input01/rgb_%03d.data");
	
	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	g_list_free (buffers);
	buffers = NULL;
	gst_caps_unref (srccaps);
}
GST_END_TEST;
#endif

#if 0	/* max-gop-length must be greater than b-pic-mode */
GST_START_TEST (test_encode_prop14)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;
	
	/* encode test 14
	 * bitrate =
	 * max-frame-size =
	 * rate-control-mode =
	 ** max-gop-length = 1
	 ** b-pic-mode = 3
	 */
	
	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			1,
				  "b-pic-mode",				3,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");
	
	g_sink_base_chain = GST_PAD_CHAINFUNC (mysinkpad);
	gst_pad_set_chain_function (mysinkpad,
								GST_DEBUG_FUNCPTR (test_encode_sink_chain));
	
	strcpy(g_output_data_file_path, "data/h264_enc/propset14/h264_%03d.data");
	
	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);
	
	/* input buffers */
	input_buffers(PUSH_BUFFERS, "data/h264_enc/input01/rgb_%03d.data");
	
	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	g_list_free (buffers);
	buffers = NULL;
	gst_caps_unref (srccaps);
}
GST_END_TEST;
#endif

GST_START_TEST (test_encode_prop21)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;
	
	/* encode test 21
	 * bitrate =
	 * max-frame-size =
	 * rate-control-mode =
	 ** max-gop-length = 2
	 ** b-pic-mode = 0
	 */
	
	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			2,
				  "b-pic-mode",				0,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");
	
	g_sink_base_chain = GST_PAD_CHAINFUNC (mysinkpad);
	gst_pad_set_chain_function (mysinkpad,
								GST_DEBUG_FUNCPTR (test_encode_sink_chain));
	
	strcpy(g_output_data_file_path, "data/h264_enc/propset21/h264_%03d.data");
	
	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);
	
	/* input buffers */
	input_buffers(PUSH_BUFFERS, "data/h264_enc/input01/rgb_%03d.data");
	
	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	g_list_free (buffers);
	buffers = NULL;
	gst_caps_unref (srccaps);
}
GST_END_TEST;

GST_START_TEST (test_encode_prop22)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;
	
	/* encode test 22
	 * bitrate =
	 * max-frame-size =
	 * rate-control-mode =
	 ** max-gop-length = 2
	 ** b-pic-mode = 1
	 */
	
	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			2,
				  "b-pic-mode",				1,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");
	
	g_sink_base_chain = GST_PAD_CHAINFUNC (mysinkpad);
	gst_pad_set_chain_function (mysinkpad,
								GST_DEBUG_FUNCPTR (test_encode_sink_chain));
	
	strcpy(g_output_data_file_path, "data/h264_enc/propset22/h264_%03d.data");
	
	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);
	
	/* input buffers */
	input_buffers(PUSH_BUFFERS, "data/h264_enc/input01/rgb_%03d.data");
	
	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	g_list_free (buffers);
	buffers = NULL;
	gst_caps_unref (srccaps);
}
GST_END_TEST;

#if 0	/* max-gop-length must be greater than b-pic-mode */
GST_START_TEST (test_encode_prop23)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;
	
	/* encode test 23
	 * bitrate =
	 * max-frame-size =
	 * rate-control-mode =
	 ** max-gop-length = 2
	 ** b-pic-mode = 2
	 */
	
	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			2,
				  "b-pic-mode",				2,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");
	
	g_sink_base_chain = GST_PAD_CHAINFUNC (mysinkpad);
	gst_pad_set_chain_function (mysinkpad,
								GST_DEBUG_FUNCPTR (test_encode_sink_chain));
	
	strcpy(g_output_data_file_path, "data/h264_enc/propset23/h264_%03d.data");
	
	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);
	
	/* input buffers */
	input_buffers(PUSH_BUFFERS, "data/h264_enc/input01/rgb_%03d.data");
	
	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	g_list_free (buffers);
	buffers = NULL;
	gst_caps_unref (srccaps);
}
GST_END_TEST;
#endif

#if 0	/* max-gop-length must be greater than b-pic-mode */
GST_START_TEST (test_encode_prop24)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;
	
	/* encode test 24
	 * bitrate =
	 * max-frame-size =
	 * rate-control-mode =
	 ** max-gop-length = 2
	 ** b-pic-mode = 3
	 */
	
	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			2,
				  "b-pic-mode",				3,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");
	
	g_sink_base_chain = GST_PAD_CHAINFUNC (mysinkpad);
	gst_pad_set_chain_function (mysinkpad,
								GST_DEBUG_FUNCPTR (test_encode_sink_chain));
	
	strcpy(g_output_data_file_path, "data/h264_enc/propset24/h264_%03d.data");
	
	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);
	
	/* input buffers */
	input_buffers(PUSH_BUFFERS, "data/h264_enc/input01/rgb_%03d.data");
	
	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	g_list_free (buffers);
	buffers = NULL;
	gst_caps_unref (srccaps);
}
GST_END_TEST;
#endif

GST_START_TEST (test_encode_prop31)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;
	
	/* encode test 31
	 * bitrate =
	 * max-frame-size =
	 * rate-control-mode =
	 ** max-gop-length = 30
	 ** b-pic-mode = 0
	 */
	
	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			30,
				  "b-pic-mode",				0,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");
	
	g_sink_base_chain = GST_PAD_CHAINFUNC (mysinkpad);
	gst_pad_set_chain_function (mysinkpad,
								GST_DEBUG_FUNCPTR (test_encode_sink_chain));
	
	strcpy(g_output_data_file_path, "data/h264_enc/propset31/h264_%03d.data");
	
	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);
	
	/* input buffers */
	input_buffers(PUSH_BUFFERS, "data/h264_enc/input01/rgb_%03d.data");
	
	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	g_list_free (buffers);
	buffers = NULL;
	gst_caps_unref (srccaps);
}
GST_END_TEST;

GST_START_TEST (test_encode_prop32)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;
	
	/* encode test 32
	 * bitrate =
	 * max-frame-size =
	 * rate-control-mode =
	 ** max-gop-length = 30
	 ** b-pic-mode = 1
	 */
	
	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			30,
				  "b-pic-mode",				1,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");
	
	g_sink_base_chain = GST_PAD_CHAINFUNC (mysinkpad);
	gst_pad_set_chain_function (mysinkpad,
								GST_DEBUG_FUNCPTR (test_encode_sink_chain));
	
	strcpy(g_output_data_file_path, "data/h264_enc/propset32/h264_%03d.data");
	
	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);
	
	/* input buffers */
	input_buffers(PUSH_BUFFERS, "data/h264_enc/input01/rgb_%03d.data");
	
	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	g_list_free (buffers);
	buffers = NULL;
	gst_caps_unref (srccaps);
}
GST_END_TEST;

GST_START_TEST (test_encode_prop33)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;
	
	/* encode test 33
	 * bitrate =
	 * max-frame-size =
	 * rate-control-mode =
	 ** max-gop-length = 30
	 ** b-pic-mode = 2
	 */
	
	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			30,
				  "b-pic-mode",				2,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");
	
	g_sink_base_chain = GST_PAD_CHAINFUNC (mysinkpad);
	gst_pad_set_chain_function (mysinkpad,
								GST_DEBUG_FUNCPTR (test_encode_sink_chain));
	
	strcpy(g_output_data_file_path, "data/h264_enc/propset33/h264_%03d.data");
	
	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);
	
	/* input buffers */
	input_buffers(PUSH_BUFFERS, "data/h264_enc/input01/rgb_%03d.data");
	
	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	g_list_free (buffers);
	buffers = NULL;
	gst_caps_unref (srccaps);
}
GST_END_TEST;

GST_START_TEST (test_encode_prop34)
{
	GstElement *acmh264enc;
	GstCaps *srccaps;
	
	/* encode test 34
	 * bitrate =
	 * max-frame-size =
	 * rate-control-mode =
	 ** max-gop-length = 30
	 ** b-pic-mode = 3
	 */
	
	/* setup */
	acmh264enc = setup_acmh264enc ();
	g_object_set (acmh264enc,
				  "max-gop-length",			30,
				  "b-pic-mode",				3,
				  NULL);
	fail_unless (gst_element_set_state (acmh264enc, GST_STATE_PLAYING)
				 == GST_STATE_CHANGE_SUCCESS, "could not set to playing");
	
	g_sink_base_chain = GST_PAD_CHAINFUNC (mysinkpad);
	gst_pad_set_chain_function (mysinkpad,
								GST_DEBUG_FUNCPTR (test_encode_sink_chain));
	
	strcpy(g_output_data_file_path, "data/h264_enc/propset34/h264_%03d.data");
	
	/* set src caps */
	srccaps = gst_caps_from_string (VIDEO_CAPS_STRING);
	gst_pad_set_caps (mysrcpad, srccaps);
	
	/* input buffers */
	input_buffers(PUSH_BUFFERS, "data/h264_enc/input01/rgb_%03d.data");
	
	/* cleanup */
	cleanup_acmh264enc (acmh264enc);
	g_list_free (buffers);
	buffers = NULL;
	gst_caps_unref (srccaps);
}
GST_END_TEST;

static Suite *
acmh264enc_suite (void)
{
	Suite *s = suite_create ("acmh264enc");
	TCase *tc_chain = tcase_create ("general");

	tcase_set_timeout (tc_chain, 0);

	suite_add_tcase (s, tc_chain);

	tcase_add_test (tc_chain, test_properties);
#if 0	// TODO : 出力されるバッファのピクチャ種別、キャプチャ順カウンタの問題解決後対応
	tcase_add_test (tc_chain, test_property_combination);
#endif

	tcase_add_test (tc_chain, test_check_caps);

#if 0	// TODO : 出力されるバッファのピクチャ種別、キャプチャ順カウンタの問題解決後対応
	tcase_add_test (tc_chain, test_encode_prop01);
#if 0	/* max-gop-length must be greater than b-pic-mode */
	tcase_add_test (tc_chain, test_encode_prop02);
	tcase_add_test (tc_chain, test_encode_prop03);
	tcase_add_test (tc_chain, test_encode_prop04);
#endif

	tcase_add_test (tc_chain, test_encode_prop11);
#if 0	/* max-gop-length must be greater than b-pic-mode */
	tcase_add_test (tc_chain, test_encode_prop12);
	tcase_add_test (tc_chain, test_encode_prop13);
	tcase_add_test (tc_chain, test_encode_prop14);
#endif

	tcase_add_test (tc_chain, test_encode_prop21);
	tcase_add_test (tc_chain, test_encode_prop22);
#if 0	/* max-gop-length must be greater than b-pic-mode */
	tcase_add_test (tc_chain, test_encode_prop23);
	tcase_add_test (tc_chain, test_encode_prop24);
#endif

	tcase_add_test (tc_chain, test_encode_prop31);
	tcase_add_test (tc_chain, test_encode_prop32);
	tcase_add_test (tc_chain, test_encode_prop33);
	tcase_add_test (tc_chain, test_encode_prop34);
#endif

	return s;
}

int
main (int argc, char **argv)
{
	int nf;
	
	Suite *s = acmh264enc_suite ();
	SRunner *sr = srunner_create (s);
	
	gst_check_init (&argc, &argv);
	
	srunner_run_all (sr, CK_NORMAL);
	nf = srunner_ntests_failed (sr);
	srunner_free (sr);
	
	return nf;
}

/*
 * End of file
 */