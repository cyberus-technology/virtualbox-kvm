#!/usr/bin/env bash

# For the license, see the LICENSE file in the root directory.

ROOT=${abs_top_builddir:-$(pwd)/..}
TESTDIR=${abs_top_testdir:-$(dirname "$0")}
DIR=${PWD}

WORKDIR=$(mktemp -d)

. ${TESTDIR}/common

function cleanup()
{
	rm -rf ${WORKDIR}
}

trap "cleanup" QUIT EXIT

pushd $WORKDIR &>/dev/null

${DIR}/tpm2_pcr_read
rc=$?

fs=$(get_filesize NVChip)
[ $? -ne 0 ] && exit 1
if [ $fs -ne 176832 ]; then
	echo "Error: Unexpected size of NVChip file."
	echo "Expected: 131072"
	echo "Got     : $fs"
	rc=1
fi

popd &>/dev/null

exit $rc
