# Server
This is a simple http server written in c utilizing pthread as the method of multi threading.
How to run:
./server *port_number* *root_web_directory*

# Test Script*
This repository contains a simple test script for the server. The script is a bash sheel script that will test the basic operation of the server to check correct return of content, headers and MIME type. 

## Files
* **test_script.sh** shell script that runs wget, grep, and diff to check HTTP responses to test HTTP GET request. 
* **/test/** folder containing test files that will be served by the server.

## Running
Make sure that the **test_script.sh** is executable then run:

./test_script.sh *name_of_your_server* *port_number* 

For example, if your server is compiled to *myserver*, run:

./test_script.sh myserver 8080 


*Test script provided by DR Chris Culnane 