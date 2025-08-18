## Open Watcom-X Fork
|Project Build Status||Download|
|---|---|---|
|![Build Status](https://github.com/open-watcom/open-watcom-x/actions/workflows/ci-build.yml/badge.svg)|CI Build|[Github Release](https://github.com/open-watcom/open-watcom-x/releases/tag/Last-CI-build) , [GitHub Actions Build](https://github.com/open-watcom/open-watcom-x/actions/workflows/ci-build.yml)|
|![Build Status](https://github.com/open-watcom/open-watcom-x/actions/workflows/release.yml/badge.svg)|Current Release Build|[Github Release](https://github.com/open-watcom/open-watcom-x/releases/tag/Current-build) , [GitHub Actions Build](https://github.com/open-watcom/open-watcom-x/actions/workflows/release.yml)|
|![Build Status](https://github.com/open-watcom/open-watcom-x/actions/workflows/coverity.yml/badge.svg) ![Coverity Scan](https://scan.coverity.com/projects/2647/badge.svg?flat=1)|Coverity Scan|[Analysis Results](https://scan.coverity.com/projects/open-watcom-open-watcom-x) , [GitHub Actions Build](https://github.com/open-watcom/open-watcom-x/actions/workflows/coverity.yml)|
||Releases Archive|[**All Github Releases**](https://github.com/open-watcom/open-watcom-x/releases)
|![WikiDocs](https://github.com/open-watcom/open-watcom-x/workflows/WikiDocs/badge.svg)[](https://github.com/open-watcom/open-watcom-x/actions?query=workflow%3AWikiDocs)|Wiki Documentation||
###
## Welcome to the Open Watcom-X Project!

This is a fork of the Open Watcom v2 project. It is more varied and wild than the originating project. If you value stability and consistency over experimentation, please consider using that project instead.

This specific branch details
----------------------------

master: This branch's changes that generally track upstream
moderate: A branch for moderate changes, usually from what is learned by the wild branch. This is often reset to master.
wild: A branch for wild changes that can greatly change functions, behaviors, and rearrange things. This is often reset to master.

Source Tree Layout
------------------

Open Watcom allows you to place the source tree almost anywhere (although
we recommend avoiding paths containing spaces). The root of the source
tree should be specified by the `OWROOT` environment variable in `setvars`
(as described in [`Build`](https://github.com/open-watcom/open-watcom-x/wiki/Build) document). All relative paths in this document are
taken relative to `OWROOT` location. Also this document uses the backslash
character as a path separator as is normal for DOS, Windows, and OS/2. Of
course on Linux systems a slash character should be used instead.

The directory layout is as follows:

    bld
      - The root of the build tree. Each project has a subdirectory under
        bld. For example:
          bld\cg       -> code generator
          bld\cc       -> C compiler
          bld\plusplus -> C++ compiler
          (see projects.txt for details)

    build
      - Various files used by building tools. Of most interest are the
        *.ctl files which are scripts for the builder tool (see below)
        and make files (makeint et al.).

    build\binbuild
      - This is where all build tools created during phase one are placed.

    docs
      - Here is everything related to documentation, sources and tools.

    distrib
      - Contains manifests and scripts for building binary distribution
        packages.

    contrib
      - Third party source code which is not integral part of Open Watcom.
        This directory contains especially several DOS extenders.

    rel
      - This is default location where the software we actually ship gets
        copied after it is built - it matches the directory structure of
        our shipping Open Watcom C/C++/FORTRAN tools. You can install the
        system by copying the rel directory to your host and then setting
        several environment variables.

        Note: the rel directory structure is created on the fly. The
        location of rel tree can be changed by OWRELROOT environment
        variable.

OpenWatcom Installation
-----------------------
[Installer installation instruction](https://open-watcom.github.io/open-watcom-x-wikidocs/c_readme.html#Installation) in Documentation (OW Wiki).

OpenWatcom Building
-------------------
[Building instruction](https://github.com/open-watcom/open-watcom-x/wiki/Build) in OW Wiki.

[Open Watcom Licence](https://github.com/open-watcom/open-watcom-x/blob/master/license.txt)
---------------------
