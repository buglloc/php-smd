--TEST--
Check collecting with object storage
--SKIPIF--
<?php if (!extension_loaded("smd")) print "skip"; ?>
--INI--
smd.enable = 1
error_reporting = E_NONE
--FILE--
<?php
class Request
{
    protected $storage = [];

    function store(array $settings) {
        $this->storage = array_merge($this->storage, $settings);
        return $this;
    }

    function get($name) {
        return $this->storage[$name];
    }

    function exists($name) {
        return isset($this->storage[$name]);
    }
}

$get = (new Request)->store($_GET);
$post = (new Request)->store($_POST);

// Check isset
$get->exists('isset_get');
// Check fetch from _GET
$get->get('fetch_get');
// Check fetch from _POST
$post->get('fetch_post');

var_dump(smd_pump());
?>
--EXPECTF--
array(4) {
  ["get"]=>
  array(2) {
    [0]=>
    string(9) "isset_get"
    [1]=>
    string(9) "fetch_get"
  }
  ["post"]=>
  array(1) {
    [0]=>
    string(10) "fetch_post"
  }
  ["request"]=>
  array(0) {
  }
  ["cookie"]=>
  array(0) {
  }
}