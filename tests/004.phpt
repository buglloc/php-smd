--TEST--
Check collecting from function call
--SKIPIF--
<?php if (!extension_loaded("smd")) print "skip"; ?>
--INI--
smd.enable = 1
error_reporting = E_NONE
--FILE--
<?php
function fromGet($name, $default = null) {
  return $_GET[$name]?: $default;
}

function fromPostRef(array &$array, $name, $default = null) {
  return $array[$name]?: $default;
}

fromGet('_GET_foo');
fromPostRef($_POST, '_POST_foo');
fromPostRef($_COOKIE, '_COOKIE_foo');

var_dump(smd_pump());
?>
--EXPECTF--
array(4) {
  ["get"]=>
  array(1) {
    [0]=>
    string(8) "_GET_foo"
  }
  ["post"]=>
  array(1) {
    [0]=>
    string(9) "_POST_foo"
  }
  ["request"]=>
  array(0) {
  }
  ["cookie"]=>
  array(1) {
    [0]=>
    string(11) "_COOKIE_foo"
  }
}