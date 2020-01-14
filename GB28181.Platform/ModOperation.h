#pragma once
struct mod_op_t {
	enum class ot_sdk :unsigned int
	{
		// SDK 操作
		default_none = 0,
		ctrl_action,
		search_status,
		search_record,
		search_guid,
		search_guid_no_sdp,
		decoder_invite,
		stop_alarm,
		notify_broadcast,
		insert_video_record,
		update_video_record,
		//gb14 add
		search_config,
		device_config,
		search_realplayurl,
		search_playbackurl,
		stopPlayUrl,
		DecoderDivision,
		DecoderStatus,
		DecoderInfo,
		// 解码器ACK不做处理
		decoder_ack,
		decoder_bye,
		subscribe_alarm,
		max_count
	};

	enum class ot_devinfo : unsigned int {
		// DevInfo操作
		default_none,
		query_info = 200,
		update_data,
		//ot_devinfo_search_catalog,
		subscribe,
		send_decoder_subscribe,
		search_perset,
		subscribe_notify,
		online_status,
		max_count
	};

	enum class ot_sipcom : unsigned int {
		// SIPCom 操作
		default_none,
		notify_reponse = 300,
		query_result,
		invite_reponse,
		video_finished,
		alarm_request,
		invite_broadcast,
		broadcast_ack,
		subscribe_reponse,
		subscribe_notify,
		subscribe_timeout_notify, // subscribe expires
		subscribe_noresrc_notify,   // no resource from subscribe request
		decoder_status_notify_result,
		video_excepion_bye,
		video_ptz,
		record_inqury,
		max_count
	};

	enum class ot_rtsp : unsigned int {
		// RTSP 操作
		default_none,
		play_ctrl_start = 400,
		play_ctrl_start_broadcast,
		play_broadcast,
		play_ctrl,
		play_ctrl_stop,
		play_no_sdp,
		play_with_sdp,
		record_start,
		record_stop,
		stop_all_play,
		max_count
	};

	enum class ot_gb_adaptor : unsigned int {
		default_none,
		register_response,
		call_invite_response,
		call_bye_response,
		call_message,
		message,
		message_response,
		subscribe_response,
		subscribe_notify,
		max_count
	};


	union  u_op_type_t {
		ot_sdk action_sdk;
		ot_devinfo action_devinfo;
		ot_sipcom action_sipcom;
		ot_rtsp action_rtsp;
		ot_gb_adaptor action_gb_adaptor;
		u_op_type_t() {
			action_sdk = ot_sdk::default_none;
			action_devinfo = ot_devinfo::default_none;
			action_sipcom = ot_sipcom::default_none;
			action_rtsp = ot_rtsp::default_none;
			action_gb_adaptor = ot_gb_adaptor::default_none;
		};
		u_op_type_t(ot_sdk sdk_op)
		{
			action_sdk = sdk_op;
		}
		u_op_type_t(ot_sipcom sipcom_op)
		{
			action_sipcom = sipcom_op;
		}
		u_op_type_t(ot_devinfo devinfo_op)
		{
			action_devinfo = devinfo_op;
		}
		u_op_type_t(ot_rtsp rtsp_op)
		{
			action_rtsp = rtsp_op;
		}

		u_op_type_t(ot_gb_adaptor gb_adaptor_op)
		{
			action_gb_adaptor = gb_adaptor_op;
		}

	};
	u_op_type_t u_op_type;
};
