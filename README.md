# Show Me Dependencies
The main goal is to identify all dependencies on user input. Soon there will be more details;)

**Requires PHP7+** so far.
# Examples
### Boring
```php
<?php
echo $_GET['foo'];

print_r(smd_pump());

/*
Output: Array
(
    [get] => Array
        (
            [0] => foo
        )

    [post] => Array
        (
        )

    [request] => Array
        (
        )

    [cookie] => Array
        (
        )

)
*/
```

### Custom storage
```php
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

print_r(smd_pump());

/*
Output: Array
(
    [get] => Array
        (
            [0] => isset_get
            [1] => fetch_get
        )

    [post] => Array
        (
            [0] => fetch_post
        )

    [request] => Array
        (
        )

    [cookie] => Array
        (
        )

)
*/
```
