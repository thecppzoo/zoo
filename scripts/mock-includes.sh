COMPILER=$1
OUTPUT=$2

shift
shift

${COMPILER} -nostdinc -nostdinc++ -E -I${OUTPUT} $* 2>&1 > /dev/null | \
	sed -n 's/^\(.*\)fatal error: '"'"'\(.*\)'"'"' file not found\(.*\)$/\2/p' |
	while read FILE
	do
		ADDENDUM="${OUTPUT}/$FILE"
		mkdir -p $(dirname $ADDENDUM)
		echo $ADDENDUM
		echo "__include_directive__ <$FILE>" > ${ADDENDUM}
	done
