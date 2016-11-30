## Abstract

.NET extensions allows for a decentralized assembly store with decentralized versioning and servicing policies. To allow for this decentralization, the runtime host and the extensions have to agree on a set of contracts. This document outlines how extensions fit into the NuGet based acquisition and the CLI's project model all the way through the runtime host and servicing of components in extensions.

## Installation

Extensions contain two main components:

1. A policy file that specifies various probing paths.
2. A set of assemblies in a known layout.

Extensions can be installed to a machine wide location and also unzipped when machine wide locations are not accessible (CI jobs.)

1. **Global Extensions**: Installed by default on any location on the machine - This is done by the developer of the extension. The policy file for the extension is placed in `ProgramFiles(x86)\dotnetextensions\{Extension Name}\{Arch}.ext.json`
2. **Local Extension**: Installed in `ProgramFiles(x86)\dotnetextensions\{Extension Name}\` or unzipped under `dotnet\..\dotnetextensions\{Extension Name}`


### Global Extensions

* Extensions are installed in a system wide folder. Example: `C:\Program Files (x86)\Fabrikam\`.
* The policy file should be placed in the following location: `C:\Program Files (x86)\dotnetextensions\Fabrikam.NETCore.Ext\x64.ext.json` with the following contents:

```
{
    "paths": {
        "C:\Program Files (x86)\Fabrikam\x64": {
            "layout": "nupkg",
            "applyPatches": false,
            "pathType": "absolute"
        },
        "C:\Program Files (x86)\Fabrikam\pkgs": {
            "layout": "nupkg",
            "applyPatches": false,
            "pathType": "absolute"
        }
    }
}
```

The runtime host probes the paths in the order they are specified in the `paths` section. On windows, the policy files have to be placed in `ProgramFiles(x86)\dotnetextensions` for 32-bit processes and for 64-bit processes. The supported architectures are: `x86` and `x64`.

### Local Extensions

Local extensions opt-in to install or unzip under `dotnetextensions` with the below suggested layout:

* `dotnet\..\dotnetextensions\Fabrikam.NETCore.Ext\1.0.0\`
  - `x64.ext.json`
  - `x86.ext.json`
  - `pkgs`
  - `x64`
  - `x86`

```
{
    "paths": {
        "x64": {
            "layout": "nupkg",
            "applyPatches": false,
            "pathType": "relative"
        },
        "pkgs": {
            "layout": "nupkg",
            "applyPatches": false,
            "pathType": "relative"
        }
    }
}
```
 
`pathType` indicates that the paths to probe are relative to the `ext.json` file. **Note** If you choose to use a local extension, you can still use global paths with `pathType: absolute`.

Note that when machine wide settings are not modifiable, extensions can still be placed locally inside a directory `dotnetextensions`, a sibling of the `dotnet` root folder, (for example, when using zip ball of `dotnet FX`.)

## Global vs. Local Extension Resolution

`dotnet.exe` looks up the sibling directory of its own directory, named `dotnetextensions`. If it cannot find the specified policy file, then the global location `ProgramFiles(x86)\dotnetextensions` is looked up to find the policy file.

## Specifying Extensions

By default, all extension files in the known locations are looked up and parsed by the runtime host. If you do not want the extension locations to be probed then a section called `extensions` can be specified in the `project.json`. When specified, only extensions listed in the section are probed.

```
{
    "dependencies": {
        "Fabrikam.NETCore.Ext": "1.0.0"
    },
    "extensions": {
        "Fabrikam.NETCore.Ext": {}
    }
}
```

**Should we instead do this [OPEN] for a much broader scope?**

The below way helps `dotnet restore` by not restoring the extension as it is already available on the machine. If the extension folder contained compilation and runtime references in a fixed format, then dotnet build can also take advantage of the `dotnetextensions`. This saves a lot of restoration time and disk space issues, if all machines have extensions deployed on the OS image or on a folder. Since restore won't resolve these dependencies, the extension folder will carry a `deps.json` that has the dependencies which `dotnet build` and the host can use. This is similar to the Shared FX location.

```
{
    "dependencies": {
        "Fabrikam.NETCore.Ext": {
            "version": "1.0.0",
            "type": "platform-extension"
        }
    }
}
```

### runtimeconfig.json

When `extensions` section is specified in the `project.json`, the `runtimeconfig.json` contains this section copied verbatim.
