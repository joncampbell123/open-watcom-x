# Master Builder Control File
# ===========================

# Default to build.log
set LOGFNAME=build
set LOGEXT=log
set CPCMD=acopy
set CCCMD=accopy
set BLDRULE=

# Set default output directory
[ BLOCK .<OWRELROOT> . ]
#=======================
    set OWRELROOT=<OWROOT>\rel
    [ IFDEF <BLD_HOST> UNIX ]
        set OWRELROOT=<OWROOT>/rel
    [ ENDIF ]

# Configure executable extension: DOS, OS/2 and Windows use .exe, others nothing
[ BLOCK <BLD_HOST> NT OS2 DOS ]
#==============================
    set CMDEXT=.exe
    set DYEXT=.dll

[ BLOCK <BLD_HOST> UNIX ]
#========================
    set CMDEXT=
    set DYEXT=.so

[ BLOCK . . ]
#============

[ BLOCK .<1> . ]
#===============
    # default rule
    set 1=build

[ BLOCK .<1> .. ]
#================
    # default rule
    set 1=build

[ BLOCK <1> boot ]
#=================
    set BLDRULE=boot
    set LOGFNAME=<1>
    echo **** Building the <PROJNAME> bootstrap

[ BLOCK <1> bootclean ]
#======================
    set BLDRULE=bootclean
    set LOGFNAME=btclean
    echo **** BOOTCLEAN rule

[ BLOCK <1> build build1 build2 build3 ]
#=======================================
    set BLDRULE=build
    set LOGFNAME=<1>
    echo **** BUILD rule

[ BLOCK <1> rel rel1 rel2 rel3 pass ]
#====================================
    set BLDRULE=rel
    set LOGFNAME=<1>
    echo **** REL rule

[ BLOCK <1> clean clean1 clean2 clean3 passclean ]
#=================================================
    set BLDRULE=clean
    set LOGFNAME=<1>
    echo **** CLEAN rule

[ BLOCK <1> cprel cprel1 cprel2 cprel3 ]
#=======================================
    set BLDRULE=cprel
    set LOGFNAME=<1>
    # with 'cprel', we want to force the copy to occur
    set CPCMD=copy
    set CCCMD=ccopy
    echo **** CPREL rule

[ BLOCK <1> install ]
#====================
    set BLDRULE=<1>
    set LOGFNAME=<1>
    echo **** INSTALL rule

[ BLOCK <1> instclean ]
#======================
    set BLDRULE=<1>
    set LOGFNAME=insclean
    echo **** INSTCLEAN rule

[ BLOCK <1> missing ]
#====================
    set BLDRULE=<1>
    set LOGFNAME=<1>
    echo **** INSTALL MISSING rule

[ BLOCK <1> test ]
#=================
    set BLDRULE=test
    set LOGFNAME=<1>
    echo **** TEST rule

[ BLOCK <1> buildtest ]
#======================
    set BLDRULE=test
    set BUILDTEST=1
    set LOGFNAME=bldtest
    echo **** TEST rule

[ BLOCK <1> testclean ]
#======================
    set BLDRULE=testclean
    set LOGFNAME=tstclean
    echo **** TESTCLEAN rule

[ BLOCK <1> cleanlog ]
#======================
    set BLDRULE=cleanlog
    set LOGFNAME=cleanlog
    echo **** CLEANLOG rule

[ BLOCK <1> docs ]
#=================
    set BLDRULE=docs
    set LOGFNAME=doc
    echo **** DOC rule

[ BLOCK <1> docsclean ]
#======================
    set BLDRULE=docsclean
    set LOGFNAME=docclean
    echo **** DOCCLEAN rule

[ BLOCK .<PROJDIR> . ]
    error PROJDIR must be set!

[ BLOCK . . ]

[ LOG <PROJDIR>/<LOGFNAME>.<LOGEXT> ]
