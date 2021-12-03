# Generic & experimental chat protocol
This is an old code written in 2019 containing server and a client for experimental chat protocol designed in mind to use JSON for serialization and boost library for networking. 

PS: This code isn't perfect at all (~~to be honest it's quite junky~~).

## Compiling
To build this code, you need to install boost (for boost::asio) and nlohmann-json libraries. 

Next, head up to the directory with cloned code and build everything with:

```
make 
make -f client.mk
```