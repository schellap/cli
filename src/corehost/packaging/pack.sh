#!/usr/bin/env bash

usage()
{
   echo "Usage: ${BASH_SOURCE[0]} --osx [1/0] --arch x64/x86/arm --hostbindir path-to-binaries"
   exit 1
}

init_distro_name()
{
    # Detect Distro
    if [ "$(cat /etc/*-release | grep -cim1 ubuntu)" -eq 1 ]; then
        export __distro_name=ubuntu
    elif [ "$(cat /etc/*-release | grep -cim1 centos)" -eq 1 ]; then
        export __distro_name=rhel
    elif [ "$(cat /etc/*-release | grep -cim1 rhel)" -eq 1 ]; then
        export __distro_name=rhel
    elif [ "$(cat /etc/*-release | grep -cim1 debian)" -eq 1 ]; then
        export __distro_name=debian
    else
        export __distro_name=""
    fi
}

set -e

# determine current directory
SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ "$SOURCE" != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done

# initialize variables
__project_dir="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
__build_arch=
__dotnet_host_bin_dir=
__is_osx="0"
__distro_name=

# parse arguments
while [ "$1" != "" ]; do
        lowerI="$(echo $1 | awk '{print tolower($0)}')"
        case $lowerI in
        -h|--help)
            usage
            exit 1
            ;;
        --arch)
            shift
            __build_arch=$1
            ;;
        --osx)
            shift
            __is_osx=$1
            ;;
        --hostbindir) 
            shift
            __dotnet_host_bin_dir=$1
            ;;
        *)
        echo "Unknown argument to pack.sh $1"; exit 1
    esac
    shift
done

# validate args
if [ -z $__dotnet_host_bin_dir ]; then
    usage
fi
if [ -z $__build_arch ]; then
    usage
fi

# setup msbuild
"$__project_dir/init-tools.sh"

# acquire dependencies
pushd "$__project_dir/deps"
"$__project_dir/Tools/dotnetcli/bin/dotnet" restore --source "https://dotnet.myget.org/F/dotnet-core" --packages "$__project_dir/packages"
popd

# cleanup existing packages
rm -rf $__project_dir/bin

# build to produce nupkgs
__corerun="$__project_dir/Tools/corerun"
__msbuild="$__project_dir/Tools/MSBuild.exe"

__targets_param="TargetsLinux=true"
if [ "$__is_osx" -eq "1" ]; then
    __targets_param="TargetsOSX=true"
else
    init_distro_name
fi

git rev-parse HEAD > "$__project_dir/version.txt"
echo "Obtaining commit hash for version file... git rev-parse HEAD"
cat "$__project_dir/version.txt"

$__corerun $__msbuild $__project_dir/projects/Microsoft.NETCore.DotNetHostPolicy.builds /p:Platform=$__build_arch /p:DotNetHostBinDir=$__dotnet_host_bin_dir /p:$__targets_param /p:DistroName=$__distro_name
$__corerun $__msbuild $__project_dir/projects/Microsoft.NETCore.DotNetHostResolver.builds /p:Platform=$__build_arch /p:DotNetHostBinDir=$__dotnet_host_bin_dir /p:$__targets_param /p:DistroName=$__distro_name
$__corerun $__msbuild $__project_dir/projects/Microsoft.NETCore.DotNetHost.builds /p:Platform=$__build_arch /p:DotNetHostBinDir=$__dotnet_host_bin_dir /p:$__targets_param /p:DistroName=$__distro_name

echo $__corerun $__msbuild $__project_dir/projects/Microsoft.NETCore.DotNetHostPolicy.builds /p:Platform=$__build_arch /p:DotNetHostBinDir=$__dotnet_host_bin_dir /p:$__targets_param
exit 0

