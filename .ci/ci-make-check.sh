#!/bin/bash

set -e

if [ "$1" = "release-ready" ] || [ "$1" = "mingw" ] || [ "$1" = "coverity" ] || [ "$1" = "options-enabled" ] || [ "$1" = "options-disabled" ] ; then
  exit 0
fi

NUM_TRIES=5

travis_fold start "ninja-test"
travis_time_start "ninja-test"
if [ "$1" = "codecov" ] ; then
  for tries in $(seq 1 ${NUM_TRIES}); do
    export EFL_TEST_ECORE_CON_IPV6=1
    meson test -t 120 -C build --wrapper dbus-launch && break
    cat build/meson-logs/testlog-dbus-launch.txt
    if [ $tries != ${NUM_TRIES} ] ; then echo "tests failed, trying again!" ; fi
      false
  done
  curl -s https://codecov.io/bash | bash -s -
  exit 0
fi

if [ "$DISTRO" != "" ] ; then
  for tries in $(seq 1 ${NUM_TRIES}); do
    (docker exec --env EINA_LOG_BACKTRACE="0" --env EIO_MONITOR_POLL=1 $(cat $HOME/cid) meson test -t 120 -C build --wrapper dbus-launch ) && break
    docker exec --env EIO_MONITOR_POLL=1 $(cat $HOME/cid) cat build/meson-logs/testlog-dbus-launch.txt
    if [ $tries != ${NUM_TRIES} ] ; then echo "tests failed, trying again!" ; fi
      false
  done
  ret=$?
elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
  meson test -t 120 -C build --wrapper dbus-launch
  ret=$?
else
  meson test -t 120 -C build --wrapper dbus-launch
  ret=$?
fi
travis_time_finish "ninja-test"
travis_fold end "ninja-test"

exit $ret
