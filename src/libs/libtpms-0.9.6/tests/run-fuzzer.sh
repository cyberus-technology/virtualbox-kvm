#!/usr/bin/env bash

# For the license, see the LICENSE file in the root directory.

DIR=${PWD}/$(dirname "$0")
ROOT=${DIR}/..
WORKDIR=$(mktemp -d)

function cleanup()
{
	rm -rf ${WORKDIR}
}

trap "cleanup" QUIT EXIT

pushd $WORKDIR

${DIR}/fuzz $@ ${DIR}/corpus-execute-command
rc=$?

popd

exit $rc
