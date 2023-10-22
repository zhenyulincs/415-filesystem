from test_scripts.cd_test import cd_test
from test_scripts.cp2_test import cp2_test
from test_scripts.ls_test import ls_test
from test_scripts.md_test import md_test
from test_scripts.mv_test import mv_test
from test_scripts.pwd_test import pwd_test
from test_scripts.rm_test import rm_test
from test_scripts.touch_test import touch_test

print("=======================cd test=====================")
md_test().reset_log()
md_test().clear_volume()
md_test().test()
touch_test().test()
cd_test().test()
# pwd_test().test()
# print("=======================cp2 test=====================")
# md_test().clear_volume()
# md_test().test()
# touch_test().test()
# cd_test().test()
# cp2_test().test()
# print("=======================ls test=====================")
# md_test().clear_volume()
# md_test().test()
# touch_test().test()
# cd_test().test()
# cp2_test().test()
# ls_test().test()
# print("=======================rm test=====================")
md_test().clear_volume()
md_test().test()
touch_test().test()
cd_test().test()
cp2_test().test()
ls_test().test()
rm_test().test()
