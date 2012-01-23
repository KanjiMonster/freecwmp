#!/bin/sh
# Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>

. /lib/functions.sh
. /usr/share/shflags/shflags.sh
. /usr/share/freecwmp/defaults
. /usr/share/freecwmp/functions/device_info
. /usr/share/freecwmp/functions/management_server
. /usr/share/freecwmp/functions/lan_device
. /usr/share/freecwmp/functions/wan_device
. /usr/share/freecwmp/functions/voice_service
. /usr/share/freecwmp/functions/x_freecwmp_org

# define a 'name' command-line string flag
DEFINE_boolean 'newline' false 'do not output the trailing newline' 'n'
DEFINE_boolean 'value' false 'output values only' 'v'
DEFINE_boolean 'empty' false 'output empty parameters' 'e'
DEFINE_boolean 'last' false 'output only last line ; for parameters that tend to have huge output' 'l'
DEFINE_boolean 'debug' false 'give debug output' 'd'
DEFINE_boolean 'dummy' false 'echo system commands' 'D'
DEFINE_string 'url' '' 'file to download [download only]' 'u'
DEFINE_string 'size' '' 'size of file to download [download only]' 's'

FLAGS_HELP=`cat << EOF
USAGE: $0 [flags] command [parameter] [values]
command:
  get
  set
  download
  factory_reset
  reboot
EOF`

FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

if [ ${FLAGS_help} -eq ${FLAGS_TRUE} ]; then
	exit 1
fi

if [ ${FLAGS_newline} -eq ${FLAGS_TRUE} ]; then
	ECHO_newline='-n'
fi

case "$1" in
	set)
		action="set"
		;;
	get)
		action="get"
		;;
	download)
		action="download"
		;;
	factory_reset)
		action="factory_reset"
		;;
	reboot)
		action="reboot"
		;;
esac

if [ -z "$action" ]; then
	echo invalid action \'$1\'
	exit 1
fi

if [ ${FLAGS_debug} -eq ${FLAGS_TRUE} ]; then
	echo "[debug] started at \"`date`\""
fi

if [ "$action" = "get" ]; then
	parameter_name=$2
	case "$parameter_name" in
		InternetGatewayDevice.DeviceInfo.*)
		get_device_info "$parameter_name"
		;;
		InternetGatewayDevice.ManagementServer.*)
		get_management_server "$parameter_name"
		;;
		InternetGatewayDevice.LANDevice.*)
		get_lan_device "$parameter_name"
		;;
		InternetGatewayDevice.WANDevice.*)
		get_wan_device "$parameter_name"
		;;
		InternetGatewayDevice.Services.VoiceService.*)
		get_voice_service "$parameter_name"
		;;
		InternetGatewayDevice.Services.*)
		get_voice_service "$parameter_name"
		;;
		InternetGatewayDevice.*)
		get_device_info "$parameter_name"
		get_management_server "$parameter_name"
		get_lan_device "$parameter_name"
		get_wan_device "$parameter_name"
		get_voice_service "$parameter_name"
		;;
	esac
fi

if [ "$action" = "set" ]; then
	parameter_name=$2
	value=$3
	case "$parameter_name" in
		InternetGatewayDevice.DeviceInfo.*)
		set_device_info "$parameter_name" "$value"
		;;
		InternetGatewayDevice.ManagementServer.*)
		set_management_server "$parameter_name" "$value"
		;;
		InternetGatewayDevice.WANDevice.*)
		set_wan_device "$parameter_name" "$value"
		;;
		InternetGatewayDevice.LANDevice.*)
		set_lan_device "$parameter_name" "$value"
		;;
		InternetGatewayDevice.Services.VoiceService.*)
		set_voice_service "$parameter_name" "$value"
		;;
	esac
fi

if [ "$action" = "download" ]; then

	rm /tmp/freecwmp_download 2> /dev/null
	wget -O /tmp/freecwmp_download "${FLAGS_url}" > /dev/null 2>&1

	dl_size=`ls -l /tmp/freecwmp_download | awk '{ print $5 }'`
	if [ ! "$dl_size" -eq "${FLAGS_size}" ]; then
		rm /tmp/freecwmp_download 2> /dev/null
		exit 1
	fi
fi

if [ "$action" = "factory_reset" ]; then
	if [ ${FLAGS_dummy} -eq ${FLAGS_TRUE} ]; then
		echo "# factory_reset"
	else
		jffs2_mark_erase "rootfs_data"
		sync
		reboot
	fi
fi

if [ "$action" = "reboot" ]; then
	uci $uci_options set freecwmp.@local[0].event="boot"
	uci $uci_options commit

	if [ ${FLAGS_dummy} -eq ${FLAGS_TRUE} ]; then
		echo "# reboot"
	else
		sync
		reboot
	fi
fi

if [ ${FLAGS_debug} -eq ${FLAGS_TRUE} ]; then
	echo "[debug] exited at \"`date`\""
fi

