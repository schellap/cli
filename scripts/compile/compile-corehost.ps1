#
# Copyright (c) .NET Foundation and contributors. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
#

. $PSScriptRoot\..\common\_common.ps1

header "Building corehost"
function main
{
    pushd "$RepoRoot\src\corehost"
    try {
        if (!(Test-Path "cmake\$Rid")) {
            mkdir "cmake\$Rid" | Out-Null
        }
        cd "cmake\$Rid"
        cmake ..\.. -G "Visual Studio 14 2015 Win64"
        $pf = $env:ProgramFiles
        if (Test-Path "env:\ProgramFiles(x86)") {
            $pf = (cat "env:\ProgramFiles(x86)")
        }
        $BuildConfiguration = $Configuration
        if ($Configuration -eq "Release") {
            $BuildConfiguration = "RelWithDebInfo"
        }
        & "$pf\MSBuild\14.0\Bin\MSBuild.exe" ALL_BUILD.vcxproj /p:Configuration="$BuildConfiguration"
        if (!$?) {
            Write-Host "Command failed: $pf\MSBuild\14.0\Bin\MSBuild.exe" ALL_BUILD.vcxproj /p:Configuration="$BuildConfiguration"
            Exit 1
        }

        if (!(Test-Path $HostDir)) {
            mkdir $HostDir | Out-Null
        }
    
        Package-CoreHost

    } finally {
        popd
    }
}

function Bin-Place
{
    param(
        [string] $TargetDir = ""
    )

    cp "$RepoRoot\src\corehost\cmake\$Rid\cli\$BuildConfiguration\corehost.exe" $TargetDir
    cp "$RepoRoot\src\corehost\cmake\$Rid\cli\dll\$BuildConfiguration\hostpolicy.dll" $TargetDir

    if (Test-Path "$RepoRoot\src\corehost\cmake\$Rid\cli\$BuildConfiguration\corehost.pdb")
    {
        cp "$RepoRoot\src\corehost\cmake\$Rid\cli\$BuildConfiguration\corehost.pdb" $TargetDir
    }
    if (Test-Path "$RepoRoot\src\corehost\cmake\$Rid\cli\dll\$BuildConfiguration\hostpolicy.pdb")
    {
        cp "$RepoRoot\src\corehost\cmake\$Rid\cli\dll\$BuildConfiguration\hostpolicy.pdb" $TargetDir
    }
}

function Package-CoreHost
{
    # Create nupkg for the host
    $PackageVersion=[System.IO.File]::ReadAllText("$RepoRoot\src\corehost\.version").Trim()
    $NuSpecContents = 
@"
<?xml version="1.0" encoding="utf-8"?>
<package xmlns="http://schemas.microsoft.com/packaging/2011/08/nuspec.xsd">
    <metadata>
        <id>Microsoft.DotNet.CoreHost</id>
        <version>{0}</version>
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
"@ -f $PackageVersion

    $ContentTypesXml =
@"
<?xml version="1.0" encoding="utf-8"?><Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml" /><Default Extension="nuspec" ContentType="application/octet" /><Default Extension="exe" ContentType="application/octet" /><Default Extension="pdb" ContentType="application/octet" /><Default Extension="dll" ContentType="application/octet" /><Default Extension="psmdcp" ContentType="application/vnd.openxmlformats-package.core-properties+xml" /></Types>
"@

    $RelsContent =
@"
<?xml version="1.0" encoding="utf-8"?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Type="http://schemas.microsoft.com/packaging/2010/07/manifest" Target="/Microsoft.DotNet.CoreHost.nuspec" Id="R4f414a783aa14934" /><Relationship Type="http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" Target="/package/services/metadata/core-properties/d1aa6e3b9d354a24915fc891446aba30.psmdcp" Id="Rf698eab28bde4c20" /></Relationships>
"@

    $PackageName="Microsoft.DotNet.CoreHost"
    $PackageRoot="$HostDir\$PackageName"
    $PackageNative="$PackageRoot\runtimes\$RID\native"
    New-Item -ItemType Directory -Force -Path $PackageNative | Out-Null
    New-Item -ItemType Directory -Force -Path "$PackageRoot\_rels" | Out-Null
    $Utf8Encoding = New-Object System.Text.UTF8Encoding($False)
    [System.IO.File]::WriteAllLines("$PackageRoot\$PackageName.nuspec", $NuSpecContents, $Utf8Encoding)
    [System.IO.File]::WriteAllLines("$PackageRoot\[Content_Types].xml", $ContentTypesXml, $Utf8Encoding)
    [System.IO.File]::WriteAllLines("$PackageRoot\_rels\", $ContentTypesXml, $Utf8Encoding)

    $TargetDirs = @("$HostDir", "$PackageNative")
    foreach ($TargetDir in $TargetDirs) {
        Bin-Place -TargetDir $TargetDir
    }

    $NuPkgFile = "$HostDir\$PackageName.$PackageVersion.nupkg"
    If (Test-Path $NuPkgFile){
	    Remove-Item $NuPkgFile
    }

    Add-Type -Assembly System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::CreateFromDirectory($PackageRoot, $NuPkgFile, "Optimal", $false)
}

main
