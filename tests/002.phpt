--TEST--
Check simple collecting
--SKIPIF--
<?php if (!extension_loaded("smd")) print "skip"; ?>
--INI--
smd.enable = 1
error_reporting = E_NONE
--FILE--
<?php
echo $_GET['_GET_foo'];
isset($_GET['_GET_bar']);

echo $_REQUEST['_REQUEST_foo'];
isset($_REQUEST['_REQUEST_bar']);

echo $_POST['_POST_foo'];
isset($_POST['_POST_bar']);

echo $_COOKIE['_COOKIE_foo'];
isset($_COOKIE['_COOKIE_bar']);

var_dump(smd_pump());
?>
--EXPECTF--
array(4) {
  ["get"]=>
  array(2) {
    [0]=>
    string(8) "_GET_foo"
    [1]=>
    string(8) "_GET_bar"
  }
  ["post"]=>
  array(2) {
    [0]=>
    string(9) "_POST_foo"
    [1]=>
    string(9) "_POST_bar"
  }
  ["request"]=>
  array(2) {
    [0]=>
    string(12) "_REQUEST_foo"
    [1]=>
    string(12) "_REQUEST_bar"
  }
  ["cookie"]=>
  array(2) {
    [0]=>
    string(11) "_COOKIE_foo"
    [1]=>
    string(11) "_COOKIE_bar"
  }
}