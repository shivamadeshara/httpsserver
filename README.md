httpserver.c
Components:

struct RequestObj():
    This function is the outline for a request sturcut which will store the request's command, version, URI, and content length if given.
    *Inspired by Vincent(former TA) section videos
Request newRequest():
    This functions creates a new request object and returns a pointer to the request. 

void delete_request(Request dr):
    This frees the memory allocated by a request and frees all the components of the request as well. 

int header_parse(char* buf, Request req):
    This function takes in a buffer and goes through each one of the headers and then searches for the "Content-Length" key and stores its value in the req->content-length. 
    *the parsing methods were inspired by Mitchell Elliot's discussion section and Offices 

int request_parse(char8 buf, Request req):
    This function goes through a buffer and parses through the request line and then stores the method, uri, and version into the req request struct. It also returns and error if the request is bad. 
    *the parsing methods were inspired by Mitchell Elliot's discussion section and Office Hours

char *status_message(int code):
    This function takes in an interger and then returns the corresponding message based on the status code inputted. 

void get(Request req, int out):
    This function does all of the functionalitity of a get request. It first checks if the URI is valid and if the file can be accessed. If it can then it will return the correct response to the user and then send the contents of the file to the client. If this is not the case then the get function will return the corresponding error message and send the response to the client. 

void put(Request req, int out, char *remainder):
    This function does all of the put functionality request. It first checks if the URI provided exists and then checks if the file already exists and then it will return the corresponding "OK" response and then it will overwrite the file contents with the message body of the request. Otherwise it will create a new file with the URI's name and then write the message body to the file and then return the corresponding "Created" response. 

other_response(int code, int out):
    This function is used to write a response based on a the code argument put in. Used for the bad_responses

fatal_error(const char* msg):
    Handles any other error handling
    *from lecture examples
    
main():
    This is where the socket setup happens and where the socket listens for connections. The client requests are also stores in here and all the other functions are used here. This is where the client request and response are handled. At the end of it the socket is closed and all memeory is freed. 
    *this is based off of the echoserver posted in the practica examples