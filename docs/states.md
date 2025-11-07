# SCHEDULER

- Create a global scheduler.  
- Each instance creates its own work task on initialization (keep a scheduler pointer inside the class struct).  
- Implement a work function that switches on available states and executes the correct functions.  
- If an instance completes or encounters an error, destroy the task.  

---

# HTTP Server States

## Initializing
**Prerequisite:** Pointer to HTTP server must exist.  
- Verify the pointer argument is not `NULL`.  
- Set initial values in the `HTTPServer` struct, including the callback function passed as an argument.  
- Call `TCP_server_init`.  
- Create a task in the scheduler.  
- If TCP initialization fails → set state to **ERROR**.  
- Otherwise → set state to **Connecting**.  

## Listening
**Prerequisite:** Successful `tcp_server_init`.  
- Wait for client connection.  

## Connecting
**Prerequisite:** Must be in **Listening** state.  
- Wait for callback response from TCP server confirming connection established.  
- If callback indicates failure: send error callback to caller and return to **Listening** state.  

## Connected
**Prerequisite:** Successful TCP callback in **Connecting** state.  
- Create `http_connection` instance.  
- Call `http_connection_init` with accept file descriptor, the `http_con` instance, and the HTTP server instance as context plus callback arguments.  
- On success → return server state to **Listening**.  
- On failure → send error callback to caller and return to **Listening**.  

## ERROR
**Prerequisite:** State set to error during **Initializing**.  
- Retry `http_init` every 30 seconds up to 3 times (90 seconds total).  
- If still failing after 90 seconds: send timeout error callback and move to **Disposing**.  

## Disposing
**Prerequisite:** Still in **ERROR** after retries or shutdown triggered.  
- If initialization failed or server is shutting down, dispose of TCP server and HTTP server instances.  
- (Add self-destruct protocol.)  

---

# HTTP Connection States

## Initializing
**Prerequisite:** Successful connection from HTTP server state.  
- Initialize TCP client.  
- Set TCP client file descriptor to the value passed in.  
- Create a task in the scheduler.  

## Read First Line
**Prerequisite:** Successful HTTP server connection.  
- Read first line of incoming data, verify valid request type, size, and HTTP version.  
- Update struct with method, domain, path, and parameters.  
- If method is valid → set appropriate method state (GET / OPTIONS / POST / etc.).  
- Otherwise → callback error and set state to **ERROR**.  

## Read Headers
- Read and parse HTTP headers.  

## Method States
- Execute logic for the specific HTTP method (GET / POST / OPTIONS / etc.).  

## Read Body
- Read and process HTTP request body.  

## Error
- Retry logic and cleanup on failure.  
- Send error callback and dispose on repeated failure.  

## Disposing
- Cleanup and destroy connection resources.  
