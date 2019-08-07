#!/bin/sh
# *****************************************************************
# gitupdf.sh - update git repository if build fails
# *****************************************************************
#
# after failure transfer log files back to GitHub repository
#

gitupdf_proc()
{
    if [ "$OWTRAVIS_DEBUG" = "1" ]; then
        set -x
        GITVERBOSE1=-v
        GITVERBOSE2=-v
    else
        GITVERBOSE1=--quiet
        GITVERBOSE2=
    fi

    echo_msg="gitupdf.sh - skipped"

    if [ "$TRAVIS_EVENT_TYPE" = "push" ] || [ "$TRAVIS_EVENT_TYPE" = "cron" ]; then
        case "$TRAVIS_BUILD_STAGE_NAME" in
            "Bootstrap" | "Build1" | "Build2" | "Build3" | "Documentation" | "Release" | "Release windows")
                #
                # clone GitHub repository
                #
                git clone $GITVERBOSE1 --branch=master https://${GITHUB_TOKEN}@github.com/${OWTRAVIS_LOGS_REPO_SLUG}.git $OWTRAVIS_LOGS_DIR
                #
                # copy build log files to git repository tree
                #
                if [ "$TRAVIS_OS_NAME" = "osx" ]; then
                    OWLOGDIR=$OWTRAVIS_LOGS_DIR/logs/osx
                elif [ "$TRAVIS_OS_NAME" = "windows" ]; then
                    OWLOGDIR=$OWTRAVIS_LOGS_DIR/logs/windows
                else
                    OWLOGDIR=$OWTRAVIS_LOGS_DIR/logs/linux
                fi
                if [ ! -d $OWLOGDIR ]; then 
                    mkdir -p $OWLOGDIR; 
                fi
                cp $OWBINDIR/*.log $OWLOGDIR/
                cp $OWDOCSDIR/*.log $OWLOGDIR/
                cp $OWDISTRDIR/ow/*.log $OWLOGDIR/
                #
                # commit new log files to GitHub repository
                #
                cd $OWTRAVIS_LOGS_DIR
                git add $GITVERBOSE2 -f .
                if [ "$TRAVIS_OS_NAME" = "osx" ]; then
                    git commit $GITVERBOSE1 -m "Travis CI build $TRAVIS_JOB_NUMBER (build failure) - log files (OSX)"
                elif [ "$TRAVIS_OS_NAME" = "windows" ]; then
                    git commit $GITVERBOSE1 -m "Travis CI build $TRAVIS_JOB_NUMBER (build failure) - log files (Windows)"
                else
                    git commit $GITVERBOSE1 -m "Travis CI build $TRAVIS_JOB_NUMBER (build failure) - log files (Linux)"
                fi
                git push $GITVERBOSE1 -f origin
                cd $TRAVIS_BUILD_DIR
                echo_msg="gitupdf.sh - done"
                ;;
            "Tests")
                #
                # clone GitHub repository
                #
                git clone $GITVERBOSE1 --branch=master https://${GITHUB_TOKEN}@github.com/${OWTRAVIS_LOGS_REPO_SLUG}.git $OWTRAVIS_LOGS_DIR
                #
                # copy build log files to git repository tree
                #
                case "$OWTRAVISTEST" in
                    "WASM")
                        test -d $OWTRAVIS_LOGS_DIR/logs/linux/wasmtest || mkdir -p $OWTRAVIS_LOGS_DIR/logs/linux/wasmtest
                        cp $OWSRCDIR/wasmtest/result.log $OWTRAVIS_LOGS_DIR/logs/linux/wasmtest/
                        cp $OWSRCDIR/wasmtest/test.log $OWTRAVIS_LOGS_DIR/logs/linux/wasmtest/
                        ;;
                    "C")
                        test -d $OWTRAVIS_LOGS_DIR/logs/linux/ctest || mkdir -p $OWTRAVIS_LOGS_DIR/logs/linux/ctest
                        cp $OWSRCDIR/ctest/result.log $OWTRAVIS_LOGS_DIR/logs/linux/ctest/
                        cp $OWSRCDIR/ctest/test.log $OWTRAVIS_LOGS_DIR/logs/linux/ctest/
                        ;;
                    "CXX")
                        test -d $OWTRAVIS_LOGS_DIR/logs/linux/plustest || mkdir -p $OWTRAVIS_LOGS_DIR/logs/linux/plustest
                        cp $OWSRCDIR/plustest/result.log $OWTRAVIS_LOGS_DIR/logs/linux/plustest/
                        cp $OWSRCDIR/plustest/test.log $OWTRAVIS_LOGS_DIR/logs/linux/plustest/
                        ;;
                    "F77")
                        test -d $OWTRAVIS_LOGS_DIR/logs/linux/f77test || mkdir -p $OWTRAVIS_LOGS_DIR/logs/linux/f77test
                        cp $OWSRCDIR/f77test/result.log $OWTRAVIS_LOGS_DIR/logs/linux/f77test/
                        cp $OWSRCDIR/f77test/test.log $OWTRAVIS_LOGS_DIR/logs/linux/f77test/
                        ;;
                    "CRTL")
                        test -d $OWTRAVIS_LOGS_DIR/logs/linux/clibtest || mkdir -p $OWTRAVIS_LOGS_DIR/logs/linux/clibtest
                        cp $OWSRCDIR/clibtest/result.log $OWTRAVIS_LOGS_DIR/logs/linux/clibtest/
                        cp $OWSRCDIR/clibtest/test.log $OWTRAVIS_LOGS_DIR/logs/linux/clibtest/
                        ;;
                    *)
                        ;;
                esac
                #
                # commit new log files to GitHub repository
                #
                cd $OWTRAVIS_LOGS_DIR
                git add $GITVERBOSE2 -f .
                git commit $GITVERBOSE1 -m "Travis CI build $TRAVIS_JOB_NUMBER (test failure) - log files (Linux)"
                git push $GITVERBOSE1 -f origin
                cd $TRAVIS_BUILD_DIR
                echo_msg="gitupdf.sh - done"
                ;;
            "Update build" | "Update build windows")
                ;;
            *)
                ;;
        esac
    fi

    echo "$echo_msg"

    return 0
}

gitupdf_proc $*
