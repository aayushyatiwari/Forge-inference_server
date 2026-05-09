# This script sends multiple requests to the server to test its functionality. 
#It includes requests to the root endpoint, the /hello endpoint with and without query parameters for name and age.
# curl localhost:8080/reverse/ && 
# curl localhost:8080/reverse/ && 
# curl localhost:8080/reverse/hello?name=John && 
# curl localhost:8080/reverse/hello?name=John&age=30


# curl -X POST localhost:8080/reverse -H "Content-Type: application/json" -d '{"text": "hello"}' &&
# curl -X POST localhost:8080/reverse -H "Content-Type: application/json" -d '{"text": "my"}' &&
# curl -X POST localhost:8080/reverse -H "Content-Type: application/json" -d '{"text": "name"}' &&
# curl -X POST localhost:8080/reverse -H "Content-Type: application/json" -d '{"text": "aayushya"}' 

# to print line use -w "\n" to add new line
curl -X POST localhost:8080/reverse -H "Content-Type: application/json" -d '{"text": "hello"}' -w "\n" &&
curl -X POST localhost:8080/reverse -H "Content-Type: application/json" -d '{"text": "my"}' -w "\n" &&
curl -X POST localhost:8080/reverse -H "Content-Type: application/json" -d '{"text": "name"}' -w "\n" &&
curl -X POST localhost:8080/reverse -H "Content-Type: application/json" -d '{"text": "aayushya"}' -w "\n" 