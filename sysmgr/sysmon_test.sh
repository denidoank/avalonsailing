#/bin/sh

./fm_client_test --task-name fmtest  testdev < fmtest.trace &
./sysmon_test --logtostderr --debug fmtest_sysmgr.conf fmtest_sysmon.conf
