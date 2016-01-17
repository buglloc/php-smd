--TEST--
Check collecting with manipulations
--SKIPIF--
<?php if (!extension_loaded("smd")) print "skip"; ?>
--INI--
smd.enable = 1
error_reporting = E_NONE
--FILE--
<?php
$get = $_GET;
unset($get['some']);
$get['another'] = 1;

echo $_GET['bar'];

var_dump(smd_pump());
?>
--EXPECTF--
array(4) {
  ["get"]=>
  array(1) {
    [0]=>
    string(3) "bar"
  }
  ["post"]=>
  array(0) {
  }
  ["request"]=>
  array(0) {
  }
  ["cookie"]=>
  array(0) {
  }
}