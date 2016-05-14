## Abstract

.NET extensions allows for a decentralized assembly store with decentralized versioning and servicing policies. To allow for this decentralization, the dotnet SDK, runtime host and dotnet extensions have to agree on a set of contracts. This document outlines how extensions fit into the NuGet based acquisition of assemblies and the CLI's project model all the way through the runtime host and servicing of components in extensions. Few direct benefits of dotnet extensions are:

+ .NET extensions are located in a well-known location so restore operations could be avoided.
+ Extensions allow for the applications to be further lightweight than carry all their dependencies beyond the shared framework.
  + This reduces copying time during `dotnet publish`.
+ Allows for independent versioning and servicing policies for first party assemblies.

## Acquisition

Extensions are acquired by downloading an extension MSI or the zip. At some point in the future we'd have:

```
dotnet install extension Fabrikam.NETCore.Ext 1.0.0
```

## Specifying Extensions

Extensions must be specified in the project manifest for build or execution (to answer **what** extension assemblies are required.)

Extensions must be specified in the project.lock.json for locating compile time assemblies and runtime manifest for runtime assemblies (to answer **where** extension assemblies are present.)

### Project Manifest
CLI users specify extensions as follows in their **project.json**:

For a NETCore app, irrespective of whether it is published as a standalone or a portable app with respect to the shared framework, an extension can be specified as follows:

```
{
    "dependencies": {
        "Fabrikam.NETCore.Ext": {
            "version": "1.0.0",
            "type": "platform-extension" <---
        }
    }
}
```

For a fully self-contained standalone app (contains its full dependency closure app local), an extension must be specified **without** `type: platform-extension`:

```
{
    "dependencies": {
        "Fabrikam.NETCore.Ext": {
            "version": "1.0.0"
        }
    }
}
```

**Note** A NETCore app can be published **standalone** with respect to `Microsoft.NETCore.App` but it can still take advantage of extensions it depends on.

### Project.lock.json

```
{
    "Fabrikam.NETCore.Ext/1.0.0": {
        "type": "platform-extension"
    }
}
```

Build uses this pointer to merge the extension's project.lock.json from `C:\Program Files (x86)\Fabrikam.NETCore.Ext\1.0.0\project.lock.json` to get an unified view of the whole compile time and run time dependencies.


### Runtime Manifest

All dependency types that are `platform-extension` should be specified in the runtimeconfig.json (or deps.json **[OPEN]**) in the order they are declared in the project.json

```
{
    "runtimeOptions": {
        "framework": {
            "Microsoft.NETCore.App": "1.0.0"
        },
        "extensions": {                               <--
            "Fabrikam.NETCore.Ext": "1.0.0",
            "Contoso.NETCore.Ext": "1.1.0"
        }
    }
}
```

## Dotnet Restore

  * Restore checks if the `type` of a dependency is `platform-extension` and ignores physically restoring all the nested dependencies for such a dependency.
  * The project.lock.json for the user project only contains references to the extensions.
  * Full closure is expanded in-memory at build time by searching the extensions folder for (`project.lock.json`). See Extension Layout below.

## Dotnet Build

  * Build merges extension's lock files using the references from the user's lock file.
  * Build uses compile time assemblies at a well known dotnet-extensions location as references to `csc`.
  * If it cannot find these assemblies in dotnet extensions, then the user is prompted to do an installation of the extension.
       * At some point we can do a `dotnet install extension` to make this easy.
  * Build produces `runtimeconfig.json` which contains the `extension` names and versions so the runtime host can lookup these assemblies.

## Dotnet Publish
  * Publish ignores anything that is specified as `"type": "platform-extension"` to be copied into the user app base.

## Extension Layout

Extension layout is allowed to take 3 forms: either it is an assembly store or it contain a `extpaths.json` which redirects to other assembly stores or both (redirection/storage.)

+ **Extension Assembly Store**: Contains a set of assemblies in a well-known layout.
    * Also contains a policy file (`extpolicy.json`) that specifies: patch roll forward, layout of the store (**ex:** nupkg, flat).
    * Extension's project.lock.json
```
        + C:\Program Files (x86)\Fabrikam
                + x86\
                + x64\                -- (crossgen assemblies)
                + pkgs\               -- (runtime assemblies + reference assemblies)
                + project.lock.json   -- (extension's lock file)
                + extpolicy.json      -- (extension location policy and layout)
```

+ **Extension Assembly Store Redirection**: This is a JSON file (`extpaths.json`) with redirection to multiple **Extension Assembly Stores**. Example:

```
    {
        "paths": {
            "C:\Program Files (x86)\Fabrikam": {    <-- This is an assembly store.
                "pathType": "absolute"
            }
        }
    }
```

```
     C:\Program Files (x86)\dotnetextensions
            + Fabrikam.NETCore.Ext
                + 1.0.0
                    + Fabrikam.NETCore.Ext.extpaths.json
```


+ When the extension folder acts as both a storage and a redirector, the `extpolicy.json` indicates whether the `extpaths` or the current location takes preference.

**Note** Any cycles in the redirections should be ignored.
**Note** Environment variables (ex: `%ProgramFiles%`) in the path names will be expanded using Win32 `ExpandEnvironmentStrings` or POSIX `wordexp()`


## Installation

Extensions can be installed to a machine wide location and also unzipped when machine wide locations are not accessible (CI jobs.)

The well known extension folder is `ProgramFiles(x86)\dotnetextensions`

**Sample Layout** in this folder is:

```
    C:\Program Files (x86)\dotnetextensions
            + Fabrikam.NETCore.Ext
                + 1.0.0
                    + Fabrikam.NETCore.Ext.extpaths.json
                + 1.0.1
                    + x86\
                    + x64\
                    + pkgs\
                    + project.lock.json
                    + extpolicy.json
```

We will also allow an override `--dotnetextensionsroot` argument specified to the `dotnet` host and other SDK commands. Example: `dotnet build|publish --dotnetextensionsroot` so that this location is used instead of the default location. This is to support CI scenarios which wants customized extensions in a non-machine wide location.


## First party servicing with extensions

An `extpath.json` file can be specified with servicing paths and the order specified will be used:

```
    {
        "paths": {
            "C:\Program Files (x86)\FabrikamServicing": {
                "pathType": "absolute"
            },
            "C:\Program Files (x86)\Fabrikam": {
                "pathType": "absolute"
            }
        }
    }
```


