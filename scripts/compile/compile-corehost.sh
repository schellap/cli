#!/usr/bin/env bash
#
# Copyright (c) .NET Foundation and contributors. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
#

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ "$SOURCE" != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

source "$DIR/../common/_common.sh"
source "$DIR/../common/_clang.sh"

header "Building corehost"

pushd "$REPOROOT/src/corehost" 2>&1 >/dev/null
[ -d "cmake/$RID" ] || mkdir -p "cmake/$RID"
cd "cmake/$RID"
cmake ../.. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE:STRING=$CONFIGURATION
make

# Publish to artifacts
[ -d "$HOST_DIR" ] || mkdir -p $HOST_DIR
if [[ "$OSNAME" == "osx" ]]; then
   COREHOST_LIBNAME=libhostpolicy.dylib
else
   COREHOST_LIBNAME=libhostpolicy.so
fi

# Create nupkg for the host
local PACKAGE_VERSION=$(cat "$REPOROOT/src/corehost/.version")
read -d '' NUSPEC_CONTENTS <<"NUSPEC"
<?xml version="1.0" encoding="utf-8"?>
<package xmlns="http://schemas.microsoft.com/packaging/2011/08/nuspec.xsd">
    <metadata>
        <id>Microsoft.DotNet.CoreHost</id>
        <version>${PACKAGE_VERSION}</version>
        <title>.NET CoreCLR Runtime Host</title>
        <authors>Microsoft</authors>
        <owners>Microsoft</owners>
        <licenseUrl>http://go.microsoft.com/fwlink/?LinkId=329770</licenseUrl>
        <projectUrl>https://github.com/dotnet/cli</projectUrl>
        <iconUrl>http://go.microsoft.com/fwlink/?LinkID=288859</iconUrl>
        <requireLicenseAcceptance>true</requireLicenseAcceptance>
        <description>Provides the runtime host for .NET CoreCLR</description>
        <releaseNotes>Initial release</releaseNotes>
        <copyright>Copyright © Microsoft Corporation</copyright>
    </metadata>
</package>
NUSPEC

local PACKAGE_NAME=Microsoft.DotNet.CoreHost
local PACKAGE_ROOT=$HOST_DIR/$PACKAGE_NAME
local PACKAGE_NATIVE=$PACKAGE_ROOT/runtimes/$RID/native/
mkdir -p $PACKAGE_NATIVE
echo $NUSPEC_CONTENTS > $PACKAGE_ROOT/$COREHOST_PACKAGE_NAME.$COREHOST_VERSION.nuspec

TARGET_DIRS=("$HOST_DIR" "$PACKAGE_NATIVE")
for TARGET_DIR in "${TARGET_DIRS[@]}"
do
  cp "$REPOROOT/src/corehost/cmake/$RID/cli/corehost" $TARGET_DIR
  cp "$REPOROOT/src/corehost/cmake/$RID/cli/dll/${COREHOST_LIBNAME}" $TARGET_DIR
done

zip -r $PACKAGE_NAME $PACKAGE_NAME
mv $PACKAGE_NAME.zip $PACKAGE_NAME.nupkg

popd 2>&1 >/dev/null
