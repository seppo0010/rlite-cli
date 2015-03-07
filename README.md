# rlite-cli

Command-line interface to interact with rlite databases.

## Example

```
$ ./rlite-cli
rlite :memory:> set key value
OK
rlite :memory:> get key
"value"
```

With persistence

```
$ ./rlite-cli -s mydb.rld
rlite mydb.rld> set key value
OK
rlite mydb.rld> exit
$ ./rlite-cli -s mydb.rld
rlite mydb.rld> get key
"value"
rlite mydb.rld> exit
```
