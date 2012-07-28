#!/bin/sh
# Copyright (C) 2011-2012 Luka Perkov <freecwmp@lukaperkov.net>

. /lib/functions.sh
. /usr/share/shflags/shflags.sh
. /usr/share/freecwmp/defaults

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

load_script() {
	. $1 
}

get_functions_names=""
set_functions_names=""
handle_scripts() {
	local section="$1"
	config_get prefix "$section" "prefix"
	config_list_foreach "$section" 'location' load_script
	config_get get_function_names "$section" "get_function_name"
	config_get set_function_names "$section" "set_function_name"
}

config_load freecwmp
config_foreach handle_scripts "scripts"

if [ "$action" = "get" ]; then
	for function_name in $get_function_names
	do
		$function_name "$2"
	done
fi

if [ "$action" = "set" ]; then
	for function_name in $set_function_names
	do
		$function_name "$2" "$3"
	done
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
	/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} set freecwmp.@local[0].event="boot"
	/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} commit

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
